/* P12: the settings screen (Exhume's ui/settings.c) in the HD font: its words in Chinese when the
   game runs in zh-TW, any text that is not ASCII in the HD font, and the Language tab's sample
   of the Chinese text with the values the screen now has. On the thread that draws the screen,
   which is the presentation thread's (the font is the presentation's). */
#include "settings_text.h"
#include "hd_font.h"
#include "i18n/settings_words.h"
#include "i18n/utf8.h"
#include "ui/settings.h"
#include <string.h>

void port_text_choice(int *size, int *line, int *border10, int *smooth);   /* sys/settab.c */

static HdFont *font;
static int chinese, smooth_now;

/* the layer as an overlay frame: 640 by 400, its own logical size */
static PlatOverlay layer(uint32_t *px)
{
    PlatOverlay f;
    memset(&f, 0, sizeof f);
    f.rgba = (uint8_t *)px;
    f.width = f.window_width = f.logical_width = SET_W;
    f.height = f.window_height = f.logical_height = SET_H;
    f.pitch = SET_W * 4;
    f.viewport_w = SET_W;
    f.viewport_h = SET_H;
    return f;
}

static HdColor colour(uint32_t c)
{
    HdColor h;
    h.r = (uint8_t)(c & 0xFF); h.g = (uint8_t)(c >> 8 & 0xFF); h.b = (uint8_t)(c >> 16 & 0xFF); h.a = 255;
    return h;
}

#define EM 14.0f        /* in the screen's 16-pixel lines */

static const char *translate(const char *english)
{
    /* the language's own name is in Chinese whatever the language, where the font can draw it */
    if (!chinese && strcmp(english, "Traditional Chinese")) return NULL;
    return port_settings_word(english);
}

static int width(const char *utf8)
{
    PlatOverlay f;
    HdTextMetrics m;
    uint32_t dummy = 0;
    f = layer(&dummy);
    if (hd_text_measure_logical(font, &f, utf8, strlen(utf8), EM, 0, &m) != HD_OK) return 0;
    return (int)(m.width + 0.5f);
}

static int draw(uint32_t *px, int x, int y, uint32_t c, const char *utf8)
{
    PlatOverlay f = layer(px);
    if (hd_text_draw(font, &f, utf8, strlen(utf8), (float)x, (float)y, EM, 0, colour(c)) != HD_OK) return 0;
    return width(utf8);
}

static const struct settings_text hooks = { translate, draw, width };

void settings_text_start(HdFont *f, int zh, int smooth)
{
    font = f;
    chinese = zh;
    smooth_now = smooth;
    settings_set_text(f ? &hooks : NULL);
}

/* The sample: two lines of the game's kind of text at the size, spacing, outline and edges the
   rows now have, the layer being twice the game's pixels */
void port_text_preview(uint32_t *px, int x, int y, int w, int h, int unused)
{
    static const char *const lines[] = { "你看到一把堪用的長劍。", "Lakshi Longtooth：你為什麼來？" };
    PlatOverlay f = layer(px);
    float clip[4];
    int size, line, border10, smooth, k;
    HdColor ink = { 0xE8, 0xD9, 0xB5, 255 };
    (void)unused;
    if (!font) return;
    port_text_choice(&size, &line, &border10, &smooth);
    clip[0] = (float)x; clip[1] = (float)y; clip[2] = (float)(x + w); clip[3] = (float)(y + h);
    hd_font_set_antialiasing(font, smooth);
    for (k = 0; k < 2; k++) {
        const char *s = lines[k];
        size_t len = strlen(s), off = 0;
        uint32_t cp;
        float cx = (float)x, em = 2.0f * size;
        float top = (float)y + k * (em * 1.3f + 2.0f * line);
        while (utf8_next(s, len, &off, &cp) > 0) {
            if (cp != ' ') hd_scroll_glyph_border(font, &f, cp, cx, top, em, clip, ink, border10 / 10.0f * 2.0f);
            cx += hd_scroll_advance(font, cp, (int)(em * 64)) / 64.0f;
        }
    }
    hd_font_set_antialiasing(font, smooth_now);
}
