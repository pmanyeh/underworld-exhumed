/* P12: the player's words typed in Chinese (the Windows input method), for the character's name
   (CHARGEN.C's pickopt) and a save's description (GAMEWRAP.C's SaveGame), in zh-TW with the HD
   text. As qty_dialog.h: the game's own entry still takes the answer. The panel takes the keys
   while it is open; on Enter it types the answer into the game's entry through the keyboard's
   hook (ASCII as typed, else the stand-in, then Enter), so the game, the recorder and the replays
   see only keys, and the Chinese is the panel's result. */
#ifndef UW1_TEXT_ENTRY_H
#define UW1_TEXT_ENTRY_H
#include <stdint.h>
struct HdFont;
struct PlatOverlay;
enum { TEXT_ENTRY_LINE, TEXT_ENTRY_NAME, TEXT_ENTRY_MAP };   /* wdialog, which a Ctrl+A Ctrl+K empties first; or the name's loop */

void text_entry_enable(int on, void (*key)(uint8_t scancode));   /* before threads */
/* game thread: the panel over [x, x+w) x [y, y+h) (top-down game pixels) with its prompt, the
   text it starts with, at most max characters, and the ASCII stand-in for an answer that is not */
void text_entry_begin(int kind, const char *prompt, const char *initial, int max, const char *standin,
                      int x, int y, int w, int h);
/* game thread, after the game's entry: the Chinese answer (valid until the next begin), or NULL */
const char *text_entry_end(void);
/* backend thread: 1 when the panel took the key */
int text_entry_key(uint8_t scancode);
/* whether the panel is open (as qty_dialog_open) */
int text_entry_open(void);
/* presentation: 1 when drawn */
int text_entry_draw(struct HdFont *font, const struct PlatOverlay *frame);
#endif
