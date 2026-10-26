/* UW1's cheats for tests (Exhume's runtime/port/sys/cheat.c, which reads them from an input
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
                             after any teleport; LEVEL 1 to 9, X and Y 1 to 62
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
                             99, then player_compute: the maxima, carrying weight str * 20 among
                             them, as a level gained does), or a skill by name (0 to 30; enum
                             Skill, lower case: attack, defense ... swimming)

   The numbers are checked when the script is read; what depends on the game (a free object, a
   free square) when the cheat runs, which then says what failed and changes nothing. */
#include "compat.h"
#include "event.h"
#include "inv.h"
#include "items.h"
#include "map.h"
#include "motion.h"
#include "object.h"
#include "player.h"
#include "sys.h"
#include "ui.h"
#include "conv.h"
#include "gfx.h"
#include "presentation/scroll_bridge.h"
#include "i18n/utf8.h"
/* Fixture files are host paths, outside the emulated DOS filesystem. */
#undef fopen
#undef fread
#undef fseek
#undef ftell
#undef fclose

unsigned char far EncumCheck(struct Object far *obj);   /* INVDATA.C; no header declares it */

static const char *skill_names[NUM_SKILLS] = {
    "attack", "defense", "barehand", "sword", "axe", "mace", "missile", "mana", "lore",
    "casting", "traps", "search", "track", "stealth", "repair", "charisma", "picklock",
    "acrobat", "appraise", "swimming"
};

/* in the game, on its 3D view, with nothing on the cursor, no move pending and the player
   alive */
static int cheat_ready(void)
{
    return in_game && scrnum == 0 && GameInputMode == GIM_NONE && CursorObjPtr == 0 && NewPlayerX == 0
        && ThePlayer != 0 && ThePlayer->hp > 0;
}

