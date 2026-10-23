#ifndef UW1_PRESENTATION_H
#define UW1_PRESENTATION_H
#include "hd_demo.h"
#include "scroll_overlay.h"
typedef struct PortPresentation {
    HdDemo demo;
    int cursor;
    HdFont *scroll_fonts[2];            /* the message scroll's and the conversation's (may be one) */
    ScrollDrawStyle scroll_style[2];    /* their outlines and baselines */
} PortPresentation;
void presentation_scanout(uint8_t *pixels, int *width, int *height, uint8_t rgb6[768]);
int presentation_draw(const PlatOverlay *frame, void *data);
#endif
