#ifndef UW1_CHEAT_MENU_H
#define UW1_CHEAT_MENU_H
void port_cheat_menu_init(void);
void port_cheat_settings(void);
void port_cheat_rescue_request(int unused);
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
int port_full_map_begin(int level);
void port_full_map_end(void);
int port_full_map_byte(int x, int y);
#endif
