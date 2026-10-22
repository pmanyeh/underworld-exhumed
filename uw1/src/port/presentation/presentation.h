#ifndef UW1_PRESENTATION_H
#define UW1_PRESENTATION_H
#include "hd_demo.h"
typedef struct PortPresentation { HdDemo demo; int cursor; HdFont *scroll_font; } PortPresentation;
void presentation_scanout(uint8_t *pixels, int *width, int *height, uint8_t rgb6[768]);
int presentation_draw(const PlatOverlay *frame, void *data);
#endif
