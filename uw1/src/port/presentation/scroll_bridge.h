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
/* P11.4: a conversation's typed answer (babl_ask) on the HD message scroll, with the host's
   input method on: ASCII keys come from next_key (the game's own input), other text from the
   input method; Enter or Esc ends it. result gets ASCII only (input_alias_map for the
   conversation slot; typed ASCII unchanged), at most maxlen bytes. Returns the key that ended
   it, or -2 when the HD scroll or the alias table is off, for the original prompt to run.
   prompt: shown before the answer (0: ">"). slot < 0: an answer typed in ASCII only (a
   mantra), with no input method and no aliases; other text gives an empty result. */
int port_scroll_ask(int slot,const char *prompt,char *result,int maxlen,int (*next_key)(void));
#endif
