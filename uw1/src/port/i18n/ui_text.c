#include "i18n/ui_text.h"
#include "i18n/localization.h"
#include "i18n/phrases.h"
#include "presentation/hd_label.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* "12TH": a level and its English ordinal (STATS.C's sp_hdr) */
static const char *ordinal(const char *s)
{
    static char buf[32];
    int n = 0;
    const char *p = s;
    while (*p >= '0' && *p <= '9') n = n * 10 + (*p++ - '0');
    if (p == s || p - s > 3 || (strcmp(p, "ST") && strcmp(p, "ND") && strcmp(p, "RD") && strcmp(p, "TH"))) return NULL;
    snprintf(buf, sizeof buf, port_zh_lit("stats.level"), n);
    return buf;
}

const char *port_ui_text(const char *english)
{
    const char *p, *zh;
    if (!english || !port_i18n_enabled()) return NULL;
    for (p = english; *p; p++)
        if ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')) break;
    if (!*p) return NULL;
    if ((zh = port_i18n_named(english)) != NULL) return zh;
    if ((zh = port_i18n_display(english)) != english) return zh;
    if ((zh = port_i18n_display_nocase(english)) != NULL) return zh;
    if ((zh = ordinal(english)) != NULL) return zh;
    return port_zh_literal(english);
}

void vga_get_dac(uint8_t rgb6[768]);

/* The brightest of a picture's common colours (each at least a twentieth of its opaque
   pixels): a lettered button's letters, whatever their outline and shading. */
static int letter_colour(const unsigned char *img, int n)
{
    static int counts[256];
    uint8_t dac[768];
    int i, opaque = 0, best = -1, light = -1;
    memset(counts, 0, sizeof counts);
    for (i = 0; i < n; i++) if (img[i]) { counts[img[i]]++; opaque++; }
    vga_get_dac(dac);
    for (i = 1; i < 256; i++)
        if (counts[i] * 20 >= opaque && counts[i]) {
            int l = dac[i * 3] * 3 + dac[i * 3 + 1] * 6 + dac[i * 3 + 2];
            if (l > light) { light = l; best = i; }
        }
    return best;
}

int port_menu_button(int i, int x, int y, int w, int h, const unsigned char *img, int selected)
{
    static const char *const keys[] = { "menu.intro", "menu.create", "menu.credits", "menu.journey" };
    int colour, top = 199 - y;
    (void)selected;
    if (i < 0 || i > 3 || !img || !hd_label_enabled() || !port_i18n_enabled()) return 0;
    if ((colour = letter_colour(img, w * h)) < 0) return 0;
    return hd_label_box(port_zh_lit(keys[i]), x, top, x + w, top + h, colour, HD_LABEL_CENTRE,
                        12 * 64, -1, 1.0f) == 0;
}

unsigned char gr_read_pixel(int x, int y);     /* src/port/gfx/grcore.c: the drawing page, bottom-up */

/* Lettered art in a box (top-down): its ink, the brightest of its common colours, and its
   face, the commonest of the others that are not dark (a shaded face outnumbers neither its
   black outline nor its letters). -1 when it has neither. */
static int box_colours(int left, int top, int right, int bottom, int *face)
{
    static int counts[256];
    uint8_t dac[768];
    int x, y, i, n = 0, ink = -1, light = -1;
    memset(counts, 0, sizeof counts);
    for (y = top; y < bottom; y++)
        for (x = left; x < right; x++) { counts[gr_read_pixel(x, 199 - y)]++; n++; }
    vga_get_dac(dac);
    for (i = 1; i < 256; i++)
        if (counts[i] && counts[i] * 50 >= n) {
            int l = dac[i * 3] * 3 + dac[i * 3 + 1] * 6 + dac[i * 3 + 2];
            if (l > light) { light = l; ink = i; }
        }
    *face = -1;
    for (i = 0; i < 256; i++) {
        int l = dac[i * 3] * 3 + dac[i * 3 + 1] * 6 + dac[i * 3 + 2];
        if (i != ink && counts[i] && l * 4 >= light && (*face < 0 || counts[i] > counts[*face])) *face = i;
    }
    return *face < 0 ? -1 : ink;
}

static void patch(const char *key, int left, int top, int right, int bottom)
{
    int face, ink = box_colours(left, top, right, bottom, &face);
    if (ink >= 0) (void)hd_label_box(port_zh_lit(key), left, top, right, bottom, ink, HD_LABEL_CENTRE, 0, face, 0);
}

