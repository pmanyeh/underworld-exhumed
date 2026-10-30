/* caption.h's presentation-thread side: the snapshot drawn in the HD font. */
#include "caption.h"
#include "presentation/hd_font.h"

int caption_draw(struct HdFont *font, const struct PlatOverlay *frame, const CaptionFrame *c, const uint8_t pal[768], float border)
{
    static const float clip[4] = {0, 0, 320, 200};
    const uint8_t *rgb;
    HdColor color;
    int i, drawn = 0;
    if (!font || !frame || !c || !pal || !c->active) return 0;
    rgb = pal + ((unsigned)c->color & 255) * 3;
    color.r = (uint8_t)((rgb[0] << 2) | (rgb[0] >> 4));
    color.g = (uint8_t)((rgb[1] << 2) | (rgb[1] >> 4));
    color.b = (uint8_t)((rgb[2] << 2) | (rgb[2] >> 4));
    color.a = 255;
    for (i = 0; i < c->count && i < CAPTION_CELLS; i++)
        if (hd_scroll_glyph_border(font, frame, c->cells[i].scalar, c->cells[i].x64 / 64.0f, c->cells[i].y64 / 64.0f,
                                   c->em64 / 64.0f, clip, color, border) == HD_OK) drawn = 1;
    return drawn;
}
