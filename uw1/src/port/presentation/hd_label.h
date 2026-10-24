/* HD labels (P12, zh-TW): the translation of a string the game draws with string_to_screen,
   in the HD font, in place of its bitmap text. The game thread lays a label out and publishes
   it with a signature of the screen under it (the bitmap text is not drawn, so the box holds
   only what was there before); the presentation thread draws a label only while the scan-out
   under it still has that signature, so a label goes when the game draws over its box (a panel
   redrawn, another screen) and needs no other end. Owned, bounded state, as caption.h; no
   game, SDL or FreeType types in the state. */
#ifndef UW1_HD_LABEL_H
#define UW1_HD_LABEL_H
#include <stdint.h>
#define HD_LABELS 64
#define HD_LABEL_CELLS 40
enum { HD_LABEL_LEFT, HD_LABEL_CENTRE, HD_LABEL_RIGHT };
typedef struct HdLabelCell { uint32_t scalar; int x64; } HdLabelCell;
typedef struct HdLabel {
    int left, top, right, bottom;   /* the signed box, top-down game pixels, right/bottom exclusive */
    uint32_t signature;
    int color, em64, y64, count;    /* y64: the line's top, 26.6 */
    int fill;                       /* a patch's opaque background (palette index), else -1 */
    HdLabelCell cells[HD_LABEL_CELLS];
} HdLabel;
typedef struct HdLabelFrame { uint64_t generation; int count; HdLabel labels[HD_LABELS]; } HdLabelFrame;
typedef int (*HdLabelMeasure)(uint32_t scalar, void *context);   /* logical 26.6 advance at em64 */
typedef int (*HdLabelPixel)(int x, int y);   /* the drawing page's pixel, top-down game pixels */

void hd_label_enable(int on);               /* before threads */
int hd_label_enabled(void);
void hd_label_start(HdLabelMeasure measure, void *context, int em64, HdLabelPixel pixel);
/* How the next hd_label_put places its text against the English it replaces: from x (left),
   on the English's centre, or ending where the English ends. Reset by each put. */
void hd_label_align(int align);
/* text (strict UTF-8, one line) for the English string_to_screen would draw at (x, y), y
   bottom-up as the game gives it, english_width wide and height high (cur_font), in palette
   colour color, kept within the game's columns [clip_left, clip_right]. The text shrinks to fit
   (to 3 pixels' em). 0: published, the caller draws nothing; -1: not (the caller draws the
   English). */
int hd_label_put(const char *text, int x, int y, int english_width, int height, int color,
                 int clip_left, int clip_right);
/* A patch: text over words the game's art has in the box [left, right) x [top, bottom)
   (top-down game pixels), which it covers with the box's commonest colour (the art's
   background), aligned in the box and centred on its rows. 0: published; -1: not. */
int hd_label_patch(const char *text, int left, int top, int right, int bottom, int color, int align);
void hd_label_clear(void);
void hd_label_read(HdLabelFrame *out);
/* Presentation thread: the signature of a box of a 320-wide indexed scan-out. */
uint32_t hd_label_signature(const uint8_t *pixels, int width, int height, const HdLabel *label);
struct HdFont;
struct PlatOverlay;
/* The labels whose box in the scan-out still has their signature; 1 when anything was drawn. */
int hd_label_draw(struct HdFont *font, const struct PlatOverlay *frame, const HdLabelFrame *labels,
                  const uint8_t *pixels, int width, int height, const uint8_t pal[768], float border, float baseline);
#endif
