/* Strict UTF-8 scalar decoding (RFC 3629); no replacement-byte recovery. */
#include "i18n/utf8.h"

int utf8_next(const char *text, size_t length, size_t *offset, uint32_t *scalar)
{
    size_t pos;
    uint32_t value, minimum;
    unsigned c, extra, j;
    if (!offset || !scalar || (!text && length) || *offset > length) return -1;
    pos = *offset;
    if (pos == length) return 0;
    c = (unsigned char)text[pos++];
    if (c < 0x80) { value = c; minimum = 0; extra = 0; }
    else if (c >= 0xC2 && c <= 0xDF) { value = c & 31; minimum = 0x80; extra = 1; }
    else if (c >= 0xE0 && c <= 0xEF) { value = c & 15; minimum = 0x800; extra = 2; }
    else if (c >= 0xF0 && c <= 0xF4) { value = c & 7; minimum = 0x10000; extra = 3; }
    else return -1;
    if (length - pos < extra) return -1;
    for (j = 0; j < extra; j++) {
        c = (unsigned char)text[pos++];
        if ((c & 0xC0) != 0x80) return -1;
        value = (value << 6) | (c & 63);
    }
    if (value < minimum || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) return -1;
    *offset = pos;
    *scalar = value;
    return 1;
}

int utf8_prev(const char *text, size_t length, size_t *offset, uint32_t *scalar)
{
    size_t start, end;
    uint32_t value;
    if (!offset || !scalar || (!text && length) || *offset > length) return -1;
    if (!*offset) return 0;
    start = *offset - 1;
    while (start && *offset - start < 4 && ((unsigned char)text[start] & 0xC0) == 0x80) start--;
    end = start;
    if (utf8_next(text, length, &end, &value) != 1 || end != *offset) return -1;
    *offset = start;
    *scalar = value;
    return 1;
}

int utf8_validate(const char *text, size_t length, size_t *error_offset)
{
    size_t pos = 0, previous = 0;
    uint32_t value;
    int status;
    while ((status = utf8_next(text, length, &pos, &value)) == 1) previous = pos;
    if (error_offset) *error_offset = status < 0 ? previous : length;
    return status == 0;
}

int utf8_count(const char *text, size_t length, size_t *count)
{
    size_t pos = 0, n = 0;
    uint32_t value;
    int status;
    if (!count) return 0;
    while ((status = utf8_next(text, length, &pos, &value)) == 1) n++;
    if (status < 0) return 0;
    *count = n;
    return 1;
}

size_t utf8_boundary_before(const char *text, size_t length, size_t offset)
{
    size_t pos = 0, boundary = 0, limit = offset < length ? offset : length;
    uint32_t value;
    while (pos < limit && utf8_next(text, length, &pos, &value) == 1) {
        if (pos > limit) break;
        boundary = pos;
    }
    return boundary;
}
