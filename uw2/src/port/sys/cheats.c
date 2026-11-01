/* UW2's cheats for tests (Exhume's runtime/port/sys/cheat.c, which reads them from an input
   script's `cheat` lines, refuses them unless EXHUME_TEST_CHEATS=1, records and replays them,
   and runs each at the main loop's safe point when cheat_ready says it may). Each goes through
   the game's own routines, so the state it leaves is one the game itself could hold, and a game
   saved after it loads in DOS:

     goto LEVEL X Y          do_teleport, as a teleport trap, and the 3D view's change bit 3,
                             as a step of the player's physics sets it: the move is carried
                             out by the main loop's new_player_pos (ChangeLevel when LEVEL is
                             another, then player_setup). A square with no free place near
                             it is refused on this level; on another, which is not loaded until
                             the move, the game kills the player it cannot place, as it does
                             after any teleport; LEVEL 1 to 80 (a level the game has: the
                             game stops with its own error for one it lacks), X and Y 1 to 62
     give ITEM [QTY [QUALITY]] [drop]
                             CreateObj (the game's allocator), QTY of a stack (1 to 511) and
                             QUALITY (0 to 63; else the game's 40), into the first empty
                             backpack slot on show (AddToInventory) when the player can carry it
                             (EncumCheck); else, or with drop, at the player's feet, as a
                             conversation puts an object there (put_at, stay_centered). ITEM is
                             the game's item id (docs/FORMATS.md; src/include/items.h), decimal
                             or 0x hex, an object a player can carry: 0x000 to 0x03F or 0x080 to
                             0x13F
     set FIELD VALUE         hp (1 to the maximum), mana (0 to the maximum), str, dex, int (1 to
                             99, then player_compute: the maxima, carrying weight str * 13 + 300
                             among them, as a level gained does), or a skill by name (0 to 30; enum
                             Skill, lower case: attack, defense ... swimming)

   The numbers are checked when the script is read; what depends on the game (a free object, a
   free square) when the cheat runs, which then says what failed and changes nothing. */
#include "compat.h"
#include "event.h"
#include "gfx.h"
#include "combat.h"
#include "inv.h"
#include "items.h"
#include "map.h"
#include "motion.h"
#include "object.h"
#include "player.h"
#include "sys.h"
#include "ui.h"

static const char *skill_names[NUM_SKILLS] = {
    "attack", "defense", "barehand", "sword", "axe", "mace", "missile", "mana", "lore",
    "casting", "traps", "search", "track", "stealth", "repair", "charisma", "picklock",
    "acrobat", "appraise", "swimming"
};

/* in the game, on its 3D view, with nothing on the cursor, no move pending and the player
   alive */
static int cheat_ready(void)
{
    return in_game && scrnum == 0 && GameInputMode == 0 && CursorObjPtr == 0 && NewPlayerX == 0
        && ThePlayer != 0 && ThePlayer->hp > 0;
}

static const char *c_goto(int argc, char *argv[], int run)
{
    long level, x, y;
    if (argc != 3 || cheat_num(argv[0], 1, NUM_LEVELS, &level) || cheat_num(argv[1], 1, 62, &x)
        || cheat_num(argv[2], 1, 62, &y))
        return "a level 1 to 80 and a square 1 to 62, 1 to 62";
    if (!run) return NULL;
    if (do_teleport(ThePlayer, (int)x, (int)y, (int)level) != 0x10)
        return "no free square there";
    /* the 3D view's change handler 3, new_player_pos, carries the move out, as after each step
       of the player's physics (PLAYMOVE.C's editchng(10)): a player standing still takes none */
    editchng(8);
    return NULL;
}

static const char *c_give(int argc, char *argv[], int run)
{
    long item = -1, qty = 0, quality = -1, v;
    int drop = 0, i, n = 0, slot;
    struct Object far *obj;
    for (i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "drop")) {
            drop = 1;
            continue;
        }
        if (n == 3 || cheat_num(argv[i], n == 1 ? 1 : 0, n == 2 ? 63 : 0x1FF, &v))
            return "an item, then a quantity 1 to 511 and a quality 0 to 63 if given";
        if (n == 0) item = v;
        else if (n == 1) qty = v;
        else quality = v;
        n++;
    }
    if (n == 0) return "an item";
    if (!((item >= 0 && item <= 0x3F) || (item >= 0x80 && item <= 0x13F)))
        return "an object a player can carry: 0x000 to 0x03F or 0x080 to 0x13F";
    if (!run) return NULL;
    if ((obj = CreateObj((int)item, 0)) == 0) return "no free object";
    if (qty) {
        if (!OBJ_ISQUANT(obj)) {
            Obj_Free(obj);
            return "not an object with a quantity";
        }
        obj->ol.f.link = (uint16)qty;
    }
    if (quality >= 0) obj->qn.f.quality = (uint16)quality;
    if (!drop && OpenBag == 0 && EncumCheck(obj))
        for (slot = 11; slot <= 18; slot++)             /* the backpack (inv.h) */
            if (Inventory[slot].f.index == 0 && ItemFitsSlot(obj, slot) > 0) {
                AddToInventory(obj, slot);
                RedisplayInvSlot(slot);
                return NULL;
            }
    stay_centered = 1;
    i = near_mob_put_at(ThePlayer, obj, 6, 1);
    stay_centered = 0;
    return i ? NULL : "no room at the player's feet";
}

