/* Native UW1 catalog lookup. UTF-8 translations stay outside the legacy game's
   mutable 512-byte string ring. The game thread owns init/shutdown and trace;
   the backend/presentation thread never reads this module's mutable state. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "i18n/localization.h"
#include "i18n/utf8.h"

/* Phase 9: sized for the whole game (10,984 PAK entries), not the PoC. */
#define MAX_CATALOG (16u * 1024u * 1024u)
#define MAX_ENTRIES 32768u
#define MAX_TARGET 16384u

struct entry {
    unsigned block, index;
    const char *source, *target;
};

static unsigned char *image;
static struct entry *entries;
static unsigned count;
static FILE *trace, *qa;
static unsigned char traced[MAX_ENTRIES];
static struct { const char *address, *target; int is_template; char source[512]; } display[8];
static unsigned display_next;

void port_i18n_forget(const char *legacy)
{
    unsigned i;
    for (i = 0; i < 8; i++) if (display[i].address == legacy) display[i].address = NULL;
}
static const char *provenance(const char *legacy, int is_template)
{
    unsigned i;
    for (i = 0; i < 8; i++)
        if (display[i].address == legacy && display[i].is_template == is_template && !strcmp(display[i].source, legacy))
            return display[i].target;
    return NULL;
}
const char *port_i18n_display(const char *legacy)
{
    const char *target;
    if (!legacy) return "";
    target = provenance(legacy, 0);
    return target ? target : legacy;
}
/* the same string, upper-cased in place since (GAMESTRN.C's seg039_3452_814; P12) */
const char *port_i18n_display_nocase(const char *legacy)
{
    unsigned i;
    if (!legacy) return NULL;
    for (i = 0; i < 8; i++)
        if (display[i].address == legacy && !display[i].is_template) {
            const char *a = display[i].source, *b = legacy;
            while (*a && *b && (*a | 0x20) == (*b | 0x20) && ((*a >= 'A' && *a <= 'Z') || (*a >= 'a' && *a <= 'z') || *a == *b)) a++, b++;
            if (!*a && !*b) return display[i].target;
        }
    return NULL;
}
const char *port_i18n_template(const char *legacy)
{
    return legacy ? provenance(legacy, 1) : NULL;
}

static struct { char english[96]; const char *translated; } names[8];
static unsigned names_next;
void port_i18n_note_name(const char *english, const char *translated)
{
    unsigned slot;
    if (!english || !translated || strlen(english) >= sizeof names[0].english) return;
    slot = names_next++ % 8;
    strcpy(names[slot].english, english);
    names[slot].translated = translated;
}
const char *port_i18n_named(const char *english)
{
    unsigned i;
    if (!english || !*english) return NULL;
    for (i = 0; i < 8; i++)
        if (names[i].translated && !strcmp(names[i].english, english)) return names[i].translated;
    return NULL;
}

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

/* Catalogs contain C strings: additionally reject embedded NUL. */
static int valid_target(const unsigned char *p, size_t n)
{
    return !memchr(p, 0, n) && utf8_validate((const char *)p, n, NULL);
}

void port_i18n_shutdown(void)
{
    memset(display, 0, sizeof display);
    display_next = 0;
    if (trace) fclose(trace);
    if (qa) fclose(qa);
    trace = qa = NULL;
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

/* The catalog entry for this key, whatever its source text. */
static const struct entry *locate(unsigned block, unsigned index)
{
    unsigned low = 0, high = count;
    if (!static_key(block, index)) return NULL;
    while (low < high) {
        unsigned middle = low + (high - low) / 2;
        const struct entry *e = entries + middle;
        if (e->block < block || (e->block == block && e->index < index)) low = middle + 1;
        else high = middle;
    }
    if (low < count && entries[low].block == block && entries[low].index == index) return entries + low;
    return NULL;
}

static const struct entry *find(unsigned block, unsigned index, const char *fallback)
{
    const struct entry *e = fallback ? locate(block, index) : NULL;
    return e && !strcmp(e->source, fallback) ? e : NULL;
}

const char *port_i18n_lookup(unsigned block, unsigned index, const char *fallback)
{
    const struct entry *e = find(block, index, fallback);
    return e ? e->target : fallback;
}

/* P12: a key's translation for a line composed outside the game's string ring; the catalog
   was checked against the game's data when it was built */
const char *port_i18n_static(unsigned block, unsigned index)
{
    const struct entry *e = image ? locate(block, index) : NULL;
    return e ? e->target : NULL;
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

/* Phase 9 QA trace: one JSONL record per lookup, in any locale (English
   records every static key as a miss). Tools aggregate repeated keys. */
int port_i18n_qa_open(const char *path)
{
    if (qa) fclose(qa);
    qa = NULL;
    if (!path) return 0;
    qa = fopen(path, "wb");
    return qa ? 0 : -1;
}

static void qa_event(const char *event, unsigned block, unsigned index)
{
    if (!qa) return;
    fprintf(qa, "{\"event\":\"%s\",\"block\":%u,\"index\":%u}\n", event, block, index);
    fflush(qa);  /* Window-close may end the process with the game parked. */
}

void port_i18n_note_dynamic(unsigned block, unsigned index)
{
    qa_event("dynamic", block, index);
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
    if (qa) {
        const struct entry *e = locate(block, index);
        qa_event(!fallback || !static_key(block, index) ? "invalid" : !e ? "miss" :
                 translated == fallback ? "drift" : strchr(translated, '@') ? "template" : "hit",
                 block, index);
    }
    port_i18n_forget(fallback);
    if (translated != fallback && strlen(fallback) < 512) {
        unsigned slot = display_next++ % 8;
        display[slot].address = fallback;
        display[slot].target = translated;
        display[slot].is_template = strchr(translated, '@') != NULL;
        strcpy(display[slot].source, fallback);
    }
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
