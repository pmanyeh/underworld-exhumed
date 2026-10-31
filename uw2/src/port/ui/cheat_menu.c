/* The settings screen's cheats and the enhancements it sets in play, as UW1's: the screen (the
   backend's thread) only queues a command (Exhume's sys/cheat.c, cheat_menu_request); the game
   thread runs it at the main loop's safe point (port_cheat_point), where it is recorded, so a
   replay sets each at the same point. */
#include "compat.h"
#include "player.h"
#include "ui.h"
#include "sys.h"
#include "inv.h"
#include "portgame.h"
#include "i18n/phrases.h"
#include "sys/cheat.h"
#include "sys/enhance.h"
#include "ui/settings.h"
#include <stdio.h>

static void say(const char *key, const char *en)
{
    if (scroll) scroll_print(port_zh() ? (char *)port_zh_lit(key) : (char *)en);
    else fputs(en, stderr);
}

static int ready(void)
{
    return in_game && scroll && scrnum == 0 && GameInputMode == 0 && CursorObjPtr == 0 && NewPlayerX == 0
        && ThePlayer != 0 && ThePlayer->hp > 0;
}

static const char *toggle(int argc, char **argv, int run, int bit)
{
    long v;
    if (argc != 1 || cheat_num(argv[0], 0, 1, &v)) return "0 or 1";
    if (run) {
        if (v) enhance_on |= 1u << bit; else enhance_on &= ~(1u << bit);
        say("cheat.applied", "Cheat setting applied.\n");
    }
    return NULL;
}

/* The enhancements the settings screen turns on and off in play, not at the next start: each read
   where it acts, with nothing set up from it when the run starts */
static const int live[] = { ENH_INVERT_LOOK, ENH_SPLIT_ALL, ENH_UNKNOWN_TOPIC };
#define LIVE ((int)(sizeof live / sizeof live[0]))
static const char *enhance(int argc, char **argv, int run)
{
    long v;
    int i, bit = argc == 2 ? enhance_index(argv[0]) : -1;
    for (i = 0; i < LIVE && live[i] != bit; i++) {}
    if (i == LIVE || cheat_num(argv[1], 0, 1, &v)) return "an enhancement the screen sets in play, and 0 or 1";
    if (run) {
        if (v) enhance_on |= 1u << bit; else enhance_on &= ~(1u << bit);
        say("enhance.applied", "Setting applied.\n");
    }
    return NULL;
}
static const char *no_wear(int n, char **a, int run) { return toggle(n, a, run, ENH_NO_WEAR); }

static const struct cheat_cmd commands[] = {
    { "menu-no-wear", "0|1", no_wear }, { "menu-enhance", "NAME 0|1", enhance }
};
static const struct cheat_game menu = { commands, sizeof commands / sizeof commands[0], ready };
void port_cheat_menu_init(void) { cheat_menu_init(&menu); }

static void request(const char *text)
{
    if (cheat_menu_request(text)) fputs("uw2port: cheat menu queue unavailable\n", stderr);
}
static void no_wear_request(int on) { request(on ? "menu-no-wear 1" : "menu-no-wear 0"); }
static void enhance_request(int bit, int on)
{
    char command[64];
    snprintf(command, sizeof command, "menu-enhance %s %d", enhance_flag(bit)->name, on != 0);
    request(command);
}
/* the screen's apply gets only the value: one for each of live[], in its order */
static void live0(int on) { enhance_request(live[0], on); }
static void live1(int on) { enhance_request(live[1], on); }
static void live2(int on) { enhance_request(live[2], on); }
static void (*const live_request[LIVE])(int) = { live0, live1, live2 };

void port_cheat_settings(void)
{
    int i;
    settings_enhancement(ENH_NO_WEAR, SET_TAB_CHEATS, no_wear_request);
    for (i = 0; i < LIVE; i++) settings_enhancement(live[i], SET_TAB_ENHANCE, live_request[i]);
}
