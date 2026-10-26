/* Real UI translation routing and STATS.C wrapper, with the real HD label layout. */
#include "presentation/hd_label.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static int chinese, bitmap;
static int port_i18n_enabled(void) { return chinese; }
static const char *port_user_text(const char *s) { return s && !strcmp(s, "#NAME") ? "名字" : NULL; }
static const char *port_i18n_named(const char *s) { (void)s; return NULL; }
static const char *port_i18n_display(const char *s) { return s; }
static const char *port_i18n_display_nocase(const char *s) { return !strcmp(s, "FIGHTER") ? "戰士" : NULL; }
static const char *port_zh_literal(const char *s) { (void)s; return NULL; }
static const char *port_zh_lit(const char *s) { (void)s; return "第%d級"; }
static void string_to_screen(char *s, int x, int y);
#include "stats_ascii_impl.h"
static int measure(uint32_t cp, void *ctx) { (void)ctx; return (cp < 128 ? 3 : 5) * 64; }
static int pixel(int x, int y) { (void)x; (void)y; return 1; }
static void string_to_screen(char *s, int x, int y)
{
    const char *text = port_ui_text(s);
    if (text && !hd_label_put(text, x, y, (int)strlen(s) * 4, 6, 241, 0, 319)) return;
    hd_label_align(HD_LABEL_LEFT); bitmap++;
}
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void)
{
    HdLabelFrame f;
    hd_label_enable(1); hd_label_start(measure, NULL, 320, pixel, NULL);
    stats_string("31/33", 292, 150, HD_LABEL_RIGHT);
    CHECK(bitmap == 1); hd_label_read(&f); CHECK(!f.count);
    chinese = 1;
    CHECK(!port_ui_text("31/33") && !port_ui_text("PLAYER123"));
    stats_string("31/33", 292, 150, HD_LABEL_RIGHT);
    hd_label_read(&f); CHECK(f.count == 1 && f.labels[0].count == 5);
    CHECK(f.labels[0].cells[0].x64 == (312 - 15) * 64);
    CHECK(!port_ui_text("31/33")); /* scope ends with each draw */
    stats_string("9", 308, 150, HD_LABEL_RIGHT);
    hd_label_read(&f); CHECK(f.count == 1 && f.labels[0].count == 1);
    CHECK(f.labels[0].cells[0].x64 == 309 * 64); /* no stale digits on update */
    stats_string("PLAYER123", 250, 185, HD_LABEL_CENTRE);
    hd_label_read(&f); CHECK(f.count == 2 && f.labels[1].count == 9);
    CHECK(f.labels[1].cells[0].x64 == 250 * 64 + (36 - 27) * 32);
    stats_string("FIGHTER", 242, 178, HD_LABEL_LEFT);
    hd_label_read(&f); CHECK(f.labels[f.count - 1].cells[0].scalar == 0x6230);
    stats_string("12TH", 296, 178, HD_LABEL_RIGHT);
    hd_label_read(&f); CHECK(f.labels[f.count - 1].cells[0].scalar == 0x7B2C);
    CHECK(!strcmp(port_ui_text("#NAME"), "名字"));
    chinese = 0; CHECK(!port_ui_text("9") && !strcmp(port_ui_text("#NAME"), "名字"));
    hd_label_enable(0); chinese = 1;
    stats_string("123", 300, 136, HD_LABEL_RIGHT);
    CHECK(bitmap == 2 && !port_ui_text("123"));
    puts("PASS statistics ASCII: locale, scoped fallback, alignment, refresh and translations");
    return 0;
}
