/* UW2's localized text (Phase 14). The catalog's translations reach only the HD scroll's
   display state (scroll_text.c's cells), never the game's strings or memory: a saved game
   and a replay are the same in every language. The modules are the runtime's, shared with
   UW1; this file only wires them to UW2's options, folders and game thread. */
#include "presentation/localized.h"
#include "i18n/input_alias.h"
#include "i18n/name_gloss.h"
#include "i18n/phrases.h"
#include "i18n/user_text.h"
#include "presentation/hd_label.h"
#include "presentation/caption.h"
#include "i18n/localization.h"
#include "presentation/hd_cursor.h"
#include "presentation/hd_font.h"
#include "presentation/scroll_bridge.h"
#include "presentation/scroll_overlay.h"
#include "presentation/scroll_text.h"
#include "presentation/text_style.h"
#include "port.h"
#include "ui/settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void scroll_wait(int ticks, char mouse);        /* SCROLLIO.C */

static const char *locale = "en", *i18n_file, *font_path;
static const char *scroll_log, *i18n_trace;     /* tests: the HD scroll's and the lookups' JSONL */
static int hd_scroll, locale_given;
static char catalog[1200];

/* Box 0 is the message scroll (and the menu scroll a conversation's choices use), box 1 the
   conversation's; each with its style and the font files it resolved to. The presentation
   thread draws with its own fonts; the game thread measures with its own copies. */
static HdTextStyle style[2];
static char font_file[2][1024], fallback_file[2][1024];
static HdFont *draw_font[2], *measure_font[2];
static ScrollDrawStyle draw_style[2];
typedef struct Metrics { HdFont *font; int em64; } Metrics;
static Metrics metrics[2];

int port_text_option(int argc, char **argv, int *i)
{
    const char *a = argv[*i];
    if (!strcmp(a, "--hd-scroll")) { hd_scroll = 1; return 1; }
    if (*i + 1 >= argc) return 0;
    if (!strcmp(a, "--locale")) {
        locale = argv[++*i];
        locale_given = 1;
        return strcmp(locale, "en") && strcmp(locale, "zh-TW") ? -1 : 1;
    }
    if (!strcmp(a, "--i18n-file")) { i18n_file = argv[++*i]; return 1; }
    if (!strcmp(a, "--font")) { font_path = argv[++*i]; return 1; }
    if (!strcmp(a, "--scroll-log")) { scroll_log = argv[++*i]; return 1; }
    if (!strcmp(a, "--i18n-trace")) { i18n_trace = argv[++*i]; return 1; }
    if (!strcmp(a, "--shot-on-text")) { port_scroll_shot_after(atoi(argv[++*i])); return 1; }
    return 0;
}

static int same_font(void)
{
    return !strcmp(font_file[1], font_file[0]) && !strcmp(fallback_file[1], fallback_file[0]);
}

