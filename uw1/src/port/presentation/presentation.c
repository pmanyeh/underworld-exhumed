#include "presentation.h"
#include "hd_cursor.h"
#include "overlay_demo.h"
#include "scroll_overlay.h"
#include "caption.h"
#include "ui/settings.h"
#include <string.h>
void vga_scanout(uint8_t *, int *, int *, uint8_t[768]);
/* Presentation thread only. Snapshot and palette are latched once with scanout;
   the overlay never dereferences game structs or the FreeType game-thread state. */
static HdCursor frame_cursor;
static uint8_t frame_palette[768];
static ScrollFrame frame_text;
static CaptionFrame frame_caption;
void presentation_scanout(uint8_t *pixels, int *w, int *h, uint8_t rgb6[768])
{
    vga_scanout(pixels,w,h,rgb6);
    hd_cursor_read(&frame_cursor);
    scroll_text_read(&frame_text);
    caption_read(&frame_caption);
    memcpy(frame_palette,rgb6,sizeof frame_palette);
}
int presentation_draw(const PlatOverlay *frame, void *data)
{
    PortPresentation *p=data;
    int drawn=0;
    if (p->demo.font) drawn=hd_demo_draw(frame,&p->demo);
    else if (p->demo.geometry) drawn=overlay_demo_draw(frame,NULL);
    if (p->scroll_fonts[0])
        drawn |= scroll_overlay_draw_styled(p->scroll_fonts,p->scroll_style,frame,&frame_text,frame_palette);
    /* cutscene subtitles (zh-TW): the message font, outlined for the moving picture under them */
    if (p->scroll_fonts[0] && frame_caption.active)
        drawn |= caption_draw(p->scroll_fonts[0],frame,&frame_caption,frame_palette,
                              p->scroll_style[0].border>0.6f?p->scroll_style[0].border:0.6f);
    /* Game cursor last, above HD text. Settings owns its OS pointer and is
       composited above the game layer, so do not leave a second pointer there. */
    if (p->cursor && !settings_open()) drawn |= hd_cursor_draw(frame,&frame_cursor,frame_palette);
    return drawn;
}