static const char *c_set(int argc, char *argv[], int run)
{
    long v;
    int k;
    if (argc != 2) return "a field and a value";
    if (!strcmp(argv[0], "hp") || !strcmp(argv[0], "mana")) {
        if (cheat_num(argv[1], 0, 255, &v)) return "a value 0 to 255";
        if (!run) return NULL;
        if (argv[0][0] == 'h') {
            if (v < 1 || v > playerdat->avghit) return "hp 1 to the player's maximum";
            ThePlayer->hp = (unsigned char)v;
        } else {
            if (v > player->max_mana) return "mana 0 to the player's maximum";
            player->play_mana = (unsigned char)v;
        }
        panel_check_hpmp();
        return NULL;
    }
    /* attr[]: strength, dexterity, intelligence (PLAYDATA.C) */
    k = !strcmp(argv[0], "str") ? 0 : !strcmp(argv[0], "dex") ? 1 : !strcmp(argv[0], "int") ? 2 : -1;
    if (k >= 0) {
        if (cheat_num(argv[1], 1, 99, &v)) return "a value 1 to 99";
        if (!run) return NULL;
        playerdat->attr[k] = (unsigned char)v;
    } else {
        for (k = 0; k < NUM_SKILLS; k++)
            if (!strcmp(argv[0], skill_names[k])) break;
        if (k == NUM_SKILLS) return "hp, mana, str, dex, int or a skill (attack, defense ... swimming)";
        if (cheat_num(argv[1], 0, 30, &v)) return "a value 0 to 30";
        if (!run) return NULL;
        player->skills[k] = (unsigned char)v;
    }
    /* the maxima again, as a level gained does; the vitals kept within them */
    player_compute(0);
    if (ThePlayer->hp > playerdat->avghit) ThePlayer->hp = playerdat->avghit;
    if (player->play_mana > player->max_mana) player->play_mana = player->max_mana;
    panel_check_hpmp();
    return NULL;
}

/* test-inv: the backpack's slots and the open bag's on show, each ITEM:QTY (e-takes' and
   split-all's fixture), the object on the cursor, as UW1's */
static const char *c_inv(int argc, char *argv[], int run)
{
    int slot;
    struct Object far *o;
    (void)argv;
    if (argc) return "no arguments";
    if (!run) return NULL;
    fprintf(stderr, "fixture: inv");
    for (slot = 11; slot <= (OpenBag ? 27 : 18); slot++)
        if (slot != 19 && (o = AskInventory(slot)) != 0)
            fprintf(stderr, " %d=0x%03X:%d", slot, OBJ_ITEM(o), OBJ_ISQUANT(o) ? o->ol.f.link : 1);
    if (CursorObjPtr) fprintf(stderr, " cursor=0x%03X:%d", OBJ_ITEM(CursorObjPtr), OBJ_ISQUANT(CursorObjPtr) ? CursorObjPtr->ol.f.link : 1);
    fprintf(stderr, "\n");
    return NULL;
}

extern int16 hitobj, fromwho;           /* combat/COMBAT.C: the blow's target and attacker */
char far do_miss(int hit);              /* combat/COMBAT.C */

/* combat SLOT DAMAGE|miss|block|status: damage-numbers' and enemy-health's fixture, as UW1's:
   the game's own damage_critter on the creature in that mobile slot, or COMBAT.C's do_miss as the
   player's blow, or what the combat text shows of it */
static const char *c_combat(int argc, char *argv[], int run)
{
    long slot, amount = 0;
    int kind, saved_from, saved_hit, status;
    struct Object far *o;
    if (argc != 2 || cheat_num(argv[0], 2, 255, &slot)) return "a mobile slot 2..255 and damage 1..255, miss, block or status";
    status = !strcmp(argv[1], "status");
    kind = !strcmp(argv[1], "miss") ? COMBAT_MISS : !strcmp(argv[1], "block") ? COMBAT_BLOCK : COMBAT_DAMAGE;
    if (!status && kind == COMBAT_DAMAGE && cheat_num(argv[1], 1, 255, &amount)) return "damage 1..255, miss, block or status";
    if (!run) return NULL;
    o = Obj_IntTMem((int)slot);
    if (OBJ_MAJOR(o) != MAJOR_CREATURE || !o->hp) return "not a live creature";
    if (status) {
        CombatFrame frame;
        int i, visible = 0, maximum = 0;
        combat_text_read(&frame);
        for (i = 0; i < frame.count; i++) if (frame.actors[i].slot == slot) {
            visible = 1; maximum = frame.actors[i].maximum;
        }
        fprintf(stderr, "fixture: combat slot=%ld hp=%u visible=%d maximum=%d floats=%d\n",
                slot, (unsigned)o->hp, visible, maximum, frame.floats);
    } else if (kind == COMBAT_DAMAGE) damage_critter(o, (unsigned char)amount, ThePlayer);
    else {
        saved_from = fromwho; saved_hit = hitobj;
        fromwho = 1; hitobj = (int)slot;
        do_miss(kind == COMBAT_MISS ? 0 : 1);
        fromwho = saved_from; hitobj = saved_hit;
    }
    editchng(2);
    return NULL;
}