int port_text_setup(const char *home, int interactive)
{
    char config[1200], settings[1200];
    int box, result;
    if (!locale_given && interactive) {
        /* a player's run (not a test's, so that replays stay as they were): the settings file's
           language; with none yet, a folder that has the catalog (the home folder's
           i18n/zh-TW, else the current folder's, a playtest folder's) starts in zh-TW */
        char lang[16];
        FILE *probe;
        int have = port_config_get(home, "language", lang, sizeof lang) == 0, zh = have && !strcmp(lang, "zh-TW");
        if (i18n_file) snprintf(catalog, sizeof catalog, "%s", i18n_file);
        else {
            snprintf(catalog, sizeof catalog, "%s/i18n/zh-TW/catalog.uw2loc", home);
            if ((probe = fopen(catalog, "rb")) != NULL) fclose(probe);
            else snprintf(catalog, sizeof catalog, "i18n/zh-TW/catalog.uw2loc");
        }
        if ((probe = fopen(catalog, "rb")) != NULL) {
            fclose(probe);
            if (!have) {
                zh = 1;
                port_config_set(home, "language", "zh-TW");
            }
            if (zh) { locale = "zh-TW"; i18n_file = catalog; hd_scroll = 1; }
        } else if (zh)
            fprintf(stderr, "uw2port: language zh-TW: no catalog (%s); using English\n", catalog);
    }
    if (!strcmp(locale, "zh-TW") && !i18n_file) {
        /* the home folder's i18n/zh-TW, else the current folder's (a playtest folder's) */
        FILE *probe;
        snprintf(catalog, sizeof catalog, "%s/i18n/zh-TW/catalog.uw2loc", home);
        if ((probe = fopen(catalog, "rb")) != NULL) fclose(probe);
        else snprintf(catalog, sizeof catalog, "i18n/zh-TW/catalog.uw2loc");
        i18n_file = catalog;
    }
    if (hd_scroll && strcmp(locale, "zh-TW")) {
        fputs("uw2port: --hd-scroll needs --locale zh-TW\n", stderr);
        return -1;
    }
    scroll_text_enable(hd_scroll);
    /* the game's cursor over the HD text, not under it (MOUSE.C), as UW1 has it with the HD scroll */
    hd_cursor_enable(hd_scroll);
    /* the bitmap UI's strings (string_to_screen) as HD labels, as UW1's P12 */
    hd_label_enable(hd_scroll);
    /* cutscene subtitles (CUTS.C) in the HD font */
    caption_enable(hd_scroll);
    if (!hd_scroll) return 0;
    text_style_program("uw2port");
    snprintf(config, sizeof config, "%s/" PORT_CONFIG_FILE, home);
    if (text_style_boxes(home, locale, config, font_path, style, font_file, fallback_file, settings, sizeof settings))
        return -1;
    for (box = 0; box < 2; box++) {
        if (box && same_font() && style[1].antialiasing == style[0].antialiasing) {
            draw_font[1] = draw_font[0];
            continue;
        }
        result = hd_font_init(&draw_font[box], font_file[box]);
        if (!result && fallback_file[box][0]) result = hd_font_set_fallback(draw_font[box], fallback_file[box]);
        if (result) {
            fprintf(stderr, "uw2port: HD text: %s: %s\n", hd_font_error(result), font_file[box]);
            return -1;
        }
        hd_font_set_antialiasing(draw_font[box], style[box].antialiasing);
    }
    for (box = 0; box < 2; box++) {
        const HdTextStyle *t = &style[box];
        draw_style[box].border = (float)t->border;
        draw_style[box].baseline = (float)t->baseline;
        fprintf(stderr, "uw2port: HD text: %s: %s%s%s, size %g, spacing %g/%g/%g, baseline %g, border %g, %s\n",
                box ? "talk" : "message", font_file[box], fallback_file[box][0] ? " + " : "", fallback_file[box],
                t->size, t->letter_spacing, t->latin_spacing, t->line_spacing, t->baseline, t->border,
                t->antialiasing ? "antialiased" : "not antialiased");
    }
    if (settings[0]) fprintf(stderr, "uw2port: HD text: locale settings %s\n", settings);
    scroll_text_more_label("[\xe7\xb9\xbc\xe7\xba\x8c]");    /* [繼續], as UW1's */
    {
        /* the locale's words for what a conversation compares (CONVERSE.C's babl_ask), the
           home folder's first, else the current folder's */
        char aliases[1200];
        FILE *probe;
        snprintf(aliases, sizeof aliases, "%s/i18n/%s/input-aliases.tsv", home, locale);
        if ((probe = fopen(aliases, "rb")) != NULL) fclose(probe);
        else snprintf(aliases, sizeof aliases, "i18n/%s/input-aliases.tsv", locale);
        if (input_alias_load(aliases))
            fprintf(stderr, "uw2port: input aliases: %s not loaded; conversations take English words only\n", aliases);
        else
            fprintf(stderr, "uw2port: input aliases: %s\n", aliases);
        /* the proper names a conversation or book shows with their English the first time */
        snprintf(aliases, sizeof aliases, "%s/i18n/%s/proper-names.tsv", home, locale);
        if ((probe = fopen(aliases, "rb")) != NULL) fclose(probe);
        else snprintf(aliases, sizeof aliases, "i18n/%s/proper-names.tsv", locale);
        if (name_gloss_load(aliases))
            fprintf(stderr, "uw2port: proper names: %s not loaded; names are shown without their English\n", aliases);
        else
            fprintf(stderr, "uw2port: proper names: %s, %d names\n", aliases, name_gloss_count());
    }
    return 0;
}

