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
                        12 * 64, 0, 1.0f) == 0;
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
