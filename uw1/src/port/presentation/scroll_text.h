/* Owned, bounded display state. One game producer, presentation reader.
   No game, SDL or FreeType types. Coordinates use a 320x200 top-down space. */
#ifndef UW1_SCROLL_TEXT_H
#define UW1_SCROLL_TEXT_H
#include <stdint.h>
#define SCROLL_CELLS 1024
#pragma pack(push, 8)
typedef struct ScrollCell { uint32_t scalar; int x, y, color, choice, y64; } ScrollCell;
typedef struct ScrollBox {
    int left, top, right, bottom, line_height, x, row, lines, color, pending, count;
    int line64, em64;               /* the line pitch with the style's spacing, the em: 26.6 */
    uint32_t last;                  /* the last glyph on the current line, 0 at its start */
    ScrollCell cells[SCROLL_CELLS]; /* x, y64: logical 26.6; y: logical line-box top, whole */
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
/* A box's text style, logical 26.6: the em, the space after each glyph, more where Latin
   meets Chinese, and more between lines (added to the game's line height). The defaults
   (scroll_text_start) are a 5 em and no spacing. Before the box's first text. */
void scroll_text_style(int which, int em64, int letter64, int latin64, int line64);
/* The [MORE] prompt's words (UTF-8, at most 31 bytes; the default "[MORE]"), before threads (P12). */
void scroll_text_more_label(const char *utf8);
/* The measure's context for one box (scroll_text_start gives both boxes its own). */
void scroll_text_measure_context(int which, void *context);
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
uint32_t scroll_text_last(int which); /* the last glyph on the current line, 0 at its start */
int scroll_text_suspend(int on); /* typed ASCII editing keeps the original path */
/* A box as it stands, and back to it: a line being typed is drawn again after each key. */
void scroll_text_mark(int which, ScrollBox *out);
void scroll_text_rewind(int which, const ScrollBox *mark);
#endif
