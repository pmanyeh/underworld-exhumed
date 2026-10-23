#include "input_alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int slot; char *token; char *alias; size_t len; } Alias;
static Alias *aliases;
static size_t count;

void input_alias_free(void)
{
    size_t i;
    for (i = 0; i < count; i++) { free(aliases[i].token); free(aliases[i].alias); }
    free(aliases);
    aliases = NULL;
    count = 0;
}

int input_alias_loaded(void) { return count > 0; }

static char *dup_range(const char *s, size_t n)
{
    char *d = malloc(n + 1);
    if (d) { memcpy(d, s, n); d[n] = 0; }
    return d;
}

static int add(int slot, const char *token, const char *alias, size_t len)
{
    Alias *grown;
    if (!len) return 0;
    grown = realloc(aliases, (count + 1) * sizeof *aliases);
    if (!grown) return -1;
    aliases = grown;
    aliases[count].slot = slot;
    aliases[count].token = dup_range(token, strlen(token));
    aliases[count].alias = dup_range(alias, len);
    aliases[count].len = len;
    if (!aliases[count].token || !aliases[count].alias) return -1;
    count++;
    return 0;
}

/* longest alias first, so 聖物 does not take the start of a longer word */
static int longer(const void *a, const void *b)
{
    const Alias *x = a, *y = b;
    return x->len < y->len ? 1 : x->len > y->len ? -1 : 0;
}

int input_alias_load(const char *path)
{
    char line[2048];
    FILE *f;
    int first = 1, bad = 0;
    input_alias_free();
    f = path ? fopen(path, "rb") : NULL;
    if (!f) return -1;
    while (fgets(line, sizeof line, f)) {
        char *field[6], *p = line, *a;
        int i;
        line[strcspn(line, "\r\n")] = 0;
        if (first) { first = 0; continue; }          /* the header */
        if (!line[0]) continue;
        for (i = 0; i < 6; i++) {
            field[i] = p;
            p = strchr(p, '\t');
            if (!p) break;
            *p++ = 0;
        }
        if (i != 5 || !field[4][0]) { bad = 1; break; }
        for (a = field[5]; *a; ) {
            size_t len = strcspn(a, "|");
            if (add(atoi(field[0]), field[4], a, len)) { bad = 1; break; }
            a += len;
            if (*a == '|') a++;
        }
        if (bad) break;
    }
    fclose(f);
    if (bad) { input_alias_free(); return -1; }
    qsort(aliases, count, sizeof *aliases, longer);
    return 0;
}

int input_alias_map(int slot, const char *text, char *out, size_t n)
{
    char work[1024];
    size_t w = 0, o = 0, i;
    int replaced = 0, space = 1;
    if (!out || !n) return 0;
    out[0] = 0;
    if (!text) return 0;
    while (*text && w < sizeof work - 1) {
        const Alias *hit = NULL;
        for (i = 0; i < count && !hit; i++)
            if (aliases[i].slot == slot && !strncmp(text, aliases[i].alias, aliases[i].len)) hit = &aliases[i];
        if (hit) {
            size_t t = strlen(hit->token);
            if (w + t + 2 >= sizeof work) break;
            work[w++] = ' ';
            memcpy(work + w, hit->token, t);
            w += t;
            work[w++] = ' ';
            text += hit->len;
            replaced++;
        } else {
            unsigned char c = (unsigned char)*text++;
            work[w++] = c < 0x80 ? (char)c : ' ';
            if (c >= 0x80) while ((*text & 0xC0) == 0x80) text++;   /* the rest of the character */
        }
    }
    work[w] = 0;
    for (i = 0; i < w && o < n - 1; i++) {
        char c = work[i];
        if (c == ' ' || c == '\t') { space = 1; continue; }
        if (space && o) { if (o >= n - 2) break; out[o++] = ' '; }
        space = 0;
        out[o++] = c;
    }
    out[o] = 0;
    return replaced;
}
