/* Native UW1 catalog lookup. UTF-8 translations stay outside the legacy game's
   mutable 512-byte string ring. The game thread owns init/shutdown and trace;
   the backend/presentation thread never reads this module's mutable state. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "i18n/localization.h"

#define MAX_CATALOG (4u * 1024u * 1024u)
#define MAX_ENTRIES 4096u
#define MAX_TARGET 16384u

struct entry {
    unsigned block, index;
    const char *source, *target;
};

static unsigned char *image;
static struct entry *entries;
static unsigned count;
static FILE *trace;
static unsigned char traced[MAX_ENTRIES];

static unsigned word(const unsigned char *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static unsigned long dword(const unsigned char *p)
{
    return (unsigned long)word(p) | (unsigned long)word(p + 2) << 16;
}

static int static_key(unsigned block, unsigned index)
{
    return block && block <= 0x7FFF && block != 0x7C && block != 0x7D && index < 512;
}

/* Input-format validation only; text iteration/layout belongs to Phase 3.
   Reject NUL, overlong encodings, surrogates and values beyond U+10FFFF. */
static int valid_target(const unsigned char *p, size_t n)
{
    size_t i = 0;
    while (i < n) {
        unsigned c = p[i++], value, minimum, extra, j;
        if (!c) return 0;
        if (c < 0x80) continue;
        if (c >= 0xC2 && c <= 0xDF) { value = c & 31; minimum = 0x80; extra = 1; }
        else if (c >= 0xE0 && c <= 0xEF) { value = c & 15; minimum = 0x800; extra = 2; }
        else if (c >= 0xF0 && c <= 0xF4) { value = c & 7; minimum = 0x10000; extra = 3; }
        else return 0;
        if (n - i < extra) return 0;
        for (j = 0; j < extra; j++) {
            c = p[i++];
            if ((c & 0xC0) != 0x80) return 0;
            value = (value << 6) | (c & 63);
        }
        if (value < minimum || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) return 0;
    }
    return 1;
}

void port_i18n_shutdown(void)
{
    if (trace) fclose(trace);
    trace = NULL;
    free(entries);
    free(image);
    entries = NULL;
    image = NULL;
    count = 0;
    memset(traced, 0, sizeof traced);
}

int port_i18n_init(const char *locale, const char *path)
{
    FILE *f;
    long length;
    unsigned long n, j;
    size_t pos;
    port_i18n_shutdown();
    if (!locale || !strcmp(locale, "en")) return 0;
    if (strcmp(locale, "zh-TW") || !path) return -1;
    f = fopen(path, "rb");
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END) || (length = ftell(f)) < 12 ||
        length > (long)MAX_CATALOG || fseek(f, 0, SEEK_SET)) {
        fclose(f);
        return -1;
    }
    image = malloc((size_t)length);
    if (!image) { fclose(f); return -1; }
    if (fread(image, 1, (size_t)length, f) != (size_t)length) {
        fclose(f);
        goto bad;
    }
    fclose(f);
    if (memcmp(image, "UW1LOC1\0", 8)) goto bad;
    n = dword(image + 8);
    if (!n || n > MAX_ENTRIES) goto bad;
    entries = calloc((size_t)n, sizeof *entries);
    if (!entries) goto bad;
    pos = 12;
    for (j = 0; j < n; j++) {
        unsigned long source_len, target_len;
        struct entry *e = entries + j;
        if ((size_t)length - pos < 12) goto bad;
        e->block = word(image + pos);
        e->index = word(image + pos + 2);
        source_len = dword(image + pos + 4);
        target_len = dword(image + pos + 8);
        pos += 12;
        if (!static_key(e->block, e->index) || !source_len || source_len > 511 ||
            !target_len || target_len > MAX_TARGET) goto bad;
        if (j && (entries[j - 1].block > e->block ||
            (entries[j - 1].block == e->block && entries[j - 1].index >= e->index))) goto bad;
        if (source_len + target_len + 2 > (size_t)length - pos) goto bad;
        e->source = (const char *)image + pos;
        if (memchr(e->source, 0, (size_t)source_len) || image[pos + source_len]) goto bad;
        pos += (size_t)source_len + 1;
        e->target = (const char *)image + pos;
        if (!valid_target(image + pos, (size_t)target_len) || image[pos + target_len]) goto bad;
        pos += (size_t)target_len + 1;
    }
    if (pos != (size_t)length) goto bad;
    count = (unsigned)n;   /* Publish only after the whole catalog has passed. */
    return 0;
bad:
    port_i18n_shutdown();
    return -1;
}

int port_i18n_enabled(void) { return count != 0; }

static const struct entry *find(unsigned block, unsigned index, const char *fallback)
{
    unsigned low = 0, high = count;
    if (!fallback || !static_key(block, index)) return NULL;
    while (low < high) {
        unsigned middle = low + (high - low) / 2;
        const struct entry *e = entries + middle;
        if (e->block < block || (e->block == block && e->index < index)) low = middle + 1;
        else high = middle;
    }
    if (low < count && entries[low].block == block && entries[low].index == index &&
        !strcmp(entries[low].source, fallback)) return entries + low;
    return NULL;
}

const char *port_i18n_lookup(unsigned block, unsigned index, const char *fallback)
{
    const struct entry *e = find(block, index, fallback);
    return e ? e->target : fallback;
}

int port_i18n_trace_open(const char *path)
{
    if (trace) fclose(trace);
    trace = NULL;
    memset(traced, 0, sizeof traced);
    if (!path || !port_i18n_enabled()) return 0;
    trace = fopen(path, "wb");
    return trace ? 0 : -1;
}

static void json_text(FILE *f, const char *text)
{
    const unsigned char *p = (const unsigned char *)text;
    fputc('"', f);
    for (; *p; p++) {
        if (*p == '"' || *p == '\\') { fputc('\\', f); fputc(*p, f); }
        else if (*p < 32) fprintf(f, "\\u%04x", (unsigned)*p);
        else fputc(*p, f);
    }
    fputc('"', f);
}

char *port_i18n_observe(unsigned block, unsigned index, char *fallback)
{
    const char *translated = port_i18n_lookup(block, index, fallback);
    if (trace && translated != fallback) {
        const struct entry *e = find(block, index, fallback);
        unsigned slot = (unsigned)(e - entries);
        if (!traced[slot]) {
            traced[slot] = 1;
            fprintf(trace, "{\"block\":%u,\"index\":%u,\"text\":", block, index);
            json_text(trace, translated);
            fputs("}\n", trace);
            fflush(trace);  /* Window-close may end the process with the game parked. */
        }
    }
    return fallback;
}
