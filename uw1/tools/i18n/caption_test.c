/* Cutscene subtitle layout (caption.c) with a fixed-width measure; no fonts, SDL or game. */
#include "presentation/caption.h"
#include <stdio.h>
#include <string.h>

static int failures, checks;
#define CHECK(c) do { checks++; if (!(c)) { failures++; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
static int measure(uint32_t cp, void *context) { (void)context; return cp < 0x80 ? 4 * 64 : 7 * 64; }

static int rows(const CaptionFrame *f)
{
    int i, n = 0, last = -1;
    for (i = 0; i < f->count; i++) if (f->cells[i].y64 != last) { n++; last = f->cells[i].y64; }
    return n;
}

int main(void)
{
    CaptionFrame f;
    char long_text[400] = "";
    int i;
    caption_enable(1);
    caption_start(measure, NULL, 7 * 64);
    CHECK(caption_set("你終於睡著了。", 0x2e, 0, 0xC7, 0x140, 0xC8) == 0);
    caption_read(&f);
    CHECK(f.active && f.count == 7 && rows(&f) == 1);
    CHECK(f.cells[0].x64 == 160 * 64 - 49 * 64 / 2);                 /* centred on the full screen */
    CHECK(f.cells[0].y64 + 7 * 64 * 13 / 10 == 197 * 64);            /* bottom-aligned */
    for (i = 0; i < 60; i++) strcat(long_text, "字");
    CHECK(caption_set(long_text, 0x2e, 0, 0xC7, 0x140, 0xC8) == 0);
    caption_read(&f);
    CHECK(rows(&f) == 2 && f.count == 60);                            /* 300 wide: 42 a line */
    for (i = 0; i < f.count; i++) CHECK(f.cells[i].x64 >= 10 * 64 && f.cells[i].x64 + 7 * 64 <= 310 * 64);
    long_text[0] = 0;
    for (i = 0; i < 42; i++) strcat(long_text, "字");
    strcat(long_text, "，下一行");
    CHECK(caption_set(long_text, 0x2e, 0, 0xC7, 0x140, 0xC8) == 0);
    caption_read(&f);
    for (i = 0; i < f.count; i++) if (f.cells[i].scalar == 0xFF0C) CHECK(f.cells[i].y64 == f.cells[0].y64);  /* no line starts with ， */
    CHECK(caption_set("一\n二\n三\n四\n五\n六\n七", 0x2e, 0, 0xC7, 0x140, 0xC8) == 0);
    caption_read(&f);
    CHECK(rows(&f) == 6);
    CHECK(caption_set("\xe4\xbd", 0x2e, 0, 0xC7, 0x140, 0xC8) == -1);  /* invalid UTF-8 */
    caption_clear();
    caption_read(&f);
    CHECK(!f.active && !caption_active());
    caption_enable(0);
    CHECK(caption_set("不顯示", 0x2e, 0, 0xC7, 0x140, 0xC8) == -1);
    printf("%d checks, %d failures\n", checks, failures);
    return failures != 0;
}
