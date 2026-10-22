#ifndef UW1_SCROLL_BRIDGE_H
#define UW1_SCROLL_BRIDGE_H
#include "scroll_text.h"
#include "i18n/localization.h"
/* Game-thread display calls. These never return translated strings to the VM. */
int port_scroll_emit(const char *prefix,const char *display,const char *suffix,int choice);
int port_scroll_choice(void);
void port_scroll_choices_reset(void);
void port_scroll_choice_bind(int option,const char *legacy);
const char *port_scroll_choice_text(int option,const char *expanded);
int port_scroll_option(int option,const char *expanded);
int port_scroll_write(int which,const char *text,int escapes);
#endif
