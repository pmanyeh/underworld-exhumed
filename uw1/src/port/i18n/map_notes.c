#include "map_notes.h"
#include "i18n/utf8.h"
#include "user_text.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

FILE *bc_fopen(const char *, const char *);
int bc_unlink(const char *);
int bc_rename(const char *, const char *);
#define ROWS (9 * MAP_NOTE_LIMIT)
#define NOTES "SAVE0\\NOTES.UTF"
#define NEW "SAVE0\\NOTES.NEW"
#define OLD "SAVE0\\NOTES.OLD"
static struct { int id, level, x, y; char text[MAP_NOTE_BYTES]; } rows[ROWS];
static int dirty;
static int valid(const char *s)
{
    size_t n = s ? strlen(s) : 0, count, i;
    for (i = 0; i < n; i++) if ((unsigned char)s[i] < 32 || (unsigned char)s[i] == 127) return 0;
    return n && n < MAP_NOTE_BYTES && !port_utf8_ascii(s) && utf8_count(s, n, &count) && count <= 46 &&
           !strpbrk(s, "\t\r\n") && !strchr(s, '\x1B');
}
void map_notes_marker(int id, char out[5]) { snprintf(out, 5, "#%03X", id & 4095); }
static int id_of(const char *s)
{
    unsigned id = 0;
    int i;
    if (!s || strlen(s) != 4 || s[0] != '#') return 0;
    for (i = 1; i < 4; i++) {
        unsigned v = (unsigned char)s[i];
        if (v >= '0' && v <= '9') v -= '0';
        else if (v >= 'A' && v <= 'F') v = v - 'A' + 10;
        else return 0;
        id = id * 16 + v;
    }
    return (int)id;
}
static int find(int level, const MapNoteKey *n)
{
    int i, id = n ? id_of(n->text) : 0;
    if (!id) return -1;
    for (i = 0; i < ROWS; i++)
        if (rows[i].id == id && rows[i].level == level && rows[i].x == n->x && rows[i].y == n->y) return i;
    return -1;
}
const char *map_notes_get(int level, const MapNoteKey *n) { int i = find(level, n); return i < 0 ? NULL : rows[i].text; }
int map_notes_remove(int level, const MapNoteKey *n)
{
    int i = find(level, n);
    if (i < 0) return 0;
    memset(&rows[i], 0, sizeof rows[i]); dirty = 1; return 1;
}
int map_notes_choose(const MapNoteKey *notes, int count)
{
    int id, i, used;
    if (count < 0 || count > MAP_NOTE_LIMIT || (!notes && count)) return 0;
    for (id = 1; id < 4096; id++) {
        used = 0;
        for (i = 0; i < ROWS; i++) if (rows[i].id == id) { used = 1; break; }
        for (i = 0; !used && i < count; i++) if (id_of(notes[i].text) == id) used = 1;
        if (!used) return id;
    }
    return 0;
}
int map_notes_put(int id, int level, int x, int y, const char *text)
{
    int i, empty = -1;
    if (id < 1 || id > 4095 || level < 1 || level > 9 || x < 0 || x > 319 || y < 0 || y > 199 || !valid(text)) return -1;
    for (i = 0; i < ROWS; i++) {
        if (rows[i].id == id) { empty = i; break; }
        if (!rows[i].id && empty < 0) empty = i;
    }
    if (empty < 0) return -1;
    rows[empty].id = id; rows[empty].level = level; rows[empty].x = x; rows[empty].y = y;
    memmove(rows[empty].text, text, strlen(text) + 1); dirty = 1; return 0;
}
void map_notes_reconcile(int level, const MapNoteKey *notes, int count)
{
    int i, j;
    for (i = 0; i < ROWS; i++) if (rows[i].id && rows[i].level == level) {
        for (j = 0; j < count; j++)
            if (notes[j].x >= 0 && find(level, &notes[j]) == i) break;
        if (j == count) { memset(&rows[i], 0, sizeof rows[i]); dirty = 1; }
    }
}
static int number(char **p, int low, int high, int *out)
{
    char *end; long n;
    if (**p < '0' || **p > '9') return 0;
    errno = 0; n = strtol(*p, &end, 10);
    if (errno || *end != '\t' || n < low || n > high) return 0;
    *out = (int)n; *p = end + 1; return 1;
}
int map_notes_load(void)
{
    FILE *f = bc_fopen(NOTES, "rb");
    char line[256], *p; int id, level, x, y, i, count = 0;
    memset(rows, 0, sizeof rows); dirty = 0;
    if (!f) f = bc_fopen(OLD, "rb");       /* recover a write interrupted after the backup rename */
    if (!f) return 0;
    if (!fgets(line, sizeof line, f) || strcmp(line, "UW1NOTES\t1\n")) { fclose(f); return -1; }
    while (fgets(line, sizeof line, f)) {
        size_t len = strlen(line);
        if (!len || line[len-1] != '\n') { while (!feof(f) && fgetc(f) != '\n') {} continue; }
        line[--len] = 0;
        p = line;
        if (!number(&p, 1, 4095, &id) || !number(&p, 1, 9, &level) ||
            !number(&p, 0, 319, &x) || !number(&p, 0, 199, &y)) continue;
        for (i = 0; i < count; i++) if (rows[i].id == id) break;
        if (i != count || count >= ROWS || map_notes_put(id, level, x, y, p)) continue;
        count++;
    }
    fclose(f); dirty = 0; return 0;
}
static int exists(const char *p) { FILE *f = bc_fopen(p, "rb"); if (!f) return 0; fclose(f); return 1; }
int map_notes_save(void)
{
    FILE *f; int i, count = 0, had;
    if (!dirty) return 0;
    for (i = 0; i < ROWS; i++) count += rows[i].id != 0;
    if (!count) {
        if (exists(NOTES) && bc_unlink(NOTES)) return -1;
        bc_unlink(OLD); dirty = 0; return 0;
    }
    f = bc_fopen(NEW, "wb");
    if (!f) return -1;
    fputs("UW1NOTES\t1\n", f);
    for (i = 0; i < ROWS; i++) if (rows[i].id)
        fprintf(f, "%d\t%d\t%d\t%d\t%s\n", rows[i].id, rows[i].level, rows[i].x, rows[i].y, rows[i].text);
    i = ferror(f);
    if (fclose(f) || i) { bc_unlink(NEW); return -1; }
    had = exists(NOTES);
    if (had) {
        bc_unlink(OLD);
        if (bc_rename(NOTES, OLD)) { bc_unlink(NEW); return -1; }
    }
    if (bc_rename(NEW, NOTES)) { if (had) bc_rename(OLD, NOTES); return -1; }
    if (had) bc_unlink(OLD);
    dirty = 0; return 0;
}
