/* Native-only, presentation-thread-owned text. No FreeType or SDL types leak.
   Each font owns one face and a bounded grayscale glyph cache; do not share it
   across threads. Text pointers are borrowed only for each call, never retained. */
#ifndef UW1_HD_FONT_H
#define UW1_HD_FONT_H
#include "plat.h"
typedef struct HdFont HdFont;
typedef struct HdColor { uint8_t r, g, b, a; } HdColor;
typedef struct HdFontStats {
    uint64_t hits, misses, rasterizations, evictions, missing_glyphs;
    size_t entries, bitmap_bytes;
} HdFontStats;
typedef struct HdTextMetrics {
    float width, height; /* advance bounds, not tight ink bounds */
    unsigned pixel_size;
    size_t lines;
} HdTextMetrics;
enum { HD_OK = 0, HD_ARGUMENT = -1, HD_UNAVAILABLE = -2, HD_FONT = -3,
       HD_MEMORY = -4, HD_TEXT = -5, HD_GLYPH = -6 };
/* *out must be NULL. Requires a scalable Unicode face; no OS font search. */
int hd_font_init(HdFont **out, const char *path);
void hd_font_shutdown(HdFont *font); /* NULL is safe */
const char *hd_font_error(int status);
void hd_font_stats(const HdFont *font, HdFontStats *out);
/* Native: pixel_size 1..256, max_width in native pixels; <=0 selects the widest
   32-bit 26.6 layout limit (INT_MAX/64 pixels, rather than an infinite line).
   Logical: em and wrap_width are game units. The backend's viewport provides
   independent X/Y scales. Font ppem follows Y; glyphs remain square in output.
   Measures use Phase 3 layout, 26.6 additive advances, no kerning or shaping.
   Text is strict UTF-8, <=1 MiB, no NUL; controls are zero-width, not executed.
   Metrics are published on success only. Logical conversion needs a valid frame. */
int hd_text_measure_native(HdFont *font, const char *text, size_t length,
                           unsigned pixel_size, float max_width, HdTextMetrics *out);
int hd_text_measure_logical(HdFont *font, const PlatOverlay *frame,
                            const char *text, size_t length, float em,
                            float wrap_width, HdTextMetrics *out);
/* x,y are the logical top-left of the line box, with ascent to its baseline.
   Draw directly into the native RGBA layer, clipping to its output bounds.
   Glyph masks use straight-alpha source-over (including an existing layer).
   Invalid input is rejected before any pixels are changed. */
int hd_text_draw(HdFont *font, const PlatOverlay *frame, const char *text,
                  size_t length, float x, float y, float em, float wrap_width, HdColor color);
/* Scroll profile: fixed logical metrics from a 64px reference face, with 5/4
   horizontal room for 4:3 correction. Separate game-owned font for measuring.
   Result is logical 26.6; em_64 is the logical em in 26.6. */
int hd_scroll_advance(HdFont *font, uint32_t scalar, int em_64);
/* Draw one scalar at its prepared logical position; clip={left,top,right,bottom}.
   Resizing changes masks, never the prepared line breaks or choice hit boxes. */
int hd_scroll_glyph(HdFont *font, const PlatOverlay *frame, uint32_t scalar,
                    float x, float y, float em, const float clip[4], HdColor color);
/* The same with a black outline border logical pixels wide (0 none, at most 8). */
int hd_scroll_glyph_border(HdFont *font, const PlatOverlay *frame, uint32_t scalar,
                           float x, float y, float em, const float clip[4], HdColor color, float border);
/* Antialiasing on (the default) or off: off, each pixel of a glyph is all or nothing. */
void hd_font_set_antialiasing(HdFont *font, int on);
/* A second font file for the characters the first lacks (measured and drawn from it alike). */
int hd_font_set_fallback(HdFont *font, const char *path);
#endif
