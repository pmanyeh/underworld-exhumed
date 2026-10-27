/* keys.c: --enhance modern-keys' and rune-keys' actions (docs/ENHANCEMENTS.md), UltimaHacks'
   keyAndMouseBindings in our names (John Glassmyer, MIT). Each key that stands for a click sets
   what that click sets (the position in inplist, as the region's handler reads it) and calls the
   handler, so the game decides everything as it does for the click. Port only; bound in
   game/PLAYER.C's init_player and ui/AUTOMAP.C. */
#include "compat.h"
#include "gfx.h"
#include "ui.h"
#include "player.h"
#include "combat.h"
#include "sys.h"
#include <stdlib.h>
#include <stdatomic.h>
#include "view3d.h"
#include "inv.h"
#include "sound.h"
#include "object.h"
#include "critter.h"
#include "i18n/phrases.h"

/* ui/INTERACT.C (and UW2's GAMESCR.C for the flasks and compass): no header declares these */
void far flask_info(void);
void far print_info(void);
struct Object far * far pick_3d(int how);
void far look_nothing(unsigned char how, int txt);
void far player_3dlook(void);
void far player_3duse(void);
void far player_3dtalk(void);
unsigned char far IsMobElem(struct Object far *obj);
unsigned char far InPickRange(int dist, struct Object far *obj, struct Tile far *tile);
char far BlockingTerrain(int dist, struct Object far *obj);
void far release_3d(struct Object far *obj);
void far player_3dget(void);
unsigned char far EncumCheck(struct Object far *obj);   /* inv/INVDATA.C */
unsigned char far HasOrIsObj(struct Object far *obj, int id);
extern char releaseable;                /* the object picked can be taken (pick_3d) */
extern int16 pTxtId;                    /* the texture a pick found, for look_nothing */
extern struct Object far *newPlObj;     /* the object picked, for player_3dlook and _3duse */
extern union Link far *releasePtr;      /* the list release_3d takes it from (pick_3d) */

extern int16 m3dx, m3dy, m3dw, m3dh;    /* ui/MOUSE.C: the 3D view (m3dy its bottom edge) */

/* h: as a click on the left flask, then on the right (UltimaHacks' clickFlasks) */
void far port_key_flasks(void)
{
    inplist->x = 20;
    inplist->y = 11;
    flask_info();
    inplist->x = 60;
    inplist->y = 11;
    flask_info();
}

/* g: as a click on the compass */
void far port_key_compass(void)
{
    print_info();
}

/* r, f: to the rune bag (1) or the statistics (2), or back to the inventory when it is showing
   (UltimaHacks' flipToPanel) */
void far port_key_panel(int panel)
{
    set_screen_frame(SCR_PANEL, RightPanel == panel ? 0 : panel);
}

/* z: the map (UltimaHacks' transitionToInterfaceMode(MAP)) */
void far port_key_open_map(void)
{
    newscr(2);
}

/* 1, 3, the grey Up and Down: the pitch as the original 1 and 3 (the other way round); the
   keypad's 5 levels it */
void far port_key_pitch(int dir)
{
    chg_plyp(dir);
}

/* --enhance mouse-wheel: the notches the platform sent (main.c's pointer hook), taken a notch at a
   time by sys/keyqueue.c inside KEY's replay hook as PORT_KEY_WHEEL_UP or _DOWN */
static _Atomic int wheel_notches;
void port_wheel_push(int steps) { atomic_fetch_add(&wheel_notches, steps); }
int port_wheel_take(void)
{
    int n = atomic_load(&wheel_notches);
    while (n != 0 && !atomic_compare_exchange_weak(&wheel_notches, &n, n > 0 ? n - 1 : n + 1)) {}
    return n > 0 ? 1 : n < 0 ? -1 : 0;
}

/* A wheel notch: the skills under the pointer, or an open bag, as their arrows do.
   mouse_getxy reads the game's cursor, whose motion is already recorded for replay. */
