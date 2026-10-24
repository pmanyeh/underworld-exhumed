/* hd_label.h's presentation-thread side: the labels whose box is as the game left it. */
#include "hd_label.h"
#include "hd_font.h"

int hd_label_draw(struct HdFont *font, const struct PlatOverlay *frame, const HdLabelFrame *labels,
                  const uint8_t *pixels, int width, int height, const uint8_t pal[768], float border, float baseline)
{
    static const float clip[4] = {0, 0, 320, 200};
    int k, i, drawn = 0;
    if (!font || !frame || !labels || !pixels || !pal) return 0;
    for (k = 0; k < labels->count && k < HD_LABELS; k++) {
        const HdLabel *l = &labels->labels[k];
        const uint8_t *rgb = pal + ((unsigned)l->color & 255) * 3;
        HdColor color;
        if (hd_label_signature(pixels, width, height, l) != l->signature) continue;
        color.r = (uint8_t)((rgb[0] << 2) | (rgb[0] >> 4));
        color.g = (uint8_t)((rgb[1] << 2) | (rgb[1] >> 4));
        color.b = (uint8_t)((rgb[2] << 2) | (rgb[2] >> 4));
        color.a = 255;
        for (i = 0; i < l->count && i < HD_LABEL_CELLS; i++)
            if (hd_scroll_glyph_border(font, frame, l->cells[i].scalar, l->cells[i].x64 / 64.0f,
                                       l->y64 / 64.0f + baseline, l->em64 / 64.0f, clip, color, border) == HD_OK)
                drawn = 1;
    }
    return drawn;
}