/* Presentation thread only: the text and the palette are latched with the scan-out, so the
   overlay never reads the game's state. */
static ScrollFrame frame_text;
static HdCursor frame_cursor;
static HdLabelFrame frame_labels;
static CaptionFrame frame_caption;
static uint8_t frame_palette[768];
static uint8_t frame_pixels[640 * 480];   /* the scan-out, for the labels' signatures */
static int frame_w, frame_h;

static void text_scanout(uint8_t *pixels, int *w, int *h, uint8_t rgb6[768])
{
    vga_scanout(pixels, w, h, rgb6);
    hd_label_read(&frame_labels);
    if (frame_labels.count) {
        frame_w = *w; frame_h = *h;
        memcpy(frame_pixels, pixels, (size_t)*w * (size_t)*h);
    }
    hd_cursor_read(&frame_cursor);
    scroll_text_read(&frame_text);
    caption_read(&frame_caption);
    memcpy(frame_palette, rgb6, sizeof frame_palette);
}

static int text_draw(const PlatOverlay *frame, void *data)
{
    int drawn;
    (void)data;
    drawn = scroll_overlay_draw_styled(draw_font, draw_style, frame, &frame_text, frame_palette);
    /* cutscene subtitles: the message font, outlined for the moving picture under them */
    if (frame_caption.active)
        drawn |= caption_draw(draw_font[0], frame, &frame_caption, frame_palette,
                              draw_style[0].border > 0.6f ? draw_style[0].border : 0.6f);
    /* the bitmap UI's strings, each while the screen under it is as the game left it; the game's
       cursor, drawn by the HD layer, is no change to a label (a pixel's margin round its box) */
    if (frame_labels.count && frame_h == 200) {
        int cursor[4] = { frame_cursor.x - frame_cursor.hotspot_x - 1, frame_cursor.y - frame_cursor.hotspot_y - 2, 0, 0 };
        cursor[2] = cursor[0] + frame_cursor.width + 2;
        cursor[3] = cursor[1] + frame_cursor.height + 3;
        drawn |= hd_label_draw(draw_font[0], frame, &frame_labels, frame_pixels, frame_w, frame_h, frame_palette,
                               draw_style[0].border, draw_style[0].baseline, frame_cursor.visible ? cursor : NULL);
    }
    /* the cursor last, above the text; the settings screen has its own pointer */
    if (!settings_open()) drawn |= hd_cursor_draw(frame, &frame_cursor, frame_palette);
    return drawn;
}

void port_text_hooks(PlatHooks *hooks)
{
    if (!hd_scroll) return;
    hooks->scanout = text_scanout;
    hooks->overlay = text_draw;
    hooks->overlay_data = NULL;
}

static int measure(uint32_t cp, void *m) { return hd_scroll_advance(((Metrics *)m)->font, cp, ((Metrics *)m)->em64); }
static Metrics label_metrics, caption_metrics;
static const char *ordinal(const char *s);
unsigned char gr_read_pixel(int x, int y);      /* gfx/grcore.c: the drawing page, bottom-up */
int port_draw_page(void);                       /* gfx/grcore.c */
static int label_pixel(int x, int y) { return gr_read_pixel(x, 199 - y); }
static int to64(double v) { return (int)(v * 64 + (v < 0 ? -0.5 : 0.5)); }   /* rounded; no libm on Linux */
static void pause_ticks(int ticks, void *unused) { (void)unused; scroll_wait(ticks, 1); }