void far port_key_wheel(int down)
{
    if (RightPanel == PANEL_STATS) {
        int16 x, y;
        mouse_getxy(&x, &y);
        if (x >= 0xF0 && x <= 0x13B && y >= 0x52 && y <= 0x81)
            port_stats_scroll(down);
        return;
    }
    if (OpenBag == 0)
        return;
    if (down)
        ScrollItemsDown();
    else
        ScrollItemsUp();
}

/* c (UW1: x), v, b: close the open container, scroll the inventory down, up */
void far port_key_bag(int how)
{
    if (how == 0)
        CloseTheBag();
    else if (how == 1)
        ScrollItemsDown();
    else
        ScrollItemsUp();
}

/* The slot on show in the panel (the backpack's, or the open bag's eight) whose stack obj would
   merge with when put down on it (INVPANEL.C's AddTogether and AddToOccupiedSlot), or -1. */
static int stack_on_show(struct Object far *obj)
{
    int slot, first = OpenBag != 0 ? INV_BAG_ITEMS : INV_PACK, last = OpenBag != 0 ? INV_BAG_LAST : INV_PACK_LAST;
    struct Object far *onto;
    for (slot = first; slot <= last; slot++) {
        if ((onto = AskInventory(slot)) == 0
            || (OBJ_MAJOR(onto) == MAJOR_MISC && OBJ_MINOR(onto) == MINOR_CONTAINER))
            continue;
        if (AddTogether(obj, onto) && ItemFitsSlot(obj, slot) == 1)
            return slot;
    }
    return -1;
}

/* --enhance e-takes: e on something that can be taken puts it in the backpack's first free slot,
   as taking it (player_3dget, the same checks: reach, the weight, an owner noticing) and dropping
   it on that slot would. A whole stack goes. When the panel shows a stack it merges with, the
   backpack's or the open bag's, it goes onto that stack as dropping it there would. No free slot
   says so and leaves it there; with a bag open in the panel and no stack there to go onto, or
   out of reach, it is taken as the original takes it. */
static void take_into_pack(void)
{
    int slot = -1, onto;
    if (!InPickRange(PickDist, newPlObj, PickMap) || BlockingTerrain(PickDist, newPlObj)) {
        player_3dget();
        return;
    }
    onto = stack_on_show(newPlObj);
    if (onto < 0 && OpenBag != 0) {
        player_3dget();
        return;
    }
    if (onto < 0) {
        for (slot = INV_PACK; slot <= INV_PACK_LAST; slot++)
            if (Inventory[slot].f.index == 0 && ItemFitsSlot(newPlObj, slot) > 0)
                break;
        if (slot > INV_PACK_LAST) {
            scroll_print(port_zh() ? (char *)port_zh_lit("take.pack_full") : "There is no room in your pack.\n");
            return;
        }
    }
    if (!EncumCheck(newPlObj)) {
        game_sprint(0x5F);              /* too heavy, as the original says */
        return;
    }
    if (HasOrIsObj(newPlObj, ITEM_MOONSTONE))
        player->moonstone = 0;
    player_grabbed(newPlObj, 0);
    release_3d(newPlObj);
    if (onto >= 0) {
        AddToOccupiedSlot(newPlObj, onto);
        return;
    }
    AddToInventory(newPlObj, slot);
    RedisplayInvSlot(slot);
}

/* sys/cheats.c's test-take: e-takes on the object of that item (and that many, qty not 0) on the
   tile nearest the player, as though picked within reach (the test's fixture); 0 when there is none */
