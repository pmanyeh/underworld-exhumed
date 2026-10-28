/* The settings screen's Keys tab (key_help.c). */
#ifndef UW1_KEY_HELP_H
#define UW1_KEY_HELP_H
/* After settings_init: whether the pointer is locked (Ctrl+F10 lets it go) and the run records
   (F12 ends it) */
void port_key_help_init(int mouse_lock, int recording);
#endif
