#include "presentation.h"
#include "hd_cursor.h"
#include "overlay_demo.h"
#include "scroll_overlay.h"
#include "caption.h"
#include "hd_label.h"
#include "ui/settings.h"
#include <string.h>
void vga_scanout(uint8_t *, int *, int *, uint8_t[768]);
/* Presentation thread only. Snapshot and palette are latched once with scanout;
   the overlay never dereferences game structs or the FreeType game-thread state. */
static HdCursor frame_cursor;
static uint8_t frame_palette[768];
static ScrollFrame frame_text;
static CaptionFrame frame_caption;
static HdLabelFrame frame_labels;
static uint8_t frame_pixels[640 * 480];   /* the scan-out, for the labels' signatures */
static int frame_w, frame_h;
void presentation_scanout(uint8_t *pixels, int *w, int *h, uint8_t rgb6[768])
{
    vga_scanout(pixels,w,h,rgb6);
    hd_label_read(&frame_labels);
    if (frame_labels.count) {
        frame_w=*w; frame_h=*h;
        memcpy(frame_pixels,pixels,(size_t)*w*(size_t)*h);
    }
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
    /* the bitmap UI's strings (P12): each while the screen under it is as the game left it */
    if (p->scroll_fonts[0] && frame_labels.count && frame_h==200)
    {
        /* the game's own cursor, drawn into video memory under the HD one, is no change to a label */
        /* (its save-under box, MOUSE.C's _actual_mhide, starts a row above; a pixel's margin) */
        int cursor[4]={frame_cursor.x-frame_cursor.hotspot_x-1,frame_cursor.y-frame_cursor.hotspot_y-2,0,0};
        cursor[2]=cursor[0]+frame_cursor.width+2; cursor[3]=cursor[1]+frame_cursor.height+3;
        drawn |= hd_label_draw(p->scroll_fonts[0],frame,&frame_labels,frame_pixels,frame_w,frame_h,frame_palette,
                               p->scroll_style[0].border,p->scroll_style[0].baseline,frame_cursor.visible?cursor:NULL);
    }
    /* Game cursor last, above HD text. Settings owns its OS pointer and is
       composited above the game layer, so do not leave a second pointer there. */
    if (p->cursor && !settings_open()) drawn |= hd_cursor_draw(frame,&frame_cursor,frame_palette);
    return drawn;
}
