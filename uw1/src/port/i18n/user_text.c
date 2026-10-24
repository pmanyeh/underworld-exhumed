#include "i18n/user_text.h"
#include "i18n/utf8.h"
#include <stdio.h>
#include <string.h>

FILE *bc_fopen(const char *path, const char *mode);   /* the game's files, by its own paths (compat.h) */
int bc_unlink(const char *path);

static char name_zh[96];
static char desc_zh[4][96];

int port_utf8_ascii(const char *s)
{
    for (; s && *s; s++) if ((unsigned char)*s >= 0x80) return 0;
    return 1;
}

static int valid(const char *s, size_t cap)
{
    size_t n = s ? strlen(s) : 0;
    return n > 0 && n < cap && utf8_validate(s, n, NULL) && !port_utf8_ascii(s);
}

void port_user_name_set(const char *zh)
{
    if (valid(zh, sizeof name_zh)) strcpy(name_zh, zh);
    else name_zh[0] = 0;
}
const char *port_user_name(void) { return name_zh[0] ? name_zh : NULL; }

/* a side file's one line: read into out (empty when missing or not UTF-8), or written */
static void read_line(const char *path, char *out, size_t cap)
{
    FILE *f = bc_fopen(path, "rb");
    size_t n = 0;
    out[0] = 0;
    if (!f) return;
    n = fread(out, 1, cap - 1, f);
    fclose(f);
    out[n] = 0;
    while (n && (out[n - 1] == '\n' || out[n - 1] == '\r')) out[--n] = 0;
    if (!valid(out, cap)) out[0] = 0;
}
static void write_line(const char *path, const char *text)
{
    FILE *f;
    if (!text || !*text) { bc_unlink(path); return; }
    if ((f = bc_fopen(path, "wb")) == NULL) return;
    fwrite(text, 1, strlen(text), f);
    fclose(f);
}

void port_user_name_save(void) { write_line("SAVE0\\NAME.UTF", name_zh); }
void port_user_name_load(void) { read_line("SAVE0\\NAME.UTF", name_zh, sizeof name_zh); }

void port_user_descs_load(void)
{
    char path[] = "SAVE1\\DESC.UTF";
    int i;
    for (i = 0; i < 4; i++) {
        path[4] = (char)('1' + i);
        read_line(path, desc_zh[i], sizeof desc_zh[i]);
    }
}
void port_user_desc_save(const char *zh) { write_line("SAVE0\\DESC.UTF", valid(zh, 96) ? zh : NULL); }

const char *port_user_desc_standin(int slot)
{
    static char text[4][8];
    if (slot < 1 || slot > 4) return "Save";
    snprintf(text[slot - 1], sizeof text[slot - 1], "Save %d", slot);
    return text[slot - 1];
}

const char *port_user_text(const char *english)
{
    int i;
    if (!english || !*english) return NULL;
    if (name_zh[0]) {
        const char *a = english, *b = USER_NAME_STANDIN;
        while (*a && *b && ((*a | 0x20) == (*b | 0x20))) a++, b++;
        if (!*a && !*b) return name_zh;
    }
    for (i = 0; i < 4; i++)
        if (desc_zh[i][0] && !strcmp(english, port_user_desc_standin(i + 1))) return desc_zh[i];
    return NULL;
}
