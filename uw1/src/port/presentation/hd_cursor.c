#include "hd_cursor.h"
#include "plat.h"
#include <stdatomic.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

static int enabled;
static atomic_flag busy = ATOMIC_FLAG_INIT;
static HdCursor published;
static FILE *trace;
int hd_cursor_trace_open(const char *path) { trace=path?fopen(path,"w"):NULL; return path&&!trace?-1:0; }
void hd_cursor_trace_close(void) { if(trace)fclose(trace);trace=NULL; }
static void log_state(void) /* producer only; the other thread is a reader */
{
    if(!trace)return;
    fprintf(trace,"{\"visible\":%d,\"picture\":%d,\"x\":%d,\"y\":%d,\"hx\":%d,\"hy\":%d,\"w\":%d,\"h\":%d,\"mode\":%d,\"path\":%d,\"generation\":%llu}\n",
        published.visible,published.picture,published.x,published.y,published.hotspot_x,published.hotspot_y,
        published.width,published.height,published.mode,published.path,(unsigned long long)published.generation);
    fflush(trace);
}
static void lock(void) { while (atomic_flag_test_and_set_explicit(&busy, memory_order_acquire)) {} }
static void unlock(void) { atomic_flag_clear_explicit(&busy, memory_order_release); }
void hd_cursor_enable(int on) { enabled = on != 0; } /* before threads only */
int hd_cursor_enabled(void) { return enabled; }
void hd_cursor_hide(void)
{
    lock(); published.visible = published.tracked = 0; published.generation++; unlock();
    if (enabled) log_state();
}
/* Track legacy geometry too: the map's quill is wider than the usual pointer. */
void hd_cursor_track_box(int x, int y, int hx, int hy, int w, int h)
{
    lock();
    published.tracked = w > 0 && h > 0;
    published.box[0] = x - hx - 1; published.box[1] = y - hy - 2;
    published.box[2] = x - hx + w + 1; published.box[3] = y - hy + h + 1;
    unlock();
}
void hd_cursor_publish(int pic, int x, int y, int hx, int hy, int w, int h, int mode, int path, const uint8_t *pixels)
{
    if (!enabled) return;
    if (!pixels || w <= 0 || h <= 0 || w > HD_CURSOR_MAX || h > HD_CURSOR_MAX) { hd_cursor_hide(); return; }
    lock();
    published.visible = 1; published.picture = pic; published.x = x; published.y = y;
    published.hotspot_x = hx; published.hotspot_y = hy;
    published.width = w; published.height = h; published.mode = mode; published.path = path;
    memcpy(published.pixels, pixels, (size_t)w * h);
    published.generation++;
    unlock();
    log_state();
}
void hd_cursor_read(HdCursor *out)
{
    if (!out) return;
    lock(); *out = published; unlock();
}
int hd_cursor_draw(const PlatOverlay *f, const HdCursor *c, const uint8_t pal[768])
{
    double sx, sy, left, top, right, bottom;
    int x0, y0, x1, y1, x, y;
    if (!f || !c || !pal || !c->visible || c->width <= 0 || c->height <= 0 ||
        c->width > HD_CURSOR_MAX || c->height > HD_CURSOR_MAX ||
        !f->rgba || f->width <= 0 || f->width > INT_MAX/4 || f->height <= 0 ||
        f->pitch < f->width*4 || (size_t)f->height > SIZE_MAX/(size_t)f->pitch ||
        f->logical_width <= 0 || f->logical_height <= 0 ||
        !isfinite(f->viewport_x) || !isfinite(f->viewport_y) ||
        !isfinite(f->viewport_w) || !isfinite(f->viewport_h) ||
        f->viewport_w <= 0 || f->viewport_h <= 0) return 0;
    sx = (double)f->viewport_w/f->logical_width; sy = (double)f->viewport_h/f->logical_height;
    /* Convert to double before subtraction: snapshots may be caller-supplied. */
    left = f->viewport_x + ((double)c->x-c->hotspot_x)*sx;
    top = f->viewport_y + ((double)c->y-c->hotspot_y)*sy;
    right = left + c->width*sx; bottom = top + c->height*sy;
    if (right <= 0 || bottom <= 0 || left >= f->width || top >= f->height) return 0;
    x0 = left < 0 ? 0 : (int)left; y0 = top < 0 ? 0 : (int)top;
    x1 = right >= f->width ? f->width : (int)right+1;
    y1 = bottom >= f->height ? f->height : (int)bottom+1;
    for (y=y0;y<y1;y++) for (x=x0;x<x1;x++) {
        double lx=((double)x+.5-f->viewport_x)/sx;
        double ly=((double)y+.5-f->viewport_y)/sy;
        double u=lx-((double)c->x-c->hotspot_x), v=ly-((double)c->y-c->hotspot_y);
        uint8_t index, *d;
        const uint8_t *rgb;
        /* Clip at game bounds too; letterbox is not part of the game. Pixel-centre
           nearest sampling matches the framebuffer, including fractional edges. */
        if (lx<0 || ly<0 || lx>=f->logical_width || ly>=f->logical_height ||
            u<0 || v<0 || u>=c->width || v>=c->height) continue;
        index=c->pixels[(size_t)(int)v*c->width+(int)u];
        if (!index) continue;
        d=f->rgba+(size_t)y*f->pitch+(size_t)x*4; rgb=pal+index*3;
        d[0]=(uint8_t)((rgb[0]<<2)|(rgb[0]>>4));
        d[1]=(uint8_t)((rgb[1]<<2)|(rgb[1]>>4));
        d[2]=(uint8_t)((rgb[2]<<2)|(rgb[2]>>4)); d[3]=255;
    }
    return 1;
}
