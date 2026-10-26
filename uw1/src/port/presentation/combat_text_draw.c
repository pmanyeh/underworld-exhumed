#include "combat_text.h"
#include "hd_font.h"
#include "i18n/utf8.h"
#include <stdio.h>
#include <string.h>

/* Avoid a new libm dependency in the Linux game link. */
static int floor_pixel(float x) { int i = (int)x; return i - (i > x); }
static int ceil_pixel(float x) { int i = (int)x; return i + (i < x); }

static void rect(const PlatOverlay *f, float l, float t, float r, float b, HdColor c)
{
    int x, y, x0, y0, x1, y1;
    float sx = f->viewport_w / f->logical_width, sy = f->viewport_h / f->logical_height;
    x0 = floor_pixel(f->viewport_x + l * sx); y0 = floor_pixel(f->viewport_y + t * sy);
    x1 = ceil_pixel(f->viewport_x + r * sx); y1 = ceil_pixel(f->viewport_y + b * sy);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > f->width) x1 = f->width;
    if (y1 > f->height) y1 = f->height;
    for (y = y0; y < y1; y++) for (x = x0; x < x1; x++) {
        uint8_t *p = f->rgba + (size_t)y * f->pitch + (size_t)x * 4;
        p[0] = c.r; p[1] = c.g; p[2] = c.b; p[3] = c.a;
    }
}
static void label(HdFont *font, const PlatOverlay *f, const CombatFrame *c,
                   const char *s, float cx, float y, HdColor color, float border)
{
    float clip[4] = { (float)c->left, (float)c->top, (float)c->right, (float)c->bottom };
    size_t n = strlen(s), offset;
    uint32_t cp;
    float width = 0, x;
    for (offset = 0; offset < n;) {
        if (utf8_next(s, n, &offset, &cp) != 1) return;
        width += hd_scroll_advance(font, cp, 6 * 64) / 64.0f;
    }
    x = cx - width / 2;
    if (x < clip[0] + 1) x = clip[0] + 1;
    if (x + width > clip[2] - 1) x = clip[2] - 1 - width;
    for (offset = 0; offset < n;) {
        if (utf8_next(s, n, &offset, &cp) != 1) return;
        hd_scroll_glyph_border(font, f, cp, x, y, 6, clip, color, border);
        x += hd_scroll_advance(font, cp, 6 * 64) / 64.0f;
    }
}
int combat_text_draw(HdFont *font, const PlatOverlay *f, const CombatFrame *c, float border)
{
    static uint64_t seen;
    static int stale;
    int i, j, drawn = 0;
    if (!font || !f || !c || !f->rgba || f->logical_width <= 0 || f->logical_height <= 0) return 0;
    if (c->generation != seen) { seen = c->generation; stale = 0; }
    else if (++stale > 20) return 0;
    for (i = 0; c->health && i < c->count; i++) {
        const CombatActor *a = &c->actors[i];
        HdColor bg = { 15, 12, 10, 255 }, color = { 85, 220, 90, 255 };
        float ratio, x, y, w;
        if (!a->engaged || a->hp <= 0 || a->maximum <= 0) continue;
        ratio = a->hp / (float)a->maximum;
        if (ratio > 1) ratio = 1;
        if (ratio <= .25f) { color.r = 255; color.g = 70; color.b = 60; }
        else if (ratio <= .5f) { color.r = 255; color.g = 205; color.b = 65; }
        w = a->right - a->left;
        if (w < 18) w = 18;
        if (w > 30) w = 30;
        x = (a->left + a->right - w) / 2;
        if (x < c->left + 1) x = (float)c->left + 1;
        if (x + w >= c->right) x = c->right - w - 1;
        y = a->top - 7.0f;
        if (y < c->top + 1) y = (float)c->top + 1;
        rect(f, x, y, x + w, y + 3, bg);
        rect(f, x + 1, y + 1, x + 1 + (w - 2) * ratio, y + 2, color);
        drawn = 1;
    }
    for (i = 0; c->numbers && i < c->floats; i++) {
        const CombatFloat *e = &c->events[i];
        HdColor color = { 255, 205, 90, 255 };
        float t = e->age / (float)COMBAT_LIFE;
        float cx = (c->left + c->right) / 2.0f, top = (c->top + c->bottom) / 2.0f;
        char s[24];
        if (e->slot) {
            for (j = 0; j < c->count; j++) if (c->actors[j].slot == e->slot) break;
            if (j == c->count) continue;
            cx = (c->actors[j].left + c->actors[j].right) / 2.0f;
            /* Begin below the bar, rising towards it; concurrent blows use lower lanes. */
            top = c->actors[j].top + 16.0f;
        }
        if (e->kind == COMBAT_DAMAGE) {
            color.r = 255; color.g = 100; color.b = 80;
            snprintf(s, sizeof s, "-%d", e->amount);
        } else snprintf(s, sizeof s, "%s", e->text);
        color.a = (uint8_t)(t < .5f ? 255 : 510 * (1 - t));
        top += e->lane * 7 - 18 * t;
        if (top < c->top + 1) top = (float)c->top + 1;
        label(font, f, c, s, cx + (e->lane - 1) * 5, top, color, border);
        drawn = 1;
    }
    return drawn;
}
