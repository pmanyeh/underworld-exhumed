#ifndef UW1_SCROLL_OVERLAY_H
#define UW1_SCROLL_OVERLAY_H
#include "hd_font.h"
#include "scroll_text.h"
int scroll_overlay_draw(HdFont *font,const PlatOverlay *frame,const ScrollFrame *text,const uint8_t rgb6[768]);
/* A box's drawing-only style, logical pixels: a black outline's width (0 none), and how far
   the glyphs are moved down from their line (up when negative). */
typedef struct ScrollDrawStyle { float border, baseline; } ScrollDrawStyle;
/* Each box with its own font and drawing style (box 0 the message scroll, box 1 the
   conversation's), at the em the box's layout style gave it. */
int scroll_overlay_draw_styled(HdFont *const fonts[2],const ScrollDrawStyle style[2],const PlatOverlay *frame,
                               const ScrollFrame *text,const uint8_t rgb6[768]);
#endif
