#include "hd_demo.h"
#include "overlay_demo.h"
#include <stdio.h>
#include <string.h>

int hd_demo_draw(const PlatOverlay *f, void *data)
{
    static const char *const samples[] = {
        "你好，冒險者。", "Ultima Underworld 中文測試", "English + 中文 123"
    };
    static const HdColor colors[] = {{255,240,190,255}, {255,255,255,255}, {150,220,255,255}};
    HdDemo *demo = data;
    int i, x, y;
    float sx = f->viewport_w / f->logical_width, sy = f->viewport_h / f->logical_height;
    int left = (int)(f->viewport_x + 20 * sx), right = (int)(f->viewport_x + 300 * sx);
    int top = (int)(f->viewport_y + 36 * sy), bottom = (int)(f->viewport_y + 112 * sy);
    if (demo->geometry) overlay_demo_draw(f, NULL);
    /* Native RGBA panel, with logical edges. Clipped even on very small outputs. */
    for (y = top < 0 ? 0 : top; y < bottom && y < f->height; y++)
        for (x = left < 0 ? 0 : left; x < right && x < f->width; x++) {
            uint8_t *p = f->rgba + (size_t)y * f->pitch + (size_t)x * 4;
            p[0] = 10; p[1] = 16; p[2] = 25; p[3] = 225;
        }
    for (i = 0; i < 3; i++) {
        int status = hd_text_draw(demo->font, f, samples[i], strlen(samples[i]),
                                  26, 42 + i * 20, 8, 268, colors[i]);
        if (status && !demo->reported) {
            fprintf(stderr, "uw1port: HD text: %s\n", hd_font_error(status));
            demo->reported = 1;
        }
    }
    return 1;
}
