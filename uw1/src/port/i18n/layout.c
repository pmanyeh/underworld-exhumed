/* Basic Traditional Chinese / ASCII wrapping, with explicit kinsoku policy.
   All break decisions operate on decoded units; byte offsets are never cut. */
#include <stdlib.h>
#include <string.h>
#include "i18n/layout.h"
#include "i18n/utf8.h"

enum { BREAK_LEGAL = 1, BREAK_PREFERRED = 2 };

static unsigned character_flags(uint32_t c)
{
    unsigned flags = 0;
    if (c == ' ' || c == '\t') return LAYOUT_SPACE;
    /* Deliberately broad Chinese character-wrap ranges, not a Unicode
       character-property database. Includes supplementary CJK scalars. */
    if ((c >= 0x2E80 && c <= 0xA4CF) || (c >= 0xF900 && c <= 0xFAFF) ||
        (c >= 0xFF00 && c <= 0xFFEF) || (c >= 0x20000 && c <= 0x3FFFF)) flags |= LAYOUT_CJK;
    switch (c) {
    case '(': case '[': case '{':
    case 0xFF08: case 0x300A: case 0x300C: case 0x300E: case 0x3010:
    case 0x3008: case 0x3014: case 0xFF5B:
        flags |= LAYOUT_NO_END; break;
    case ',': case '.': case '!': case '?': case ';': case ':':
    case ')': case ']': case '}':
    case 0xFF0C: case 0x3002: case 0xFF01: case 0xFF1F: case 0xFF1B: case 0xFF1A: case 0x3001:
    case 0xFF09: case 0x300B: case 0x300D: case 0x300F: case 0x3011:
    case 0x3009: case 0x3015: case 0xFF5D:
        flags |= LAYOUT_NO_START; break;
    }
    return flags;
}

static int control(unsigned c)
{
    return c == 'p' || c == 'P' || c == 'm' || (c >= '0' && c <= '6');
}

static int visible(const struct layout_unit *u)
{
    return u->kind == LAYOUT_GLYPH && !(u->flags & LAYOUT_SPACE);
}

/* Precompute the nearest non-space glyphs at each boundary, ignoring controls.
   This keeps punctuation constraints intact across color/pause tokens/spaces. */
static void mark_breaks(struct text_layout *out)
{
    size_t i;
    unsigned right = 0, left = 0;
    for (i = out->unit_count; i > 0; i--) {
        struct layout_unit *u = out->units + i - 1;
        if (u->kind == LAYOUT_NEWLINE) right = 0;
        else if (visible(u)) right = u->flags;
        u->break_before = right;
    }
    for (i = 0; i < out->unit_count; i++) {
        struct layout_unit *u = out->units + i;
        right = u->break_before;
        u->break_before = 0;
        if (!(left & LAYOUT_NO_END) && !(right & LAYOUT_NO_START) &&
            (!i || out->units[i - 1].kind != LAYOUT_CONTROL)) {
            u->break_before = BREAK_LEGAL;
            if ((left | right) & LAYOUT_CJK || (i && (out->units[i - 1].flags & LAYOUT_SPACE)))
                u->break_before |= BREAK_PREFERRED;
        }
        if (u->kind == LAYOUT_NEWLINE) left = 0;
        else if (visible(u)) left = u->flags;
    }
}

static void emit(struct text_layout *out, size_t start, size_t end, int max_width)
{
    struct layout_line *line = out->lines + out->line_count++;
    size_t first = end, last = start, i;
    line->unit_start = start;
    line->unit_count = end - start;
    for (i = start; i < end; i++) if (visible(out->units + i)) {
        if (first == end) first = i;
        last = i;
    }
    for (i = start; i < end; i++) {
        struct layout_unit *u = out->units + i;
        if ((u->flags & LAYOUT_SPACE) && (first == end || i < first || i > last))
            u->flags |= LAYOUT_TRIMMED;
        else line->width += u->advance;
    }
    if (line->width > max_width) line->flags |= LAYOUT_OVERFLOW;
}

