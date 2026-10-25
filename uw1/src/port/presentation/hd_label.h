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
#define HD_LABEL_CELLS 48
#define HD_LABEL_UNDER 1536    /* a box this many pixels or fewer keeps them, for hd_label_shows */
enum { HD_LABEL_LEFT, HD_LABEL_CENTRE, HD_LABEL_RIGHT };
typedef struct HdLabelCell { uint32_t scalar; int x64; } HdLabelCell;
typedef struct HdLabel {
    int left, top, right, bottom;   /* the signed box, top-down game pixels, right/bottom exclusive */
    uint32_t signature;
    int color, em64, y64, count;    /* y64: the line's top, 26.6 */
    int fill;                       /* a patch's opaque background (palette index), else -1 */
    float border;                   /* the outline, game pixels; < 0: the HD text style's */
    int page;                       /* the drawing page it was put on */
    int kept;                       /* under holds the box's pixels, row by row */
    uint8_t under[HD_LABEL_UNDER];
    HdLabelCell cells[HD_LABEL_CELLS];
} HdLabel;
typedef struct HdLabelFrame { uint64_t generation; int count; HdLabel labels[HD_LABELS]; } HdLabelFrame;
typedef int (*HdLabelMeasure)(uint32_t scalar, void *context);   /* logical 26.6 advance at em64 */
typedef int (*HdLabelPixel)(int x, int y);   /* the drawing page's pixel, top-down game pixels */
typedef int (*HdLabelPage)(void);            /* which page the game draws on (any number naming it) */

void hd_label_enable(int on);               /* before threads */
int hd_label_enabled(void);
void hd_label_start(HdLabelMeasure measure, void *context, int em64, HdLabelPixel pixel, HdLabelPage page);
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
/* Prepare one independent label (map notes may overlap and have their own lifetime).
   No change to the shared label list. Coordinates as hd_label_put; width is measured. */
int hd_label_build(HdLabel *out, const char *text, int x, int y, int height, int color,
                   int clip_right, int em64);
void hd_label_refresh(HdLabel *label);
/* A patch: text over words the game's art has in the box [left, right) x [top, bottom)
   (top-down game pixels), which it covers with the box's commonest colour (the art's
   background), aligned in the box and centred on its rows. 0: published; -1: not. */
int hd_label_patch(const char *text, int left, int top, int right, int bottom, int color, int align);
/* The general box: text at em64 (0: the base em; it shrinks to fit), over fill (a palette index,
   HD_LABEL_COMMONEST the box's commonest colour, -1 nothing), outlined border game pixels wide
   (< 0: the HD text style's). */
#define HD_LABEL_COMMONEST (-2)
int hd_label_box(const char *text, int left, int top, int right, int bottom, int color, int align,
                 int em64, int fill, float border);
void hd_label_clear(void);
/* The game draws a picture over [left, right) x [top, bottom) (top-down game pixels): the
   labels it covers on the page it draws on go, even when it draws again what was under them (a
   background reloaded); those on the other page stay (a screen built on the hidden page). */
void hd_label_forget(int left, int top, int right, int bottom);
/* The game has drawn a sprite over [left, right) x [top, bottom) (a highlight, an icon): the
   labels under it on its page stay, signed with the screen as it is now. */
void hd_label_resign(int left, int top, int right, int bottom);
/* The game has copied [left, right) x [top, bottom) from page from onto the page it draws on:
   the labels wholly inside it on that page are copied too, signed with what is there now. */
void hd_label_copy(int left, int top, int right, int bottom, int from);
void hd_label_read(HdLabelFrame *out);
/* Presentation thread: whether the scan-out under l is as it was signed, the game's cursor's
   box (left, top, right, bottom; NULL none) aside when l kept its pixels, so that the cursor
   over a label does not take it away. */
int hd_label_shows(const uint8_t *pixels, int width, int height, const HdLabel *l, const int cursor[4]);
/* Presentation thread: the signature of a box of a 320-wide indexed scan-out. */
uint32_t hd_label_signature(const uint8_t *pixels, int width, int height, const HdLabel *label);
struct HdFont;
struct PlatOverlay;
/* The labels whose box in the scan-out still has their signature; 1 when anything was drawn. */
int hd_label_draw(struct HdFont *font, const struct PlatOverlay *frame, const HdLabelFrame *labels,
                  const uint8_t *pixels, int width, int height, const uint8_t pal[768], float border, float baseline,
                  const int cursor[4]);
#endif
