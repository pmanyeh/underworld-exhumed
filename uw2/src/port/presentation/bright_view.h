#ifndef UW2_BRIGHT_VIEW_H
#define UW2_BRIGHT_VIEW_H
#define BRIGHT_STRENGTH_MAX 15
#define BRIGHT_STRENGTH_DEFAULT 4
/* Game-thread state; the settings callback requests a recorded command. */
void port_bright_strength_set(int strength);
int port_bright_strength(void);
int port_bright_map_shade(const void *cell, int fallback);
/* Game-thread scope around building and rasterizing a normal 3D frame, never a pick frame. */
void port_bright_begin(void);
void port_bright_end(void);
#endif
