/* flask_text.h's presentation-thread side: the values on the flasks and the floating changes. */
#include "flask_text.h"
#include "hd_font.h"
#include <stdio.h>

/* the flasks (PANELS.C's FL_X; 24 wide) and where on them, top-down game pixels */
static const int flask_left[2] = { 0xF8, 0x11C };
#define FLASK_W 24
#define VALUE_TOP 146                   /* the flask's round bottom */
#define FLOAT_TOP 128                   /* a change starts at the flask's neck and rises */
#define FLOAT_RISE 18
#define STALE_FRAMES 20                 /* the game gave nothing for this many frames: another screen */

static int text_width(HdFont *font, const char *s, int em64)
{
    int w = 0;
    for (; *s; s++) w += hd_scroll_advance(font, (unsigned char)*s, em64);
    return w;
}

static int draw_text(HdFont *font, const PlatOverlay *frame, const char *s, int centre, float top, int em64,
                     HdColor color, float border)
{
    static const float clip[4] = { 0, 0, 320, 200 };
    float x = centre - text_width(font, s, em64) / 128.0f;
    int drawn = 0;
    for (; *s; s++) {
        if (hd_scroll_glyph_border(font, frame, (unsigned char)*s, x, top, em64 / 64.0f, clip, color, border) == HD_OK)
            drawn = 1;
        x += hd_scroll_advance(font, (unsigned char)*s, em64) / 64.0f;
    }
    return drawn;
}

int flask_text_draw(HdFont *font, const PlatOverlay *frame, const FlaskFrame *f, float border)
{
    static uint64_t seen;
    static int still;
    char s[24];
    int i, drawn = 0;
    if (!font || !frame || !f || !f->active) return 0;
    if (f->generation != seen) { seen = f->generation; still = 0; }
    else if (++still > STALE_FRAMES) return 0;
    for (i = 0; i < 2; i++) {
        HdColor c = { 255, 255, 255, 255 };
        if (i == 0 && f->poisoned) { c.r = 150; c.g = 255; c.b = 120; }
        snprintf(s, sizeof s, "%d/%d", f->value[i], f->max[i]);
        drawn |= draw_text(font, frame, s, flask_left[i] + FLASK_W / 2, VALUE_TOP, 4 * 64, c, border);
    }
    for (i = 0; i < f->count && i < FLASK_FLOATS; i++) {
        const FlaskFloat *fl = &f->floats[i];
        float t = fl->age / (float)FLASK_FLOAT_LIFE;
        HdColor c;
        if (fl->delta < 0 && fl->which == 0) { c.r = 255; c.g = 80; c.b = 70; }
        else if (fl->delta < 0) { c.r = 120; c.g = 170; c.b = 255; }
        else { c.r = 110; c.g = 255; c.b = 110; }
        c.a = (uint8_t)(t < 0.5f ? 255 : 255 * (1.0f - t) * 2);
        snprintf(s, sizeof s, "%+d", fl->delta);
        drawn |= draw_text(font, frame, s, flask_left[fl->which] + FLASK_W / 2, FLOAT_TOP - FLOAT_RISE * t, 5 * 64, c, border);
    }
    return drawn;
}