static const char *c_goto(int argc, char *argv[], int run)
{
    long level, x, y;
    if (argc != 3 || cheat_num(argv[0], 1, NUM_LEVELS, &level) || cheat_num(argv[1], 1, 62, &x)
        || cheat_num(argv[2], 1, 62, &y))
        return "a level 1 to 9 and a square 1 to 62, 1 to 62";
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
    if (qty && item >= ITEM_KEY_100 && item < ITEM_LOCK && item != ITEM_LOCKPICK)
        obj->ol.f.owner = (uint16)qty;  /* a key: QTY is the lock it opens (auto-key's fixture) */
    else if (qty) {
        if (!OBJ_ISQUANT(obj)) {
            Obj_Free(obj);
            return "not an object with a quantity";
        }
        obj->ol.f.link = (uint16)qty;
    }
    if (quality >= 0) obj->qn.f.quality = (uint16)quality;
    if (!drop && OpenBag == 0 && EncumCheck(obj))
        for (slot = INV_PACK; slot <= INV_PACK_LAST; slot++)
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
    k = !strcmp(argv[0], "str") ? ATTR_STR : !strcmp(argv[0], "dex") ? ATTR_DEX : !strcmp(argv[0], "int") ? ATTR_INT : -1;
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

/* Presentation integration fixtures only. Still behind EXHUME_TEST_CHEATS and
   the runtime's native-only safe point; no game data or test text is embedded. */
static const char *c_scroll(int argc, char *argv[], int run)
{
    FILE *f;long size;char *text;
    if(argc!=1)return "one UTF-8 text file path";
    if(!run)return NULL;
    if(!scroll_text_enabled())return "--hd-scroll is required";
    f=fopen(argv[0],"rb");if(!f)return "cannot open UTF-8 fixture";
    if(fseek(f,0,SEEK_END) || (size=ftell(f))<0 || size>16384) {fclose(f);return "fixture must be <=16KiB";}
    rewind(f);text=malloc((size_t)size+1);
    if(!text) {fclose(f);return "allocation failed";}
    if(fread(text,1,(size_t)size,f)!=(size_t)size || memchr(text,0,(size_t)size) ||
       !utf8_validate(text,(size_t)size,NULL)) {
        free(text);fclose(f);return "bad UTF-8 fixture";
    }
    fclose(f);text[size]=0;do_main_scroll();scroll_clear(1);scroll_print(text);free(text);return NULL;
}
/* Deterministic combat fixtures. The damage goes through the real damage handler;
   misses/blocks through COMBAT.C, before fromwho is reused as an item id. */
unsigned char far damage_critter(struct Object far *, unsigned char, struct Object far *);
char far do_miss(int);
extern int16 fromwho, hitobj;
static const char *c_combat(int argc, char *argv[], int run)
{
    long slot, amount = 0;
    int kind, saved_from, saved_hit, action;
    struct Object *o;
    if (argc != 2 || cheat_num(argv[0], 2, 255, &slot)) return "a mobile slot 2..255 and damage 1..255, miss, block, attack or status";
    action = !strcmp(argv[1], "attack") ? 1 : !strcmp(argv[1], "status") ? 2 : 0;
    kind = !strcmp(argv[1], "miss") ? COMBAT_MISS : !strcmp(argv[1], "block") ? COMBAT_BLOCK : COMBAT_DAMAGE;
    if (!action && kind == COMBAT_DAMAGE && cheat_num(argv[1], 1, 255, &amount)) return "damage 1..255, miss, block, attack or status";
    if (!run) return NULL;
    o = Obj_IntTMem((int)slot);
    if (OBJ_MAJOR(o) != MAJOR_CREATURE || !o->hp) return "not a live creature";
    if (action == 1) {
        /* Ask the real AI to attack, without damaging the creature or revealing it here. */
        set_critter_vars(o);
        critter_set_goal(GOAL_ATTACK, 1);
        SET_ATTITUDE(o, ATT_HOSTILE);
        fprintf(stderr, "fixture: provoke slot=%ld hp=%u\n", slot, (unsigned)o->hp);
    } else if (action == 2) {
        CombatFrame frame;
        int i, visible = 0, engaged = 0;
        combat_text_read(&frame);
        for (i = 0; i < frame.count; i++) if (frame.actors[i].slot == slot) {
            visible = 1; engaged = frame.actors[i].engaged;
        }
        fprintf(stderr, "fixture: combat slot=%ld hp=%u visible=%d engaged=%d seq=%u target=%u\n",
                slot, (unsigned)o->hp, visible, engaged, (unsigned)OBJ_SEQ(o), (unsigned)OBJ_GTARG(o));
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
/* Aim the test camera at a mobile, optionally with a clear light level. */
void far set_light(signed char);
static const char *c_view(int argc, char *argv[], int run)
{
    long slot, light = -1;
    int dx, dy, distance;
    struct Object *o;
    if ((argc != 1 && argc != 2) || cheat_num(argv[0], 2, 255, &slot) ||
        (argc == 2 && cheat_num(argv[1], 0, 7, &light))) return "a mobile slot 2..255, optionally light 0..7";
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
    if (light >= 0) set_light((signed char)light);
    editchng(2);
    return NULL;
}
static const char *c_talk(int argc, char *argv[], int run)
{
    long who;int i;struct Object *obj;
    if(argc!=1 || cheat_num(argv[0],1,239,&who))return "an NPC whoami 1..239";
    if(!run)return NULL;
    for(i=1;i<256;i++) {
        obj=Obj_IntTMem(i);
        if(OBJ_MAJOR(obj)==MAJOR_CREATURE && obj->whoami==who) {TalkTo(obj);return NULL;}
    }
    return "NPC not found on this level";
}
/* look ITEM [QTY [QUALITY [LORE]]]: the look command's description of a new object (LookAt),
   which is then freed; a fixture for its wording (P11.2). For a book or scroll (0x130 to 0x13F)
   QTY is its text's number (P13: a book's names glossed). */
static const char *c_look(int argc, char *argv[], int run)
{
    long v[4] = {0, 0, -1, 0};
    int i;
    struct Object far *obj;
    if (argc < 1 || argc > 4) return "an item, then a quantity, a quality 0 to 63 and a lore 0 to 3 if given";
    for (i = 0; i < argc; i++)
        if (cheat_num(argv[i], 0, i == 0 ? 0x1FF : i == 1 ? 511 : i == 2 ? 63 : 3, &v[i]))
            return "an item, then a quantity, a quality 0 to 63 and a lore 0 to 3 if given";
    if (!run) return NULL;
    if ((obj = CreateObj((int)v[0], 0)) == 0) return "no free object";
    if (v[1] > 1 && (OBJ_ISQUANT(obj) || (v[0] >= 0x130 && v[0] <= 0x13F))) obj->ol.f.link = (uint16)v[1];
    if (v[2] >= 0) obj->qn.f.quality = (uint16)v[2];
    LookAt(obj, (int)v[3]);
    Obj_Free(obj);
    return NULL;
}
/* lookwall N: the look command's words for texture N, as a wall's look gives them. */
static const char *c_lookwall(int argc, char *argv[], int run)
{
    long n;
    if (argc != 1 || cheat_num(argv[0], 1, 0x3B, &n)) return "a texture 1 to 59";
    if (run) look_nothing(2, (int)n);
    return NULL;
}
void far mantra_advance(void);      /* SKILLS.C */
void far game_stats(void);          /* SKILLS.C */
void far grfx_clear(void);          /* declared where it is used (SHOWPIC.C) */
/* endstats: the end of the game's screens (SKILLS.C, after the last talisman), the winning
   picture, then the character's statistics, until a key or click; a fixture for its wording
   (P12). The game's screen is drawn again after it only as far as play redraws it. */
static const char *c_endstats(int argc, char *argv[], int run)
{
    (void)argv;
    if (argc != 0) return "no arguments";
    if (!run) return NULL;
    mouse_hide();
    grfx_clear();
    LoadBitMap_ovr141_0(7, "DATA\\win1.byt");
    grSoftPageFlip();
    LoadBitMap_ovr141_0(-1, "DATA\\win2.byt");
    game_stats();
    grSoftPageFlip();
    copy_visible_to_hidden();
    while (mouse_get_input() < 0)
        ;
    mouse_show();
    return NULL;
}
static const char *c_mantra(int argc, char *argv[], int run)
{
    (void)argv;
    if (argc != 0) return "no arguments";
    if (run) mantra_advance();
    return NULL;
}
/* use INDEX X Y: the player's use of a door or furniture, object INDEX on square X, Y of this
   level (UseRect, as a click on it does); a fixture for auto-key (P13). */
static const char *c_use(int argc, char *argv[], int run)
{
    long v[3];
    struct Object far *obj;
    if (argc != 3 || cheat_num(argv[0], 1, 1023, &v[0]) || cheat_num(argv[1], 0, 63, &v[1])
        || cheat_num(argv[2], 0, 63, &v[2]))
        return "an object 1 to 1023 and its square 0 to 63, 0 to 63";
    if (!run) return NULL;              /* the objects are there only when it runs */
    obj = Obj_IntTMem((int)v[0]);
    if (OBJ_MAJOR(obj) != 5) return "not a door or furniture (class 5)";
    MapObj_X = (int16)v[1];
    MapObj_Y = (int16)v[2];
    UseRect(ThePlayer, obj);
    return NULL;
}
/* Test fixtures only; the public menu table does not expose these commands. */
unsigned char far go_into_dying_sequence(struct Object far *);
void far backfire(struct Object far *, char);
void far EtherealVoidSpecialEffects_seg008_150(void);
extern struct Phys PN;
static const char *c_hurt(int argc, char **argv, int run)
{
    long v; int old, i, type = 0;
    if (argc != 2 || cheat_num(argv[1], 1, 255, &v)) return "physical|fire|poison|magic|void|backfire|death and 1..255";
    if (strcmp(argv[0],"physical") && strcmp(argv[0],"fire") && strcmp(argv[0],"poison") && strcmp(argv[0],"magic") &&
        strcmp(argv[0],"void") && strcmp(argv[0],"backfire") && strcmp(argv[0],"death")) return "a damage source";
    if (!run) return NULL;
    old = ThePlayer->hp;
    if (!strcmp(argv[0],"void")) for (i=0;i<v;i++) EtherealVoidSpecialEffects_seg008_150();
    else if (!strcmp(argv[0],"backfire")) backfire(ThePlayer,1);
    else if (!strcmp(argv[0],"death")) go_into_dying_sequence(ThePlayer);
    else {
        if (!strcmp(argv[0],"fire")) type = DMG_FIRE;
        else if (!strcmp(argv[0],"poison")) type = DMG_POISON;
        else if (!strcmp(argv[0],"magic")) type = DMG_MAGIC;
        damage_item(ThePlayer,0,0,0,(unsigned char)v,(unsigned char)type);
    }
    fprintf(stderr,"fixture: hurt %s hp %d -> %d\n",argv[0],old,ThePlayer->hp);
    return NULL;
}
static void blocked_destination(void)
{
    int x,y;
    for(y=5;y<=20;y++) for(x=5;x<=20;x++) Map_GetAddr(x,y)->type=TILE_SOLID;
}
static const char *c_fault(int argc,char **argv,int run)
{
    if (argc!=1 || (strcmp(argv[0],"bounds") && strcmp(argv[0],"teleport") && strcmp(argv[0],"blocked"))) return "bounds|teleport|blocked";
    if (!run) return NULL;
    if (!ENHANCED(ENH_GOD_MODE)) return "god-mode required";
    if (!strcmp(argv[0],"bounds")) { PN.x=-256;PN.z=-8; }
    else {
        NewPlayerX=10;NewPlayerY=10;NewPlayerLevel=!strcmp(argv[0],"teleport")?10:2;
        NewPlyFade=0;
        if (!strcmp(argv[0],"blocked")) npp_func=blocked_destination;
        editchng(8);
    }
    fprintf(stderr,"fixture: fault %s\n",argv[0]);return NULL;
}
static const char *c_state(int argc,char **argv,int run)
{
    (void)argv;if(argc) return "no arguments";
    if(run) fprintf(stderr,"fixture: state level=%d hp=%d x=%d y=%d z=%d god=%u map=%u\n",PlayerLevel,ThePlayer->hp,PN.x,PN.y,PN.z,ENHANCED(ENH_GOD_MODE),ENHANCED(ENH_FULL_MAP));
    return NULL;
}
extern char EndGameMode_dseg_1C8F;
static const char *c_victory(int argc,char **argv,int run)
{
    (void)argv;if(argc) return "no arguments";
    if(run) { EndGameMode_dseg_1C8F=1;editchng(0x400); }
    return NULL;
}
static const struct cheat_cmd cmds[] = {
    { "test-victory", "(ending flow fixture)", c_victory },
    { "test-hurt", "SOURCE AMOUNT", c_hurt },
    { "test-fault", "bounds|teleport|blocked", c_fault },
    { "test-state", "", c_state },
    { "look", "ITEM [QTY [QUALITY [LORE]]] (look fixture)", c_look },
    { "lookwall", "N (wall look fixture)", c_lookwall },
    { "mantra", "(shrine mantra fixture)", c_mantra },
    { "endstats", "(end of game statistics fixture)", c_endstats },
    { "goto", "LEVEL X Y", c_goto },
    { "give", "ITEM [QTY [QUALITY]] [drop]", c_give },
    { "set", "FIELD VALUE", c_set },
    { "scroll", "UTF8_FILE (HD scroll fixture)", c_scroll },
    { "combat", "SLOT DAMAGE|miss|block|attack|status (combat fixture)", c_combat },
    { "view", "SLOT [LIGHT] (camera fixture)", c_view },
    { "talk", "WHOAMI (conversation fixture)", c_talk },
    { "use", "INDEX X Y (door use fixture)", c_use },
};

const struct cheat_game cheat_game = { cmds, (int)(sizeof cmds / sizeof cmds[0]), cheat_ready };
