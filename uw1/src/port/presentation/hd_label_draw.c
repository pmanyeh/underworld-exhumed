/* hd_label.h's presentation-thread side: the labels whose box is as the game left it. */
#include "hd_label.h"
#include "presentation/hd_font.h"

static uint8_t level(uint8_t v6) { return (uint8_t)((v6 << 2) | (v6 >> 4)); }

/* a patch's box in its fill colour, opaque, in output pixels */
static void fill(const struct PlatOverlay *frame, const HdLabel *l, const uint8_t pal[768])
{
    const uint8_t *rgb = pal + ((unsigned)l->fill & 255) * 3;
    float sx = frame->viewport_w / frame->logical_width, sy = frame->viewport_h / frame->logical_height;
    int x0 = (int)(frame->viewport_x + l->left * sx), x1 = (int)(frame->viewport_x + l->right * sx + 0.999f);
    int y0 = (int)(frame->viewport_y + l->top * sy), y1 = (int)(frame->viewport_y + l->bottom * sy + 0.999f), x, y;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > frame->width) x1 = frame->width;
    if (y1 > frame->height) y1 = frame->height;
    for (y = y0; y < y1; y++)
        for (x = x0; x < x1; x++) {
            uint8_t *p = frame->rgba + (size_t)y * (size_t)frame->pitch + (size_t)x * 4;
            p[0] = level(rgb[0]); p[1] = level(rgb[1]); p[2] = level(rgb[2]); p[3] = 255;
        }
}

int hd_label_draw(struct HdFont *font, const struct PlatOverlay *frame, const HdLabelFrame *labels,
                  const uint8_t *pixels, int width, int height, const uint8_t pal[768], float border, float baseline,
                  const int cursor[4])
{
    static const float clip[4] = {0, 0, 320, 200};
    int k, i, drawn = 0;
    if (!font || !frame || !labels || !pixels || !pal) return 0;
    for (k = 0; k < labels->count && k < HD_LABELS; k++) {
        const HdLabel *l = &labels->labels[k];
        const uint8_t *rgb = pal + ((unsigned)l->color & 255) * 3;
        HdColor color;
        if (!hd_label_shows(pixels, width, height, l, cursor)) continue;
        if (l->fill >= 0) { fill(frame, l, pal); drawn = 1; }
        color.r = level(rgb[0]);
        color.g = level(rgb[1]);
        color.b = level(rgb[2]);
        color.a = 255;
        for (i = 0; i < l->count && i < HD_LABEL_CELLS; i++)
            if (hd_scroll_glyph_border(font, frame, l->cells[i].scalar, l->cells[i].x64 / 64.0f,
                                       l->y64 / 64.0f + baseline, l->em64 / 64.0f, clip, color, l->border >= 0 ? l->border : border) == HD_OK)
                drawn = 1;
    }
    return drawn;
}