/* test-mobiles: the live creatures of the level, slot, item and tile (to aim the fixtures) */
static const char *c_mobiles(int argc, char *argv[], int run)
{
    int i;
    (void)argv;
    if (argc) return "no arguments";
    if (!run) return NULL;
    for (i = 2; i < NUM_MOBILE; i++) {
        struct Object far *o = Obj_IntTMem(i);
        if (OBJ_MAJOR(o) == MAJOR_CREATURE && o->hp)
            fprintf(stderr, "fixture: mobile slot=%d item=0x%03X hp=%u tile=%d,%d\n", i, OBJ_ITEM(o), (unsigned)o->hp,
                    OBJ_HOMEX(o), OBJ_HOMEY(o));
    }
    return NULL;
}

/* view SLOT: turn the player to face the creature in that mobile slot (the camera fixture) */
static const char *c_view(int argc, char *argv[], int run)
{
    long slot;
    int dx, dy, distance;
    struct Object far *o;
    if (argc != 1 || cheat_num(argv[0], 2, 255, &slot)) return "a mobile slot 2..255";
    if (!run) return NULL;
    o = Obj_IntTMem((int)slot);
    if (OBJ_MAJOR(o) != MAJOR_CREATURE || !o->hp) return "not a live creature";
    dx = (OBJ_HOMEX(o) << 8) + (OBJ_FINEX(o) << 5) - PN.x;
    dy = (OBJ_HOMEY(o) << 8) + (OBJ_FINEY(o) << 5) - PN.y;
    if (!dx && !dy) return "camera and target coincide";
    distance = cSqRt((int32)dx * dx + (int32)dy * dy);
    PlayerHeading = PlayerFacing = (uint16)(0x4000 - cAtan2((int)((int32)dy * 32767 / distance),
                                                            (int)((int32)dx * 32767 / distance)));
    SET_HEADING(ThePlayer, (uint16)PlayerFacing >> 13);
    SET_FINEHEAD(ThePlayer, (uint16)PlayerFacing >> 8);
    PlayerPitch = 0;
    editchng(2);
    return NULL;
}

/* test-state: the level, hit points, position and god-mode's state (its fixture), as UW1's */
static const char *c_state(int argc, char *argv[], int run)
{
    (void)argv;
    if (argc) return "no arguments";
    if (run) fprintf(stderr, "fixture: state level=%d hp=%d x=%d y=%d z=%d god=%u map=%u drawn=%d panel=%d bag=%d\n", PlayerLevel,
                     ThePlayer->hp, PN.x, PN.y, PN.z, ENHANCED(ENH_GOD_MODE), ENHANCED(ENH_FULL_MAP), player->drawn,
                     RightPanel, OpenBag != 0);
    return NULL;
}

/* test-take ITEM [QTY]: e-takes (E with modern-keys) on the nearest object of that item (a stack
   of QTY), as though picked within reach */
int far port_test_take(int item, int qty);         /* port/ui/keys.c */
static const char *c_take(int argc, char *argv[], int run)
{
    long v, q = 0;
    if (argc < 1 || argc > 2 || cheat_num(argv[0], 0, 0x1FF, &v) || (argc == 2 && cheat_num(argv[1], 1, 999, &q)))
        return "an item 0x000 to 0x1FF, and a quantity 1 to 999 if given";
    if (!run) return NULL;
    if (!ENHANCED(ENH_E_TAKES)) return "e-takes is off";
    return port_test_take((int)v, (int)q) ? NULL : "none on the map";
}

static const struct cheat_cmd cmds[] = {
    { "goto", "LEVEL X Y", c_goto },
    { "give", "ITEM [QTY [QUALITY]] [drop]", c_give },
    { "set", "FIELD VALUE", c_set },
    { "test-state", "", c_state },
    { "test-mobiles", "", c_mobiles },
    { "combat", "SLOT DAMAGE|miss|block|status (combat fixture)", c_combat },
    { "view", "SLOT (camera fixture)", c_view },
    { "test-inv", "", c_inv },
    { "test-take", "ITEM [QTY] (e-takes fixture)", c_take },
};

const struct cheat_game cheat_game = { cmds, (int)(sizeof cmds / sizeof cmds[0]), cheat_ready };
