/* The settings screen's cheats and the enhancements it sets in play (cheat_menu.c), and god-mode's
   hooks in the game's code, as UW1's. */
#ifndef UW2_CHEAT_MENU_H
#define UW2_CHEAT_MENU_H
void port_cheat_menu_init(void);
void port_cheat_settings(void);
void port_cheat_rescue_request(int unused);
void port_cheat_brightness_request(int value);
void port_cheat_brightness_point(void);
void port_cheat_anchor_request(int unused);
int port_god_protect(const void *object);
int port_god_hp(int old_hp, int new_hp);
void port_god_reset(void);
void port_god_world_loaded(void);
void port_god_tick(void);
int port_god_motion(void);
void port_god_bad_motion(void);
void port_god_wait_limit(void);
void port_god_teleport_begin(void);
int port_god_teleport_failed(void);
int port_god_teleport_valid(int level, int x, int y);
#endif
