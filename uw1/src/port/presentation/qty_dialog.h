/* P12: "Move how many?" (INVPANEL.C's AskHowMany) as an HD panel with a slider and buttons, in
   zh-TW with the HD text. The game's own text entry (wdialog) still takes the answer: the panel
   covers it and, for what the player does on the panel, types into it (Backspace, the digits,
   Enter or Escape) through the keyboard's hook, so the game, the recorder and the replays see
   only keys. The game thread opens and closes it; the backend's thread (pointer, keys, drawing)
   does the rest. */
#ifndef UW1_QTY_DIALOG_H
#define UW1_QTY_DIALOG_H
#include <stdint.h>
struct HdFont;
struct PlatPointer;
struct PlatOverlay;

void qty_dialog_enable(int on, void (*key)(uint8_t scancode));   /* before threads */
/* game thread: around wdialog, with the stack's count */
void qty_dialog_begin(int max);
void qty_dialog_end(void);
/* backend thread: 1 when the panel took the event (the game does not see it) */
int qty_dialog_pointer(const struct PlatPointer *ev);
int qty_dialog_key(uint8_t scancode);
/* presentation: 1 when drawn */
int qty_dialog_draw(struct HdFont *font, const struct PlatOverlay *frame);
#endif
