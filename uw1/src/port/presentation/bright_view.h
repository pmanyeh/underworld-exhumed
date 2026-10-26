#ifndef UW1_BRIGHT_VIEW_H
#define UW1_BRIGHT_VIEW_H
/* Game-thread scope around building and rasterizing a normal 3D frame, never a pick frame. */
void port_bright_begin(void);
void port_bright_end(void);
#endif
