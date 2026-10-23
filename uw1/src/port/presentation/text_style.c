/* text_style.c: the HD Chinese text parameters (text_style.h). */
#include "text_style.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const surface_name[TEXT_STYLE_SURFACES] = { "message", "talk", "book" };

void text_style_defaults(HdTextStyle *s)
{
    memset(s, 0, sizeof *s);
    snprintf(s->file, sizeof s->file, "%s", TEXT_STYLE_DEFAULT_FONT);
    s->size = 5;
    s->antialiasing = 1;
}

/* KEY's value in PATH, the last line that sets it, as the port's own settings file reads. */
static int get(const char *path, const char *key, char *out, size_t outsz)
{
    char line[1024];
    size_t kl = strlen(key);
    int found = -1;
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    while (fgets(line, sizeof line, f))
        if (!strncmp(line, key, kl) && line[kl] == '=') {
            line[strcspn(line, "\r\n")] = 0;
            if (strlen(line + kl + 1) < outsz) { strcpy(out, line + kl + 1); found = 0; }
        }
    fclose(f);
    return found;
}

static void key_of(char *key, size_t n, int surface, const char *item)
{
    if (surface < 0) snprintf(key, n, "font_cjk_hd_%s", item);
    else snprintf(key, n, "font_cjk_%s_%s", surface_name[surface], item);
}

static int number(const char *path, int surface, const char *item, double lo, double hi, double *v)
{
    char key[64], text[64], *end;
    double d;
    key_of(key, sizeof key, surface, item);
    if (get(path, key, text, sizeof text)) return 0;
    d = strtod(text, &end);
    while (*end == ' ' || *end == '\t') end++;
    if (end == text || *end || !isfinite(d) || d < lo || d > hi) {
        fprintf(stderr, "uw1port: %s: %s=%s is not a number from %g to %g; using %g\n", path, key, text, lo, hi, *v);
        return 0;
    }
    *v = d;
    return 1;
}

static int name(const char *path, int surface, const char *item, char *v, size_t n)
{
    char key[64], text[512];
    key_of(key, sizeof key, surface, item);
    if (get(path, key, text, sizeof text)) return 0;
    snprintf(v, n, "%s", text);
    return 1;
}

int text_style_load(const char *path, int surface, HdTextStyle *s)
{
    char key[64], text[64];
    int n = 0;
    if (!path || !*path || surface < -1 || surface >= TEXT_STYLE_SURFACES) return 0;
    if (name(path, surface, "file", s->file, sizeof s->file)) {
        if (!s->file[0]) {
            fprintf(stderr, "uw1port: %s: an empty font file; using %s\n", path, TEXT_STYLE_DEFAULT_FONT);
            snprintf(s->file, sizeof s->file, "%s", TEXT_STYLE_DEFAULT_FONT);
        } else n++;
    }
    n += name(path, surface, "fallback_file", s->fallback_file, sizeof s->fallback_file);
    n += number(path, surface, "size", 2, 16, &s->size);
    n += number(path, surface, "letter_spacing", -2, 8, &s->letter_spacing);
    n += number(path, surface, "latin_spacing", -2, 8, &s->latin_spacing);
    n += number(path, surface, "line_spacing", -3, 16, &s->line_spacing);
    n += number(path, surface, "baseline", -4, 4, &s->baseline);
    n += number(path, surface, "border", 0, 2, &s->border);
    key_of(key, sizeof key, surface, "antialiasing");
    if (!get(path, key, text, sizeof text)) {
        if (!strcmp(text, "true") || !strcmp(text, "1")) { s->antialiasing = 1; n++; }
        else if (!strcmp(text, "false") || !strcmp(text, "0")) { s->antialiasing = 0; n++; }
        else fprintf(stderr, "uw1port: %s: %s=%s is not true or false; using %s\n", path, key, text,
                     s->antialiasing ? "true" : "false");
    }
    return n;
}

static int readable(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f) fclose(f);
    return f != NULL;
}

int text_style_font_path(const char *home, const char *file, char *out, size_t outsz)
{
    const char *dirs[3];
    int i;
    if (!file || !*file) return -1;
    if (strpbrk(file, "/\\:")) {
        snprintf(out, outsz, "%s", file);
        return readable(out) ? 0 : -1;
    }
    dirs[0] = home; dirs[1] = "."; dirs[2] = "tools";
    for (i = 0; i < 3; i++) {
        if (!dirs[i]) continue;
        snprintf(out, outsz, "%s/fonts/%s", dirs[i], file);
        if (readable(out)) return 0;
    }
    snprintf(out, outsz, "%s", file);
    return -1;
}

void text_style_locale_file(const char *home, const char *locale, char *out, size_t outsz)
{
    if (home) {
        snprintf(out, outsz, "%s/i18n/%s/text-settings.ini", home, locale);
        if (readable(out)) return;
    }
    snprintf(out, outsz, "i18n/%s/text-settings.ini", locale);
    if (!readable(out)) out[0] = 0;
}