static void paragraph(struct text_layout *out, size_t start, size_t end, int max_width)
{
    size_t i = start, preferred = start, legal = start;
    int64_t width = 0, trailing = 0;
    int have_glyph = 0;
    while (i < end) {
        struct layout_unit *u = out->units + i;
        if (i > start && have_glyph && (u->break_before & BREAK_LEGAL)) {
            if (width - trailing > max_width) {
                emit(out, start, i, max_width);
                start = i;
                preferred = legal = start;
                width = trailing = 0;
                have_glyph = 0;
                continue;
            }
            legal = i;
            if (u->break_before & BREAK_PREFERRED) preferred = i;
        }
        if (visible(u) && width + u->advance > max_width && have_glyph && legal > start) {
            size_t split = preferred > start ? preferred : legal;
            emit(out, start, split, max_width);
            start = i = split;
            preferred = legal = start;
            width = trailing = 0;
            have_glyph = 0;
            continue;
        }
        if (visible(u)) {
            width += u->advance;
            trailing = 0;
            have_glyph = 1;
        } else if ((u->flags & LAYOUT_SPACE) && have_glyph) {
            width += u->advance;
            trailing += u->advance;
        }
        i++;
    }
    emit(out, start, end, max_width);
}

void layout_free(struct text_layout *out)
{
    if (!out) return;
    free(out->units);
    free(out->lines);
    memset(out, 0, sizeof *out);
}

int layout_text(const char *text, size_t length, int max_width,
                layout_measure_fn measure, void *context,
                struct text_layout *out, size_t *error_offset)
{
    struct text_layout result = {0};
    size_t pos = 0, bad = 0, start = 0, i;
    const char *nul;
    if (error_offset) *error_offset = 0;
    if (!out || out->units || out->lines || out->unit_count || out->line_count ||
        !measure || max_width <= 0 || (!text && length) || length > 1024u * 1024u)
        return LAYOUT_BAD_ARGUMENT;
    if (!utf8_validate(text, length, &bad)) {
        if (error_offset) *error_offset = bad;
        return LAYOUT_BAD_TEXT;
    }
    nul = length ? memchr(text, 0, length) : NULL;
    if (nul) {
        if (error_offset) *error_offset = (size_t)(nul - text);
        return LAYOUT_BAD_TEXT;
    }
    /* Also bounds total measured width to int64 even with INT_MAX advances. */
    result.units = length ? calloc(length, sizeof *result.units) : NULL;
    result.lines = calloc(length + 1, sizeof *result.lines);
    if ((length && !result.units) || !result.lines) { layout_free(&result); return LAYOUT_NO_MEMORY; }
    while (pos < length) {
        struct layout_unit *u = result.units + result.unit_count++;
        u->byte_offset = pos;
        if (text[pos] == '\\' && length - pos >= 2 && control((unsigned char)text[pos + 1])) {
            u->kind = LAYOUT_CONTROL;
            u->scalar = (unsigned char)text[pos + 1];
            pos += 2;
        } else if (text[pos] == '\n' || text[pos] == '\r') {
            u->kind = LAYOUT_NEWLINE;
            u->scalar = '\n';
            if (text[pos++] == '\r' && pos < length && text[pos] == '\n') pos++;
        } else {
            utf8_next(text, length, &pos, &u->scalar);  /* Already validated. */
            u->flags = character_flags(u->scalar);
            u->advance = measure(u->scalar, context);
            if (u->advance < 0) {
                if (error_offset) *error_offset = u->byte_offset;
                layout_free(&result);
                return LAYOUT_BAD_WIDTH;
            }
        }
        u->byte_length = pos - u->byte_offset;
    }
    mark_breaks(&result);
    for (i = 0; i <= result.unit_count; i++) {
        if (i == result.unit_count || result.units[i].kind == LAYOUT_NEWLINE) {
            paragraph(&result, start, i, max_width);
            if (i < result.unit_count) {
                result.lines[result.line_count - 1].unit_count++;
                result.lines[result.line_count - 1].flags |= LAYOUT_HARD_BREAK;
            }
            start = i + 1;
        }
    }
    *out = result;
    if (error_offset) *error_offset = length;
    return LAYOUT_OK;
}
