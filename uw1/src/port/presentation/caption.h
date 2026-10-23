/* Cutscene subtitles in the HD font (zh-TW): the game thread lays a subtitle out in the
   cutscene's text area, the presentation thread draws the published snapshot over the scan-out.
   Owned, bounded state, as scroll_text; no game, SDL or FreeType types in the state. */
#ifndef UW1_CAPTION_H
#define UW1_CAPTION_H
#include <stdint.h>
#define CAPTION_CELLS 512
#define CAPTION_LINES 6
typedef struct CaptionCell { uint32_t scalar; int x64, y64; } CaptionCell;
typedef struct CaptionFrame { uint64_t generation; int active, color, em64, count; CaptionCell cells[CAPTION_CELLS]; } CaptionFrame;
typedef int (*CaptionMeasure)(uint32_t scalar, void *context);  /* logical 26.6 advance at em64 */

void caption_enable(int on);               /* before threads */
int caption_enabled(void);
void caption_start(CaptionMeasure measure, void *context, int em64);
/* text (strict UTF-8, \n breaks) centred in the cutscene's text area as CUTS.C centres its
   subtitle (x, y bottom-up as the game gives it, w, h), bottom-aligned, wrapped to the width,
   at most CAPTION_LINES lines; colour is a palette index. -1 on invalid text. */
int caption_set(const char *text, int color, int x, int y, int w, int h);
void caption_clear(void);
int caption_active(void);
void caption_read(CaptionFrame *out);
/* Presentation thread: the snapshot's glyphs with an outline; 1 when anything was drawn. */
struct HdFont;
struct PlatOverlay;
int caption_draw(struct HdFont *font, const struct PlatOverlay *frame, const CaptionFrame *caption, const uint8_t pal[768], float border);
#endif
