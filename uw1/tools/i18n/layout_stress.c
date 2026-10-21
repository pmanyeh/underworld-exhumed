/* Asset-free sanitizer stress: exact-length heap input catches look-ahead
   overruns, and unit/line invariants check lossless byte coverage. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "i18n/layout.h"
#include "i18n/utf8.h"

static uint32_t seed = 303;
static uint32_t random32(void)
{
    seed = seed * 1664525u + 1013904223u;
    return seed;
}

static int measure(uint32_t scalar, void *context)
{
    (void)context;
    return scalar < 128 ? 1 : 2;
}

static void check(const char *text, size_t n, int width)
{
    struct text_layout out = {0};
    size_t error, i, byte = 0, unit = 0;
    int status = layout_text(text, n, width, measure, NULL, &out, &error);
    if (!utf8_validate(text, n, NULL) || (n && memchr(text, 0, n))) {
        assert(status == LAYOUT_BAD_TEXT);
        assert(!out.units && !out.lines);
        return;
    }
    assert(status == 0 && out.line_count > 0);
    for (i = 0; i < out.unit_count; i++) {
        assert(out.units[i].byte_offset == byte);
        assert(out.units[i].byte_length > 0);
        byte += out.units[i].byte_length;
        assert(byte <= n);
    }
    assert(byte == n);
    for (i = 0; i < out.line_count; i++) {
        struct layout_line *line = out.lines + i;
        size_t j;
        int64_t sum = 0;
        assert(line->unit_start == unit);
        unit += line->unit_count;
        assert(unit <= out.unit_count);
        for (j = line->unit_start; j < unit; j++)
            if (!(out.units[j].flags & LAYOUT_TRIMMED)) sum += out.units[j].advance;
        assert(line->width == sum);
        assert(!!(line->flags & LAYOUT_OVERFLOW) == (sum > width));
    }
    assert(unit == out.unit_count);
    layout_free(&out);
    layout_free(&out);
}

int main(void)
{
    static const char *const pieces[] = {"a", "W", " ", "\t", "\n", "\r\n", "\\1", "\\m",
        "\xE4\xB8\xAD", "\xE6\x96\x87", "\xEF\xBC\x8C", "\xEF\xBC\x88", "\xEF\xBC\x89",
        "\xF0\x9F\x98\x80", "\xF0\xA0\x80\x80"};
    unsigned trial;
    for (trial = 0; trial < 100000; trial++) {
        size_t i, n = random32() % 64;
        char *text = n ? malloc(n) : NULL;
        size_t pos = 0, count = 0;
        uint32_t scalar;
        assert(!n || text);
        for (i = 0; i < n; i++) text[i] = (char)(random32() >> 24);
        while (utf8_next(text, n, &pos, &scalar) == 1) count++;
        (void)count;
        for (i = 0; i <= n; i++) {
            size_t boundary = utf8_boundary_before(text, n, i);
            assert(boundary <= i);
        }
        pos = n;
        while (utf8_prev(text, n, &pos, &scalar) == 1) { }
        check(text, n, 1 + (int)(random32() % 40));
        free(text);
    }
    for (trial = 0; trial < 10000; trial++) {
        char scratch[1024];
        char *text;
        size_t i, n = 0, pieces_count = 1 + random32() % 128;
        for (i = 0; i < pieces_count; i++) {
            const char *piece = pieces[random32() % (sizeof pieces / sizeof pieces[0])];
            size_t len = strlen(piece);
            memcpy(scratch + n, piece, len);
            n += len;
        }
        text = malloc(n);
        assert(text);
        memcpy(text, scratch, n);
        check(text, n, 1 + (int)(random32() % 40));
        free(text);
    }
    puts("PASS: 100000 exact-length random-byte inputs + 10000 mixed-layout inputs; seed 303");
    return 0;
}
