/* UW2's localized text (Phase 14). The catalog's translations reach only the HD scroll's
   display state (scroll_text.c's cells), never the game's strings or memory: a saved game
   and a replay are the same in every language. The modules are the runtime's, shared with
   UW1; this file only wires them to UW2's options, folders and game thread. */
#include "presentation/localized.h"
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
static int hd_scroll;
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

int port_text_setup(const char *home)
{
    char config[1200], settings[1200];
    int box, result;
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
    return 0;
}

/* Presentation thread only: the text and the palette are latched with the scan-out, so the
   overlay never reads the game's state. */
static ScrollFrame frame_text;
static HdCursor frame_cursor;
static uint8_t frame_palette[768];

static void text_scanout(uint8_t *pixels, int *w, int *h, uint8_t rgb6[768])
{
    vga_scanout(pixels, w, h, rgb6);
    hd_cursor_read(&frame_cursor);
    scroll_text_read(&frame_text);
    memcpy(frame_palette, rgb6, sizeof frame_palette);
}

static int text_draw(const PlatOverlay *frame, void *data)
{
    int drawn;
    (void)data;
    drawn = scroll_overlay_draw_styled(draw_font, draw_style, frame, &frame_text, frame_palette);
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
}

void port_text_end(void)
{
    scroll_text_trace_close();
    if (measure_font[1] != measure_font[0]) hd_font_shutdown(measure_font[1]);
    hd_font_shutdown(measure_font[0]);
    measure_font[0] = measure_font[1] = NULL;
    port_i18n_shutdown();
}

int port_zh(void)
{
    return scroll_text_enabled() && port_i18n_enabled();
}
