#include "map_note_overlay.h"
#include "presentation/hd_font.h"
int map_note_overlay_draw(HdFont *font, const PlatOverlay *frame, const MapNoteFrame *notes,
                           const unsigned char *pixels, int w, int h, const unsigned char *pal,
                           const int cursor[4])
{
    static const float clip[4] = { 0, 0, 316, 200 };
    int i, j, drawn = 0;
    if (!font || !frame || !notes || !pixels || !pal || w != 320 || h != 200) return 0;
    for (i = 0; i < notes->count && i < MAP_NOTE_LIMIT; i++) {
        const HdLabel *n = &notes->notes[i];
        const unsigned char *rgb = pal + n->color * 3;
        HdColor ink = { (uint8_t)((rgb[0] << 2) | (rgb[0] >> 4)),
                        (uint8_t)((rgb[1] << 2) | (rgb[1] >> 4)),
                        (uint8_t)((rgb[2] << 2) | (rgb[2] >> 4)), 255 };
        if (!hd_label_shows(pixels, w, h, n, cursor)) continue;
        for (j = 0; j < n->count; j++)
            if (hd_scroll_glyph_border(font, frame, n->cells[j].scalar, n->cells[j].x64 / 64.0f,
                                       n->y64 / 64.0f, n->em64 / 64.0f, clip, ink, 0) == HD_OK) drawn = 1;
    }
    return drawn;
}
