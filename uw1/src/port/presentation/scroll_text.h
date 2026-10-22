/* Owned, bounded display state. One game producer, presentation reader.
   No game, SDL or FreeType types. Coordinates use a 320x200 top-down space. */
#ifndef UW1_SCROLL_TEXT_H
#define UW1_SCROLL_TEXT_H
#include <stdint.h>
#define SCROLL_CELLS 1024
#pragma pack(push, 8)
typedef struct ScrollCell { uint32_t scalar; int x, y, color, choice; } ScrollCell;
typedef struct ScrollBox {
    int left, top, right, bottom, line_height, x, row, lines, color, pending, count;
    ScrollCell cells[SCROLL_CELLS]; /* x: logical 26.6, y: logical line-box top */
} ScrollBox;
typedef struct ScrollFrame { int screen; uint64_t generation; ScrollBox boxes[2]; } ScrollFrame;
#pragma pack(pop)
typedef int (*ScrollMeasure)(uint32_t scalar, void *context);
typedef void (*ScrollWait)(int ticks, void *context);
void scroll_text_enable(int on); /* immutable startup flag, before threads */
int scroll_text_enabled(void);
void scroll_text_start(ScrollMeasure measure, void *context, ScrollWait wait, void *wait_context);
int scroll_text_trace_open(const char *path);
void scroll_text_trace_close(void);
void scroll_text_screen(int mode);
void scroll_text_clear(int which);
void scroll_text_box(int which, int left, int top, int right, int bottom, int line_height, int color);
/* Strict UTF-8, <=16KiB. Copy before waits; never keep the caller's pointer.
   Returns -1 on invalid text/allocation/width/capacity, without legacy raster fallback. */
int scroll_text_write(int which, const char *text, int escapes, int choice);
void scroll_text_read(ScrollFrame *out);
int scroll_text_hit(int x, int y);
void scroll_text_selection(int option);
int scroll_text_x(int which);
int scroll_text_y(int which);
int scroll_text_lines(int which);
int scroll_text_color(int which);
int scroll_text_suspend(int on); /* typed ASCII editing keeps the original path */
#endif
