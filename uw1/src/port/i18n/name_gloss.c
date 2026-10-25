#include "name_gloss.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { char *zh, *english; size_t len; int seen; } Name;
static Name *names;
static int count;
static int mode = NAME_GLOSS_FIRST;

void name_gloss_free(void)
{
    int i;
    for (i = 0; i < count; i++) { free(names[i].zh); free(names[i].english); }
    free(names);
    names = NULL;
    count = 0;
}

int name_gloss_count(void) { return count; }
void name_gloss_mode(int m) { mode = m < NAME_GLOSS_OFF || m > NAME_GLOSS_EVERY ? NAME_GLOSS_FIRST : m; }

void name_gloss_reset(void)
{
    int i;
    for (i = 0; i < count; i++) names[i].seen = 0;
}

static char *dup(const char *s)
{
    char *d = malloc(strlen(s) + 1);
    if (d) strcpy(d, s);
    return d;
}

/* longest first, so 凱布勒斯爵士 is found before 凱布勒斯 */
static int longer(const void *a, const void *b)
{
    const Name *x = a, *y = b;
    return x->len < y->len ? 1 : x->len > y->len ? -1 : 0;
}

int name_gloss_load(const char *path)
{
    char line[512];
    FILE *f;
    int first = 1, bad = 0;
    name_gloss_free();
    f = path ? fopen(path, "rb") : NULL;
    if (!f) return -1;
    while (fgets(line, sizeof line, f)) {
        char *tab;
        Name *grown;
        line[strcspn(line, "\r\n")] = 0;
        if (first) { first = 0; continue; }          /* the header */
        if (!line[0]) continue;
        tab = strchr(line, '\t');
        if (!tab || tab == line || !tab[1] || strchr(tab + 1, '\t')) { bad = 1; break; }
        *tab = 0;
        grown = realloc(names, (size_t)(count + 1) * sizeof *names);
        if (!grown) { bad = 1; break; }
        names = grown;
        names[count].zh = dup(line);
        names[count].english = dup(tab + 1);
        names[count].len = strlen(line);
        names[count].seen = 0;
        if (!names[count].zh || !names[count].english) { free(names[count].zh); free(names[count].english); bad = 1; break; }
        count++;
    }
    fclose(f);
    if (bad) { name_gloss_free(); return -1; }
    qsort(names, (size_t)count, sizeof *names, longer);
    return 0;
}

/* inner is a whole word of outer: Cabirus in Sir Cabirus */
static int word_in(const char *inner, const char *outer)
{
    size_t n = strlen(inner);
    const char *p;
    for (p = strstr(outer, inner); p; p = strstr(p + 1, inner))
        if ((p == outer || p[-1] == ' ') && (p[n] == 0 || p[n] == ' ')) return 1;
    return 0;
}

/* glossed: the same person or place by a shorter name has had its English too */
static void mark_seen(const Name *glossed)
{
    int i;
    for (i = 0; i < count; i++)
        if (!names[i].seen && word_in(names[i].english, glossed->english)) names[i].seen = 1;
}

/* the English is already there: （ or ( right after the name */
static int bracketed(const char *after)
{
    return after[0] == '(' || !strncmp(after, "\xEF\xBC\x88", 3);
}

char *name_gloss_apply(const char *text)
{
    char *out = NULL;
    size_t cap = 0, used = 0, n;
    const char *p = text;
    int added = 0;
    if (!text || !count || mode == NAME_GLOSS_OFF) return NULL;
    while (*p) {
        Name *hit = NULL;
        int i;
        for (i = 0; i < count && !hit; i++)
            if (!strncmp(p, names[i].zh, names[i].len)) hit = &names[i];
        n = hit ? hit->len : 1;
        if (!hit) while ((p[n] & 0xC0) == 0x80) n++;   /* the rest of the character */
        if (used + n + (hit ? strlen(hit->english) + 7 : 0) + 1 > cap) {
            char *grown;
            cap = (used + n + 256) * 2;
            grown = realloc(out, cap);
            if (!grown) { free(out); return NULL; }
            out = grown;
        }
        memcpy(out + used, p, n);
        used += n;
        p += n;
        if (hit && (mode == NAME_GLOSS_EVERY || !hit->seen) && !bracketed(p)) {
            /* （English）, full-width brackets round the English */
            used += (size_t)sprintf(out + used, "\xEF\xBC\x88%s\xEF\xBC\x89", hit->english);
            added = 1;
        }
        if (hit && !hit->seen) { hit->seen = 1; mark_seen(hit); }
    }
    if (!added) { free(out); return NULL; }
    out[used] = 0;
    return out;
}