int far port_test_take(int item, int qty)
{
    struct Object far *o, *found = 0;
    struct Tile far *where = 0;
    int x, y, d, best = 0x7FFF;
    for (y = 0; y < 64; y++)
        for (x = 0; x < 64; x++)
            for (o = Obj_PtrTMem(&Map_GetAddr(x, y)->objects); o != 0; o = Obj_PtrTMem(&o->qn.link))
                if (OBJ_ITEM(o) == item && o != ThePlayer && (qty == 0 || (OBJ_ISQUANT(o) && o->ol.f.link == qty))
                    && (d = abs(x - OBJ_HOMEX(ThePlayer)) + abs(y - OBJ_HOMEY(ThePlayer))) < best) {
                    best = d;
                    found = o;
                    where = Map_GetAddr(x, y);
                }
    if (found == 0)
        return 0;
    newPlObj = found;
    releaseable = 1;
    PickDist = 0;                       /* any distance, never blocked */
    PickMap = where;                    /* the tile and its list, as pick_3d's */
    releasePtr = &where->objects;
    take_into_pack();
    return 1;
}

/* q looks (0), e uses (1) at the pointer, as a click there in look or use would (UltimaHacks'
   interactAtCursor): only in the 3D view, and no use while carrying something or aiming */
void far port_key_interact(int use)
{
    int16 x, y;

    if (GameInputMode != 0 && use)
        return;
    mouse_getxy(&x, &y);
    if (x < m3dx || x > m3dx + m3dw || y > m3dy || y < m3dy - m3dh)
        return;
    inplist->x = x - m3dx;              /* relative to the view, as pick_3d reads a click */
    inplist->y = y - (m3dy - m3dh);
    newPlObj = pick_3d(2);
    if (newPlObj == 0) {                /* nothing but terrain: a look describes it */
        if (!use)
            look_nothing(2, pTxtId);
        return;
    }
    if (!use)
        player_3dlook();
    else if (ENHANCED(ENH_E_TAKES) && releaseable)
        take_into_pack();
    else if ((IsMobElem(newPlObj) && OBJ_MAJOR(newPlObj) == 1) || (newPlObj->id & 0x1FF) == 0x1CD)
        player_3dtalk();                /* a character (or item 0x1CD): talk, as UltimaHacks */
    else
        player_3duse();
}

/* Ctrl+Alt+letter, Backspace, Space (rune-keys, UltimaHacks' runeKey): a rune the player has goes
   on the shelf as a click on it in the rune bag would; one it lacks is refused with a sound */
void far port_key_rune(int ch)
{
    int rune, col, row;
    int16 mode;

    {  /* a test switch, UW1PORT_GRANT_RUNES (Exhume's tools/enhcheck.py keys): every rune, given at the
           first rune key, since the staged saved game's player has none */
        static int granted;
        if (!granted && getenv("UW1PORT_GRANT_RUNES")) {
            granted = 1;
            player->runebag[0] = player->runebag[1] = player->runebag[2] = 0xFF;
        }
    }
    if (ch == ' ') {
        try_cast(1);
        return;
    }
    if (ch == KEY_BACKSPACE)
        rune = -1;
    else {
        if (ch < 'a' || ch > 'y' || ch == 'x')
            return;
        rune = ch == 'y' ? 23 : ch - 'a';   /* no X rune: Y is Ylem, 23 */
        if (!(player->runebag[rune >> 3] >> (7 - (rune & 7)) & 1)) {
            /* refused silently: UltimaHacks plays effect 45 (a file it shares with UW2, which has
               49 effects); UW1 has 24 (sound/SOUND.C), so there is no effect 45 to play */
            return;
        }
    }
    play_effect_here(rune < 0 ? 4 : 3, 64, 0);   /* UltimaHacks: 4 clearing, 3 a rune taken */
    col = rune & 3;                         /* mous_in_rune's slot, inverted: four to a row, */
    row = 5 - (rune >> 2);                  /* 20 to 23 at the top */
    mode = GameInputMode;
    inplist->x = rune < 0 ? 40 : 3 + col * 0x12 + 9;
    inplist->y = rune < 0 ? 10 : 0x12 + row * 0xF + 7;   /* y < 0x12 clears the shelf */
    inplist->cmd &= ~2;                     /* the left button: add, not look */
    GameInputMode = 0;
    mous_in_rune();
    GameInputMode = mode;
}
