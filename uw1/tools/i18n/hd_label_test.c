/* HD labels (hd_label.c) with a fixed-width measure and a fake screen; no fonts, SDL or game. */
#include "presentation/hd_label.h"
#include <stdio.h>
#include <string.h>

static int failures, checks;
#define CHECK(c) do { checks++; if (!(c)) { failures++; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
static int measure(uint32_t cp, void *context) { (void)context; return cp < 0x80 ? 3 * 64 : 5 * 64; }
static uint8_t screen[320 * 200];
static int pixel(int x, int y) { return screen[y * 320 + x]; }

int main(void)
{
    HdLabelFrame f;
    int i;
    for (i = 0; i < 320 * 200; i++) screen[i] = (uint8_t)(i * 7);
    CHECK(hd_label_put("拉克希", 0x30, 0xC5, 40, 6, 0x65, 0, 319) == -1);   /* not enabled yet */
    hd_label_enable(1);
    hd_label_start(measure, NULL, 5 * 64, pixel);
    CHECK(hd_label_put("拉克希・長牙", 0x30, 0xC5, 64, 6, 0x65, 0, 319) == 0);
    hd_label_read(&f);
    CHECK(f.count == 1 && f.labels[0].count == 6 && f.labels[0].color == 0x65);
    CHECK(f.labels[0].cells[0].x64 == 0x30 * 64);                    /* left-aligned at x */
    CHECK(f.labels[0].top == 199 - 0xC5 && f.labels[0].bottom == 199 - 0xC5 + 6);
    CHECK(f.labels[0].left == 0x30 && f.labels[0].right == 0x30 + 64); /* the English box: wider than 30 */
    CHECK(hd_label_signature(screen, 320, 200, &f.labels[0]) == f.labels[0].signature);
    screen[(199 - 0xC5 + 2) * 320 + 0x31] ^= 1;                      /* the game draws in the box */
    CHECK(hd_label_signature(screen, 320, 200, &f.labels[0]) != f.labels[0].signature);
    screen[(199 - 0xC5 + 2) * 320 + 0x31] ^= 1;
    /* centred and right-aligned against the English */
    hd_label_align(HD_LABEL_CENTRE);
    CHECK(hd_label_put("力量", 100, 100, 20, 6, 1, 0, 319) == 0);
    hd_label_read(&f);
    CHECK(f.count == 2 && f.labels[1].cells[0].x64 == 110 * 64 - 5 * 64);
    hd_label_align(HD_LABEL_RIGHT);
    CHECK(hd_label_put("力量", 200, 100, 20, 6, 1, 0, 319) == 0);
    hd_label_read(&f);
    CHECK(f.labels[2].cells[1].x64 == 220 * 64 - 5 * 64);
    /* the alignment is for one put only */
    CHECK(hd_label_put("力量", 260, 100, 20, 6, 1, 0, 319) == 0);
    hd_label_read(&f);
    CHECK(f.labels[3].cells[0].x64 == 260 * 64);
    /* a put over another's box replaces it */
    CHECK(hd_label_put("智慧", 100, 100, 20, 6, 1, 0, 319) == 0);
    hd_label_read(&f);
    CHECK(f.count == 4 && f.labels[3].cells[0].scalar == 0x667A);
    /* too wide: shrinks to the clip, down to 3 pixels' em, else refused */
    CHECK(hd_label_put("一二三四五六七八", 10, 50, 10, 6, 1, 0, 39) == 0);
    hd_label_read(&f);
    CHECK(f.labels[f.count - 1].em64 == 5 * 64 * 30 / 40 && f.labels[f.count - 1].right <= 40);
    CHECK(hd_label_put("一二三四五六七八九十", 10, 40, 10, 6, 1, 0, 19) == -1);
    /* invalid text and newlines are refused */
    CHECK(hd_label_put("\xFF", 10, 30, 10, 6, 1, 0, 319) == -1);
    CHECK(hd_label_put("一\n二", 10, 30, 10, 6, 1, 0, 319) == -1);
    /* a patch covers its box with the box's commonest colour and centres its rows */
    hd_label_clear();
    for (i = 0; i < 7 * 30; i++) screen[(28 + i / 30) * 320 + 241 + i % 30] = (uint8_t)(i % 5 ? 0x4A : 0x01);
    CHECK(hd_label_patch("力量", 241, 28, 271, 35, 0xF1, HD_LABEL_LEFT) == 0);
    hd_label_read(&f);
    CHECK(f.count == 1 && f.labels[0].fill == 0x4A && f.labels[0].cells[0].x64 == 241 * 64);
    CHECK(f.labels[0].y64 == (28 + 35) * 32 - 5 * 32 && f.labels[0].right == 271);
    CHECK(hd_label_patch("力量", 241, 28, 241, 35, 0xF1, HD_LABEL_LEFT) == -1);
    /* a plain label has no fill */
    CHECK(hd_label_put("力量", 100, 100, 20, 6, 1, 0, 319) == 0);
    hd_label_read(&f);
    CHECK(f.labels[1].fill == -1);
    /* bounded: the oldest goes */
    hd_label_clear();
    for (i = 0; i < HD_LABELS + 5; i++) CHECK(hd_label_put("字", (i % 30) * 10, 190 - (i / 30) * 10, 5, 6, 1, 0, 319) == 0);
    hd_label_read(&f);
    CHECK(f.count == HD_LABELS && f.labels[HD_LABELS - 1].cells[0].x64 == ((HD_LABELS + 4) % 30) * 10 * 64);
    printf("%s %d assertions\n", failures ? "FAIL" : "PASS", checks);
    return failures != 0;
}
