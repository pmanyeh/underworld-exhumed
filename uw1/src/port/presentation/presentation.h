#ifndef UW1_PRESENTATION_H
#define UW1_PRESENTATION_H
#include "hd_demo.h"
#include "presentation/scroll_overlay.h"
typedef struct PortPresentation {
    HdDemo demo;
    int cursor;
    HdFont *scroll_fonts[2];            /* the message scroll's and the conversation's (may be one) */
    ScrollDrawStyle scroll_style[2];    /* their outlines and baselines */
    HdFont *label_font;                 /* the labels' without the HD scroll (English: the player's Chinese, P12) */
} PortPresentation;
void presentation_scanout(uint8_t *pixels, int *width, int *height, uint8_t rgb6[768]);
int presentation_draw(const PlatOverlay *frame, void *data);
/* the backend's pointer events, for the cursor drawn over a panel and the labels (backend thread) */
void presentation_pointer(const PlatPointer *ev);
#endif
