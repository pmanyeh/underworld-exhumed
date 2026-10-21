#ifndef UW1_HD_DEMO_H
#define UW1_HD_DEMO_H
#include "hd_font.h"
typedef struct HdDemo {
    HdFont *font;
    int geometry, reported; /* presentation thread only after startup */
} HdDemo;
int hd_demo_draw(const PlatOverlay *frame, void *data);
#endif
