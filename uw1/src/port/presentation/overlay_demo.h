/* Opt-in native-resolution presentation probe; no game-thread state is read. */
#ifndef UW1_OVERLAY_DEMO_H
#define UW1_OVERLAY_DEMO_H
#include "plat.h"
int overlay_demo_draw(const PlatOverlay *frame, void *data);
#endif
