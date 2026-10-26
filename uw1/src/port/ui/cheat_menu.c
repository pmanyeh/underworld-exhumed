/* Public cheats run only on the game thread, through the recorded menu queue. */
#include "compat.h"
#include "player.h"
#include "motion.h"
#include "ui.h"
#include "sys.h"
#include "event.h"
#include "inv.h"
#include "i18n/phrases.h"
#include "ui/settings.h"
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>
void far player_setup(int x, int y);
char far ChangeLevel(int from, int to);

static struct { int x, y, valid; } anchor[10], recent[10];
static int bad_motion, recovering, blocked, world_loaded, rollback_level, rollback_x, rollback_y;
/* A dragged slider can change hundreds of times while the game is paused. */
static atomic_int pending_strength = -1;
static int valid_level(int l) { return l >= 1 && l <= 9; }
static int valid_tile(int x, int y) { return x >= 1 && x <= 62 && y >= 1 && y <= 62; }
static void say(const char *key, const char *en)
{
    if (scroll) scroll_print(port_zh() ? (char *)port_zh_lit(key) : (char *)en);
    else fputs(en, stderr);
}
int port_god_protect(const void *o) { return ENHANCED(ENH_GOD_MODE) && o && o == ThePlayer; }
int port_god_hp(int old_hp, int new_hp) { return ENHANCED(ENH_GOD_MODE) && old_hp > 0 && new_hp < old_hp ? old_hp : new_hp; }
void port_god_reset(void)
{
    memset(anchor, 0, sizeof anchor); memset(recent, 0, sizeof recent);
    bad_motion = recovering = blocked = world_loaded = rollback_level = 0;
}
void port_god_world_loaded(void) { world_loaded = 1; blocked = bad_motion = 0; }
static int clear_floor(int x, int y)
{
    struct Tile *t;
    int z;
    if (!mapdata || !ThePlayer || !valid_tile(x, y)) return 0;
    t = mapdata + x + y * 64;
    /* Keep anchors on ordinary ground, not slopes, liquid, moving platforms or objects. */
    if (t->type != TILE_OPEN || t->floor >= TXM_FLOORS || (TxmTerr[t->floor] & 0x30)) return 0;
    z = t->height << 3;
    return can_place(OBJ_ITEM(ThePlayer), 1, x * 8 + 3, y * 8 + 3, z, 0, 0)
        && nvokTerr == FOOT_FLOOR && nvokHgt == z;
}
static void stop_motion(void)
{
    PN.speed = PN.time = PN.impact = 0;
    memset(PN.vel, 0, sizeof PN.vel); memset(PN.acc, 0, sizeof PN.acc);
    PlayerInput = PIN_NONE; NewPlayerX = 0; NewPlyFade = 0; npp_func = NULL;
}
static int place(int x, int y)
{
    if (!clear_floor(x, y)) return 0;
    stop_motion();
    if (GrSq < 0 || GrSq >= MAP_TILES) GrSq = -1;
    else GrSq = OBJ_HOMEX(ThePlayer) + OBJ_HOMEY(ThePlayer) * 64;
    player_setup(x, y);
    PN.impact = 0; bad_motion = blocked = 0;
    editchng(0x7FFE);
    return 1;
}
static int recover(int manual)
{
    int x, y, l = PlayerLevel;
    if (recovering || !valid_level(l) || !ThePlayer || !mapdata) return 0;
    recovering = 1;
    if ((manual && anchor[l].valid && place(anchor[l].x, anchor[l].y)) ||
        (recent[l].valid && place(recent[l].x, recent[l].y)) ||
        (anchor[l].valid && place(anchor[l].x, anchor[l].y))) goto ok;
    /* A missing or occupied anchor: bounded scan of this loaded map, never a random teleport. */
    for (y = 1; y <= 62; y++) for (x = 1; x <= 62; x++) if (place(x, y)) goto ok;
    stop_motion();
    /* No legal destination exists: keep invalid physics out of the map access paths. */
    PN.x = (OBJ_HOMEX(ThePlayer) * 256) + 128;
    PN.y = (OBJ_HOMEY(ThePlayer) * 256) + 128;
    PN.z = OBJ_Z(ThePlayer) * 8;
    blocked = 1; recovering = 0;
    say("cheat.no_safe", "No safe floor is available. Restore a saved game.\n");
    return 0;
ok:
    recovering = 0;
    say("cheat.rescued", "Returned to a safe position.\n");
    return 1;
}
void port_god_bad_motion(void) { bad_motion = 1; }
int port_god_motion(void)
{
    if (!ENHANCED(ENH_GOD_MODE) || !world_loaded || !in_game || recovering) return 1;
    if (blocked) return 0;
    if (!valid_level(PlayerLevel) || !mapdata || !ThePlayer) return 0;
    if (bad_motion || PN.x < 256 || PN.x >= 63 * 256 || PN.y < 256 || PN.y >= 63 * 256 ||
        PN.z < 0 || PN.z > 0x3F8 || GrSq < -1 || GrSq >= MAP_TILES ||
        (GrSq >= 0 && GrSq != OBJ_HOMEX(ThePlayer) + OBJ_HOMEY(ThePlayer) * 64)) { recover(0); return 0; }
    return 1;
}
void port_god_wait_limit(void)
{
    if (ENHANCED(ENH_GOD_MODE)) recover(0);
}
void port_god_tick(void)
{
    int x, y, l = PlayerLevel;
    /* Restore leaves GrSq=-1 until phys_affect_player links the saved position.
       The first view is not initialized yet; never rescue or print in that interval. */
    if (!ENHANCED(ENH_GOD_MODE) || !world_loaded || !in_game || !scroll || GrSq < 0 || scrnum != 0 || NewPlayerX || !ThePlayer || recovering) return;
    if (!port_god_motion()) return;
    if (PN.vel[2] || PN.acc[2] || !(PN.terrain & FOOT_FLOOR)) return;
    x = PN.x >> 8; y = PN.y >> 8;
    if (recent[l].valid && recent[l].x == x && recent[l].y == y) return;
    if (clear_floor(x, y)) {
        recent[l].x = x; recent[l].y = y; recent[l].valid = 1;
        if (!anchor[l].valid) anchor[l] = recent[l];
    }
}
void port_god_teleport_begin(void)
{
    if (!ENHANCED(ENH_GOD_MODE) || !valid_level(PlayerLevel) || !ThePlayer) return;
    rollback_level = PlayerLevel; rollback_x = OBJ_HOMEX(ThePlayer); rollback_y = OBJ_HOMEY(ThePlayer);
}
int port_god_teleport_valid(int l, int x, int y)
{
    return !ENHANCED(ENH_GOD_MODE) || (valid_level(l) && valid_tile(x, y));
}
int port_god_teleport_failed(void)
{
    int l = rollback_level;
    if (!ENHANCED(ENH_GOD_MODE)) return 0;
    stop_motion(); blocked = 0;
    if (valid_level(l) && PlayerLevel != l) {
        if (!ChangeLevel(PlayerLevel, l)) { blocked = 1; say("cheat.no_safe", "Cannot restore the previous level. Restore a saved game.\n"); return 1; }
        PlayerLevel = l;
    }
    if (!(valid_level(l) && PlayerLevel == l && place(rollback_x, rollback_y))) recover(0);
    else say("cheat.rescued", "Returned to a safe position.\n");
    return 1;
}
static int ready(void)
{
    return world_loaded && mapdata && in_game && scroll && GrSq >= 0 && scrnum == 0 && GameInputMode == GIM_NONE && !CursorObjPtr && !NewPlayerX && ThePlayer && ThePlayer->hp;
}
static const char *toggle(int argc, char **argv, int run, int bit)
{
    long v;
    if (argc != 1 || cheat_num(argv[0], 0, 1, &v)) return "0 or 1";
    if (run) {
        if (bit == ENH_GOD_MODE && !v && blocked && !recover(1)) return "no safe position yet";
        if (v) enhance_on |= 1u << bit; else enhance_on &= ~(1u << bit);
        if (bit == ENH_GOD_MODE) { blocked = 0; port_god_tick(); }
        say("cheat.applied", "Cheat setting applied.\n");
    }
    return NULL;
}
static const char *god(int n, char **a, int run) { return toggle(n, a, run, ENH_GOD_MODE); }
static const char *map(int n, char **a, int run) { return toggle(n, a, run, ENH_FULL_MAP); }
static const char *bright(int n, char **a, int run) { return toggle(n, a, run, ENH_BRIGHT_VIEW); }
static const char *brightness(int n, char **a, int run)
{
    long value;
    if (n != 1 || cheat_num(a[0], 0, BRIGHT_STRENGTH_MAX, &value)) return "0..15";
    if (run) port_bright_strength_set((int)value);
    return NULL;
}
static const char *rescue(int n, char **a, int run)
{
    (void)a;
    if (n) return "no arguments";
    if (run) { blocked = 0; recover(1); }
    return NULL;
}
static const char *remember(int n, char **a, int run)
{
    int x, y; (void)a;
    if (n) return "no arguments";
    if (!run) return NULL;
    x = PN.x >> 8; y = PN.y >> 8;
    if (valid_level(PlayerLevel) && !PN.vel[2] && !PN.acc[2] && clear_floor(x, y)) {
        anchor[PlayerLevel].x = x; anchor[PlayerLevel].y = y; anchor[PlayerLevel].valid = 1;
        say("cheat.remembered", "Safe position remembered for this level.\n");
    } else say("cheat.not_safe", "Stand on a clear, dry floor first.\n");
    return NULL;
}
static const struct cheat_cmd commands[] = {
    { "menu-god", "0|1", god }, { "menu-map", "0|1", map },
    { "menu-rescue", "", rescue }, { "menu-anchor", "", remember },
    { "menu-bright", "0|1", bright }, { "menu-bright-level", "0..15", brightness }
};
static const struct cheat_game menu = { commands, sizeof commands / sizeof commands[0], ready };
void port_cheat_menu_init(void) { cheat_menu_init(&menu); }
static void request(const char *text)
{
    if (cheat_menu_request(text)) fputs("uw1port: cheat menu queue unavailable\n", stderr);
}
static void god_request(int on) { request(on ? "menu-god 1" : "menu-god 0"); }
static void map_request(int on) { request(on ? "menu-map 1" : "menu-map 0"); }
static void bright_request(int on) { request(on ? "menu-bright 1" : "menu-bright 0"); }
void port_cheat_brightness_request(int value)
{
    if (value >= 0 && value <= BRIGHT_STRENGTH_MAX) atomic_store(&pending_strength, value);
}
void port_cheat_brightness_point(void)
{
    int value, empty = -1;
    char command[32];
    if (!ready()) return;
    value = atomic_exchange(&pending_strength, -1);
    if (value < 0) return;
    snprintf(command, sizeof command, "menu-bright-level %d", value);
    if (cheat_menu_request(command)) atomic_compare_exchange_strong(&pending_strength, &empty, value);
}
void port_cheat_rescue_request(int unused) { (void)unused; request("menu-rescue"); }
void port_cheat_anchor_request(int unused) { (void)unused; request("menu-anchor"); }
void port_cheat_settings(void)
{
    settings_enhancement(ENH_FULL_MAP, SET_TAB_CHEATS, map_request);
    settings_enhancement(ENH_GOD_MODE, SET_TAB_CHEATS, god_request);
    settings_enhancement(ENH_BRIGHT_VIEW, SET_TAB_CHEATS, bright_request);
}
