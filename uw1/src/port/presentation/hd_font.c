#include "hd_font.h"
#include "i18n/layout.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

const char *hd_font_error(int status)
{
    switch (status) {
    case HD_OK: return "ok";
    case HD_ARGUMENT: return "invalid argument or geometry";
    case HD_UNAVAILABLE: return "FreeType unavailable in this build";
    case HD_FONT: return "cannot open scalable Unicode font";
    case HD_MEMORY: return "glyph cache allocation failed";
    case HD_TEXT: return "invalid UTF-8 or layout input";
    default: return "glyph rasterization failed";
    }
}

#ifdef UW1_HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#define CACHE_SLOTS 512
#define CACHE_BYTES (8u * 1024u * 1024u)
typedef struct Glyph {
    uint32_t scalar;
    unsigned size, mode; /* face is implicit in the owning HdFont */
    int width, height, left, top, advance;
    uint8_t *mask;
    uint64_t used;
} Glyph;
struct HdFont {
    FT_Library library;
    FT_Face face;
    Glyph glyphs[CACHE_SLOTS];
    HdFontStats stats;
    uint64_t clock;
    int error;
};

int hd_font_init(HdFont **out, const char *path)
{
    HdFont *f;
    if (!out || *out || !path || !*path) return HD_ARGUMENT;
    f = calloc(1, sizeof *f);
    if (!f) return HD_MEMORY;
    if (FT_Init_FreeType(&f->library) || FT_New_Face(f->library, path, 0, &f->face) ||
        !FT_IS_SCALABLE(f->face) || FT_Select_Charmap(f->face, FT_ENCODING_UNICODE)) {
        hd_font_shutdown(f); return HD_FONT;
    }
    *out = f;
    return HD_OK;
}
void hd_font_shutdown(HdFont *f)
{
    unsigned i;
    if (!f) return;
    for (i = 0; i < CACHE_SLOTS; i++) free(f->glyphs[i].mask);
    if (f->face) FT_Done_Face(f->face);
    if (f->library) FT_Done_FreeType(f->library);
    free(f);
}
void hd_font_stats(const HdFont *f, HdFontStats *out)
{
    if (out) { memset(out, 0, sizeof *out); if (f) *out = f->stats; }
}
static void evict(HdFont *f, Glyph *g)
{
    f->stats.bitmap_bytes -= (size_t)g->width * g->height;
    f->stats.entries--; f->stats.evictions++;
    free(g->mask); memset(g, 0, sizeof *g);
}
static Glyph *oldest(HdFont *f)
{
    unsigned i;
    Glyph *g = NULL;
    for (i = 0; i < CACHE_SLOTS; i++) if (f->glyphs[i].size &&
        (!g || f->glyphs[i].used < g->used)) g = &f->glyphs[i];
    return g;
}
static Glyph *glyph(HdFont *f, unsigned size, uint32_t scalar)
{
    unsigned i;
    FT_UInt index;
    FT_GlyphSlot s;
    size_t bytes;
    Glyph *g = NULL;
    const unsigned mode = FT_RENDER_MODE_NORMAL;
    for (i = 0; i < CACHE_SLOTS; i++) {
        Glyph *p = &f->glyphs[i];
        if (p->size == size && p->scalar == scalar && p->mode == mode) {
            p->used = ++f->clock; f->stats.hits++; return p;
        }
        if (!p->size) g = p;
    }
    f->stats.misses++;
    index = FT_Get_Char_Index(f->face, scalar);
    if (!index) {
        f->stats.missing_glyphs++;
        index = FT_Get_Char_Index(f->face, 0xfffd); /* else .notdef */
    }
    if (FT_Set_Pixel_Sizes(f->face, 0, size) ||
        FT_Load_Glyph(f->face, index, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP) ||
        FT_Render_Glyph(f->face->glyph, (FT_Render_Mode)mode)) {
        f->error = HD_GLYPH; return NULL;
    }
    s = f->face->glyph;
    if (s->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY || s->bitmap.num_grays != 256 ||
        s->bitmap.width > 2048 || s->bitmap.rows > 2048 ||
        s->advance.x < 0 || s->advance.x > INT_MAX) { f->error = HD_GLYPH; return NULL; }
    bytes = (size_t)s->bitmap.width * s->bitmap.rows;
    while (f->stats.bitmap_bytes + bytes > CACHE_BYTES) {
        Glyph *victim = oldest(f);
        evict(f, victim);
        if (!g) g = victim;
    }
    if (!g || g->size) { g = oldest(f); evict(f, g); }
    if (bytes) {
        int row;
        g->mask = malloc(bytes);
        if (!g->mask) { f->error = HD_MEMORY; return NULL; }
        for (row = 0; row < (int)s->bitmap.rows; row++) {
            /* FreeType pitch is signed: negative pitch walks upwards. */
            const uint8_t *src = s->bitmap.buffer + (ptrdiff_t)row * s->bitmap.pitch;
            memcpy(g->mask + (size_t)row * s->bitmap.width, src, s->bitmap.width);
        }
    }
    g->scalar = scalar; g->size = size; g->mode = mode;
    g->width = (int)s->bitmap.width; g->height = (int)s->bitmap.rows;
    g->left = s->bitmap_left; g->top = s->bitmap_top; g->advance = (int)s->advance.x;
    g->used = ++f->clock;
    f->stats.entries++; f->stats.bitmap_bytes += bytes; f->stats.rasterizations++;
    return g;
}
typedef struct Measure { HdFont *font; unsigned size; } Measure;
static int advance(uint32_t cp, void *context)
{
    Measure *m = context;
    Glyph *g = glyph(m->font, m->size, cp == '\t' ? ' ' : cp);
    if (g && cp == '\t' && g->advance > INT_MAX / 4) { m->font->error = HD_GLYPH; return -1; }
    return g ? g->advance * (cp == '\t' ? 4 : 1) : -1;
}
static int prepare(HdFont *f, const char *text, size_t len, unsigned size,
                   float width, struct text_layout *layout, HdTextMetrics *metrics)
{
    Measure m = { f, size };
    size_t i;
    int status, limit;
    int64_t widest = 0;
    if (!f || !metrics || !size || size > 256 || !isfinite(width) || width > 1000000) return HD_ARGUMENT;
    if (FT_Set_Pixel_Sizes(f->face, 0, size)) return HD_GLYPH;
    f->error = 0;
    limit = width > 0 ? (int)(width * 64) : INT_MAX;
    if (limit < 1) limit = 1;
    status = layout_text(text, len, limit, advance, &m, layout, NULL);
    if (status != LAYOUT_OK) return f->error ? f->error : status == LAYOUT_NO_MEMORY ? HD_MEMORY : HD_TEXT;
    for (i = 0; i < layout->line_count; i++) if (layout->lines[i].width > widest) widest = layout->lines[i].width;
    metrics->width = (float)widest / 64;
    metrics->height = (float)(f->face->size->metrics.height / 64.0 * layout->line_count);
    metrics->lines = layout->line_count; metrics->pixel_size = size;
    return HD_OK;
}
static int geometry(const PlatOverlay *p, float em, float width, unsigned *size, float *sx, float *sy)
{
    double pixels;
    if (!p || p->logical_width <= 0 || p->logical_height <= 0 ||
        !isfinite(p->viewport_x) || !isfinite(p->viewport_y) ||
        !isfinite(p->viewport_w) || !isfinite(p->viewport_h) ||
        p->viewport_w <= 0 || p->viewport_h <= 0 || !isfinite(em) || em <= 0 ||
        !isfinite(width) || width > 1000000) return HD_ARGUMENT;
    *sx = p->viewport_w / p->logical_width; *sy = p->viewport_h / p->logical_height;
    if (!isfinite(*sx) || !isfinite(*sy) || *sx <= 0 || *sy <= 0) return HD_ARGUMENT;
    pixels = em * (double)*sy;
    if (pixels < .5 || pixels > 256.49 || !isfinite(width * *sx)) return HD_ARGUMENT;
    *size = (unsigned)(pixels + .5);
    return HD_OK;
}
int hd_text_measure_native(HdFont *f, const char *text, size_t len, unsigned size, float width, HdTextMetrics *out)
{
    struct text_layout layout = {0};
    HdTextMetrics m;
    int status;
    if (!out) return HD_ARGUMENT;
    status = prepare(f, text, len, size, width, &layout, &m);
    if (!status) *out = m;
    layout_free(&layout); return status;
}
int hd_text_measure_logical(HdFont *f, const PlatOverlay *p, const char *text, size_t len,
                            float em, float width, HdTextMetrics *out)
{
    unsigned size;
    float sx, sy;
    HdTextMetrics m;
    int status = geometry(p, em, width, &size, &sx, &sy);
    if (status || !out) return HD_ARGUMENT;
    status = hd_text_measure_native(f, text, len, size, width > 0 ? width * sx : 0, &m);
    if (!status) { m.width /= sx; m.height /= sy; *out = m; }
    return status;
}
static void paint(const PlatOverlay *p, const Glyph *g, double x, double baseline, HdColor c)
{
    int ox, oy, row, col;
    /* Bound before converting, then round to the nearest output pixel. C casts
       truncate toward zero, so the negative fraction needs one extra step.
       This small integer operation needs no separate libm link dependency. */
    x += g->left + .5; baseline += .5 - g->top;
    if (x >= p->width || baseline >= p->height || x + g->width <= 0 || baseline + g->height <= 0) return;
    ox = (int)x; oy = (int)baseline;
    if (x < ox) ox--;
    if (baseline < oy) oy--;
    for (row = 0; row < g->height; row++) {
        int64_t dy = (int64_t)oy + row;
        if (dy < 0 || dy >= p->height) continue;
        for (col = 0; col < g->width; col++) {
            uint8_t *d;
            unsigned a, da, oa;
            int64_t dx = (int64_t)ox + col;
            if (dx < 0 || dx >= p->width) continue;
            a = (g->mask[(size_t)row * g->width + col] * c.a + 127u) / 255u;
            if (!a) continue;
            d = p->rgba + (size_t)dy * p->pitch + (size_t)dx * 4;
            da = d[3]; oa = a * 255 + da * (255 - a);
            d[0] = (uint8_t)((c.r * a * 255 + d[0] * da * (255 - a) + oa / 2) / oa);
            d[1] = (uint8_t)((c.g * a * 255 + d[1] * da * (255 - a) + oa / 2) / oa);
            d[2] = (uint8_t)((c.b * a * 255 + d[2] * da * (255 - a) + oa / 2) / oa);
            d[3] = (uint8_t)((oa + 127) / 255);
        }
    }
}
int hd_text_draw(HdFont *f, const PlatOverlay *p, const char *text, size_t len,
                  float x, float y, float em, float width, HdColor color)
{
    struct text_layout layout = {0};
    HdTextMetrics m;
    unsigned size;
    float sx, sy;
    size_t i, j;
    double baseline, line_height;
    int status = geometry(p, em, width, &size, &sx, &sy);
    if (status || !p->rgba || p->width <= 0 || p->width > INT_MAX / 4 || p->height <= 0 ||
        p->pitch < p->width * 4 || (size_t)p->height > SIZE_MAX / (size_t)p->pitch ||
        !isfinite(x) || !isfinite(y) || fabsf(x) > 1000000 || fabsf(y) > 1000000) return HD_ARGUMENT;
    status = prepare(f, text, len, size, width > 0 ? width * sx : 0, &layout, &m);
    if (status) return status;
    line_height = f->face->size->metrics.height / 64.0;
    baseline = p->viewport_y + y * (double)sy + f->face->size->metrics.ascender / 64.0;
    for (i = 0; i < layout.line_count; i++, baseline += line_height) {
        const struct layout_line *line = &layout.lines[i];
        int64_t pen = 0;
        for (j = line->unit_start; j < line->unit_start + line->unit_count; j++) {
            const struct layout_unit *u = &layout.units[j];
            Glyph *g;
            if (u->kind != LAYOUT_GLYPH || u->flags & LAYOUT_TRIMMED) continue;
            g = glyph(f, size, u->scalar == '\t' ? ' ' : u->scalar);
            if (!g) { status = f->error; goto done; }
            if (u->scalar != '\t') paint(p, g, p->viewport_x + x * (double)sx + pen / 64.0, baseline, color);
            pen += u->advance;
        }
    }
done:
    layout_free(&layout); return status;
}
#else
/* No dependency means a usable original port, with an explicit opt-in error. */
int hd_font_init(HdFont **out, const char *path) { (void)out; (void)path; return HD_UNAVAILABLE; }
void hd_font_shutdown(HdFont *f) { (void)f; }
void hd_font_stats(const HdFont *f, HdFontStats *out) { (void)f; if (out) memset(out, 0, sizeof *out); }
int hd_text_measure_native(HdFont *f, const char *t, size_t n, unsigned s, float w, HdTextMetrics *o)
{ (void)f; (void)t; (void)n; (void)s; (void)w; (void)o; return HD_UNAVAILABLE; }
int hd_text_measure_logical(HdFont *f, const PlatOverlay *p, const char *t, size_t n, float e, float w, HdTextMetrics *o)
{ (void)f; (void)p; (void)t; (void)n; (void)e; (void)w; (void)o; return HD_UNAVAILABLE; }
int hd_text_draw(HdFont *f, const PlatOverlay *p, const char *t, size_t n, float x, float y, float e, float w, HdColor c)
{ (void)f; (void)p; (void)t; (void)n; (void)x; (void)y; (void)e; (void)w; (void)c; return HD_UNAVAILABLE; }
#endif
