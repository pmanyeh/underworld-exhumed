/* Phase 4 probe: an alpha patch, a logical reference box, and a one-output-pixel
   crosshair. Geometry is derived solely from the presentation callback. */
#include "overlay_demo.h"

static void pixel(const PlatOverlay *f, int x, int y, int r, int g, int b, int a)
{
    uint8_t *p;
    if (x < 0 || y < 0 || x >= f->width || y >= f->height) return;
    p = f->rgba + (size_t)y * (size_t)f->pitch + (size_t)x * 4;
    p[0] = (uint8_t)r; p[1] = (uint8_t)g; p[2] = (uint8_t)b; p[3] = (uint8_t)a;
}

int overlay_demo_draw(const PlatOverlay *f, void *data)
{
    int x, y;
    int left = (int)(f->viewport_x + f->viewport_w / 8);
    int top = (int)(f->viewport_y + f->viewport_h / 8);
    int right = (int)(f->viewport_x + f->viewport_w * 7 / 8);
    int bottom = (int)(f->viewport_y + f->viewport_h * 7 / 8);
    int cx = (int)(f->viewport_x + f->viewport_w / 2);
    int cy = (int)(f->viewport_y + f->viewport_h / 2);
    (void)data;
    for (y = top; y < top + (bottom - top) / 4; y++)
        for (x = left; x < left + (right - left) / 3; x++)
            pixel(f, x, y, 0, 160, 255, 128);
    for (x = left; x <= right; x++) {
        pixel(f, x, top, 255, 220, 0, 255);
        pixel(f, x, bottom, 255, 220, 0, 255);
    }
    for (y = top; y <= bottom; y++) {
        pixel(f, left, y, 255, 220, 0, 255);
        pixel(f, right, y, 255, 220, 0, 255);
    }
    for (x = cx - 12; x <= cx + 12; x++) pixel(f, x, cy, 255, 255, 255, 255);
    for (y = cy - 12; y <= cy + 12; y++) pixel(f, cx, y, 255, 255, 255, 255);
    return 1;
}