/* OPTBTNS.GR's lettered pictures (OPTIONS.C): each button picture, lit and not, by number */
static const char *option_key(int pic)
{
    static const char *const keys[] = {
        "opt.save", "opt.save", "opt.restore", "opt.restore", "opt.music", "opt.music",   /* 6.. */
        "opt.sound", "opt.sound", "opt.detail", "opt.detail", "opt.return", "opt.return",
        "opt.quit", "opt.quit", "opt.on", "opt.on", "opt.off", "opt.off",
        "opt.cancel", "opt.cancel", "opt.done", "opt.done", "opt.done", "opt.done",
        0, 0, 0, 0, 0, 0, 0, 0,                                                           /* I .. IV */
        "opt.low", "opt.low", "opt.medium", "opt.medium", "opt.high", "opt.high",
        "opt.very_high", "opt.very_high", "opt.restore_title", "opt.music_on",
        "opt.music_off", "opt.sound_on", "opt.sound_off", "opt.turn_music", "opt.turn_sound",
        "opt.level_low", "opt.level_medium", "opt.level_high", "opt.level_very_high",
        "opt.yes", "opt.yes", "opt.no", "opt.no",                                        /* .. 60 */
    };
    return pic >= 6 && pic < 6 + (int)(sizeof keys / sizeof keys[0]) ? keys[pic - 6] : 0;
}

/* the box of the options panel's row (OPTIONS.C's draw_options_btn), top-down */
static int row_top(int row) { return 12 + (6 - row) * 15; }

void port_options_back(int pic)
{
    if (!hd_label_enabled() || !port_i18n_enabled()) return;
    hd_label_forget(4, 10, 0x27, 10 + 0x6C);
    switch (pic) {          /* the words the page's background has besides its buttons */
    case 1: {
        static const char *const rows[] = { "opt.quit", "opt.return", "opt.detail", "opt.sound", "opt.music", "opt.restore", "opt.save" };
        int row;
        for (row = 0; row < 7; row++) patch(rows[row], 5, row_top(row), 0x26, row_top(row) + 14);
        break;
    }
    case 2: case 3: case 4: case 5: {
        /* each page's rows from 0 up; the rows its other pictures draw over are left */
        static const char *const pages[4][7] = {
            { 0, "opt.cancel", 0, 0, 0, 0, "opt.save_title" },
            { 0, 0, 0, "opt.no", "opt.yes", "opt.sure", "opt.quit_title" },
            { 0, 0, "opt.done", "opt.off", "opt.on", 0, 0 },
            { "opt.done", "opt.very_high", "opt.high", "opt.medium", "opt.low", "opt.set_level", 0 },
        };
        int row;
        for (row = 0; row < 7; row++) {
            /* the detail page's title picture reaches 2 rows into row 5, whose words reach one
               row further down */
            int shift = pic == 5 && row == 5;
            if (pages[pic - 2][row]) patch(pages[pic - 2][row], 5, row_top(row) + 2 * shift, 0x26, row_top(row) + 14 + shift);
        }
        break;
    }
    }
}

void port_options_btn(int row, int pic)
{
    const char *key = option_key(pic);
    if (key && hd_label_enabled() && port_i18n_enabled()) patch(key, 5, row_top(row), 5 + 0x1F, row_top(row) + 14);
}

void port_options_title(int pic)
{
    const char *key = option_key(pic);
    if (key && hd_label_enabled() && port_i18n_enabled()) patch(key, 5, 199 - 0xBC, 5 + 0x22, 199 - 0xBC + 0x12);
}

/* The statistics page's attribute words, which its art has (STATS.C's RedispStat): one row of
   7 pixels each from y 28, in the page's text colour, over the art's background. */
void port_stats_labels(void)
{
    static const char *const keys[] = { "stats.str", "stats.dex", "stats.int", "stats.vit", "stats.mana", "stats.exp" };
    int i;
    if (!hd_label_enabled() || !port_i18n_enabled()) return;
    for (i = 0; i < 6; i++)
        (void)hd_label_patch(port_zh_lit(keys[i]), 0xF1, 28 + i * 7, 0x112, 35 + i * 7, 0xF1, HD_LABEL_LEFT);
}
