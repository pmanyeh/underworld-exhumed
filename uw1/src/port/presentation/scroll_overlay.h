#ifndef UW1_SCROLL_OVERLAY_H
#define UW1_SCROLL_OVERLAY_H
#include "hd_font.h"
#include "scroll_text.h"
int scroll_overlay_draw(HdFont *font,const PlatOverlay *frame,const ScrollFrame *text,const uint8_t rgb6[768]);
#endif