void port_text_begin(void)
{
    int box;
    if (port_i18n_init(locale, i18n_file) < 0)
        fprintf(stderr, "uw2port: i18n: catalog unavailable or invalid (%s); using English\n", i18n_file ? i18n_file : "none");
    if (port_i18n_qa_open(i18n_trace) < 0) fputs("uw2port: i18n: cannot open the lookup trace\n", stderr);
    if (!hd_scroll) return;
    for (box = 0; box < 2; box++) {
        if (box && same_font()) measure_font[1] = measure_font[0];
        else if (hd_font_init(&measure_font[box], font_file[box]) ||
                 (fallback_file[box][0] && hd_font_set_fallback(measure_font[box], fallback_file[box])))
            port_fatal("HD scroll: cannot initialize logical metrics font");
        metrics[box].font = measure_font[box];
        metrics[box].em64 = to64(style[box].size);
    }
    scroll_text_start(measure, &metrics[0], pause_ticks, NULL);
    for (box = 0; box < 2; box++) {
        scroll_text_measure_context(box, &metrics[box]);
        scroll_text_style(box, metrics[box].em64, to64(style[box].letter_spacing),
                          to64(style[box].latin_spacing), to64(style[box].line_spacing));
    }
    if (scroll_text_trace_open(scroll_log) < 0) port_fatal("HD scroll: cannot open the scroll log");
    /* the bitmap UI's strings in the message font at its size */
    caption_metrics.font = measure_font[0];
    caption_metrics.em64 = metrics[0].em64 * 7 / 5;    /* the subtitles larger, as UW1's */
    caption_start(measure, &caption_metrics, caption_metrics.em64);
    label_metrics = metrics[0];
    hd_label_start(measure, &label_metrics, label_metrics.em64, label_pixel, port_draw_page);
}

/* SCROLL.C's and the bitmap UI's strings: the translation of a string string_to_screen draws
   (gfx/grcore.c), or NULL to draw the English: the player's own words, a name get_name noted,
   a string just decoded (also when upper-cased since), or a piece the code prints by itself */
const char *port_ui_text(const char *english)
{
    const char *p, *zh;
    if (english && (zh = port_user_text(english)) != NULL) return zh;
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

void port_text_end(void)
{
    scroll_text_trace_close();
    if (measure_font[1] != measure_font[0]) hd_font_shutdown(measure_font[1]);
    hd_font_shutdown(measure_font[0]);
    measure_font[0] = measure_font[1] = NULL;
    port_i18n_shutdown();
}

/* MAINMENU.C: main menu button i (0..3) in Chinese over the menu's background, its colour the
   picture's letters' (the selected picture's when it is selected), centred on the screen as the
   column of buttons is; 1: the caller does not show the picture. x, y, w, h as the menu shows
   it (y bottom-up, the top row). As UW1's ui_text.c. */
int port_menu_button(int i, int x, int y, int w, int h, const unsigned char *img, int selected)
{
    static const char *const keys[] = { "menu.intro", "menu.create", "menu.credits", "menu.journey" };
    uint8_t dac[768];
    int colour, top = 199 - y, half;
    (void)selected;
    if (i < 0 || i > 3 || !img || !hd_label_enabled() || !port_i18n_enabled()) return 0;
    vga_get_dac(dac);
    if ((colour = hd_label_letter_colour(img, w * h, dac)) < 0) return 0;
    half = 0xA0 - x > x + w - 0xA0 ? 0xA0 - x : x + w - 0xA0;
    return hd_label_box(port_zh_lit(keys[i]), 0xA0 - half, top, 0xA0 + half, top + h, colour, HD_LABEL_CENTRE,
                        12 * 64, -1, 1.0f) == 0;
}

/* STATS.C's RedispStat: the words in the statistics page's art (STR .. EXP, one row each from the
   six rows from the top row 24, below the class's label, x 0xF0 to 0x110, left of the values)
   as HD patches over the art's background */
void port_stats_labels(void)
{
    static const char *const keys[] = { "stats.str", "stats.dex", "stats.int", "stats.vit", "stats.mana", "stats.exp" };
    int i;
    if (!hd_label_enabled() || !port_i18n_enabled()) return;
    for (i = 0; i < 6; i++)
        (void)hd_label_patch(port_zh_lit(keys[i]), 0xF0, 24 + i * 7, 0x110, 30 + i * 7, 0xC4, HD_LABEL_LEFT);
}

/* "12TH": a level and its English ordinal (STATS.C's sp_hdr), as the locale's words */
static const char *ordinal(const char *s)
{
    static char buf[32];
    int n = 0;
    const char *p = s;
    while (*p >= '0' && *p <= '9') n = n * 10 + (*p++ - '0');
    if (p == s || (strcmp(p, "ST") && strcmp(p, "ND") && strcmp(p, "RD") && strcmp(p, "TH"))) return NULL;
    snprintf(buf, sizeof buf, port_zh_lit("stats.level"), n);
    return buf;
}

int port_zh(void)
{
    return scroll_text_enabled() && port_i18n_enabled();
}
