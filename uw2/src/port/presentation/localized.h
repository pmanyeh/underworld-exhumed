/* UW2's localized text (Phase 14): the language options, the catalog and the HD message
   scroll, wired from the runtime's shared modules (i18n/, presentation/, as UW1 uses them). */
#ifndef UW2_LOCALIZED_H
#define UW2_LOCALIZED_H
#include "plat.h"

/* argv[*i], one of --locale en|zh-TW, --i18n-file FILE, --hd-scroll, --font FILE: 1 taken
   (*i past its value), 0 not one of these, -1 a bad value. */
int port_text_option(int argc, char **argv, int *i);
/* Before plat_run, with the home directory: the language (a player's run with no --locale: the
   settings file's language=, else zh-TW when a catalog is there), the catalog and the HD fonts.
   0, or -1 (reported) when the options cannot be met. */
int port_text_setup(const char *home, int interactive);
/* The scan-out and the overlay that draws the HD scroll, when it is on. */
void port_text_hooks(PlatHooks *hooks);
/* On the game's thread, around the game: load the catalog and the measuring fonts; free them. */
void port_text_begin(void);
void port_text_end(void);
/* The game's sources: whether translated sentences are composed for display (the HD scroll
   with a catalog). */
int port_zh(void);

#endif
