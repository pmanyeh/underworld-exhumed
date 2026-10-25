#include "i18n/map_notes.h"
#include "i18n/user_text.h"
#include "presentation/map_note_overlay.h"
#include "presentation/text_entry.h"
#include "presentation/hd_font.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
static int failures, checks, rename_fail;
#define CHECK(c) do { checks++; if (!(c)) { failures++; printf("FAIL %d: %s\n", __LINE__, #c); } } while (0)
static char dir[512];
static void path_of(const char *s, char *p)
{
    char flat[64]; size_t i;
    snprintf(flat, sizeof flat, "%s", s);
    for (i = 0; flat[i]; i++) if (flat[i] == '\\') flat[i] = '_';
    snprintf(p, 600, "%s/%s", dir, flat);
}
FILE *bc_fopen(const char *s, const char *mode) { char p[600]; path_of(s, p); return fopen(p, mode); }
int bc_unlink(const char *s) { char p[600]; path_of(s, p); return remove(p); }
int bc_rename(const char *a, const char *b)
{
    char p[600], q[600];
    if (rename_fail && !strcmp(a, "SAVE0\\NOTES.NEW")) return -1;
    path_of(a, p); path_of(b, q); return rename(p, q);
}
static void put(const char *s, const char *text) { FILE *f = bc_fopen(s, "wb"); fputs(text, f); fclose(f); }
static int exists(const char *s) { FILE *f = bc_fopen(s, "rb"); if (f) fclose(f); return f != NULL; }
static const char zh[] = "\xE5\x9C\xB0\xE5\x9C\x96"; /* two Chinese scalars */
static unsigned char screen[320 * 200];
static int pixel(int x, int y) { return screen[y * 320 + x]; }
static int measure(uint32_t cp, void *ctx) { (void)ctx; return (cp < 128 ? 3 : 6) * 64; }
static char input[64]; static unsigned char sent[512]; static int nsent, text_mode;
void plat_text_input(int on) { text_mode = on; }
size_t plat_text_take(char *out, size_t cap)
{
    if (!*input) return 0;
    snprintf(out, cap, "%s", input); input[0] = 0; return 1;
}
static void send(uint8_t sc) { if (nsent < (int)sizeof sent) sent[nsent++] = sc; }
int hd_text_draw(HdFont *f, const PlatOverlay *p, const char *s, size_t n, float x, float y, float em, float wrap, HdColor c)
{ (void)f; (void)p; (void)s; (void)n; (void)x; (void)y; (void)em; (void)wrap; (void)c; return HD_OK; }
int hd_text_measure_logical(HdFont *f, const PlatOverlay *p, const char *s, size_t n, float em, float w, HdTextMetrics *m)
{ (void)f; (void)p; (void)s; (void)n; (void)em; (void)w; memset(m, 0, sizeof *m); return HD_OK; }
int main(int argc, char **argv)
{
    MapNoteKey notes[3] = { { "#001", 40, 100 }, { "#002", 90, 100 }, { "PLAIN", 120, 100 } };
    static MapNoteFrame frame;
    HdLabel label;
    char marker[5], longtext[160]; int i;
    snprintf(dir, sizeof dir, "%s", argc > 1 ? argv[1] : ".");
    CHECK(!map_notes_load() && !map_notes_save() && !exists("SAVE0\\NOTES.UTF"));
    CHECK(map_notes_choose(notes, 3) == 3); /* token-looking ASCII notes are reserved too */
    CHECK(map_notes_put(1, 1, 40, 100, "plain") < 0);
    CHECK(map_notes_put(1, 0, 40, 100, zh) < 0 && map_notes_put(1, 1, -1, 100, zh) < 0);
    CHECK(map_notes_put(1, 1, 40, 100, "\xFF") < 0);
    CHECK(!map_notes_put(1, 1, 40, 100, zh));
    CHECK(map_notes_get(1, &notes[0]) && !strcmp(map_notes_get(1, &notes[0]), zh));
    CHECK(!map_notes_get(2, &notes[0])); notes[0].x++;
    CHECK(!map_notes_get(1, &notes[0])); notes[0].x--;
    CHECK(!map_notes_put(2, 2, 90, 100, zh) && !map_notes_save());
    CHECK(exists("SAVE0\\NOTES.UTF") && !exists("SAVE0\\NOTES.OLD"));
    CHECK(!map_notes_load() && map_notes_get(1, &notes[0]) && map_notes_get(2, &notes[1]));
    map_notes_reconcile(1, notes + 1, 2); /* deleted on level 1; level 2 survives */
    CHECK(!map_notes_get(1, &notes[0]) && map_notes_get(2, &notes[1]));
    CHECK(!map_notes_save());
    /* Failed replacement restores the previous complete file. */
    CHECK(!map_notes_put(3, 3, 120, 120, zh)); rename_fail = 1;
    CHECK(map_notes_save() < 0); rename_fail = 0;
    CHECK(!map_notes_load() && map_notes_get(2, &notes[1]));
    CHECK(map_notes_remove(2, &notes[1]) && !map_notes_save() && !exists("SAVE0\\NOTES.UTF"));
    put("SAVE0\\NOTES.UTF", "wrong header\n");
    CHECK(map_notes_load() < 0 && !map_notes_get(2, &notes[1]));
    put("SAVE0\\NOTES.UTF", "UW1NOTES\t1\n1\t1\t40\t100\t\xE5\x9C\xB0\n1\t2\t90\t100\t\xE5\x9C\x96\n2\t1\t9\t9\t\xFF\n");
    CHECK(!map_notes_load() && map_notes_get(1, &notes[0]) && !map_notes_get(2, &notes[1]));
    bc_unlink("SAVE0\\NOTES.UTF"); CHECK(!map_notes_load());
    /* No 64-label limit or overlap eviction for the map's 100 notes. */
    hd_label_enable(1); hd_label_start(measure, NULL, 5 * 64, pixel, NULL);
    map_note_overlay_begin();
    for (i = 0; i < MAP_NOTE_LIMIT; i++) CHECK(map_note_overlay_add(zh, 50, 100));
    CHECK(!map_note_overlay_add(zh, 50, 100));
    screen[99 * 320 + 50] = 1; /* raster drawing after notes were laid out */
    map_note_overlay_publish(); map_note_overlay_read(&frame);
    CHECK(frame.count == 100 && frame.notes[0].count == 2 && frame.notes[0].right == 62);
    CHECK(hd_label_shows(screen, 320, 200, &frame.notes[0], NULL));
    screen[99 * 320 + 50] = 2;
    CHECK(!hd_label_shows(screen, 320, 200, &frame.notes[0], NULL));
    map_note_overlay_hide(); map_note_overlay_read(&frame); CHECK(!frame.count);
    for (i = 0; i < 46; i++) memcpy(longtext + i * 3, zh, 3);
    longtext[138] = 0;
    CHECK(!hd_label_build(&label, longtext, 0, 100, 6, 45, 315, 5 * 64) && label.count == 46);
    CHECK(!hd_label_build(&label, zh, 297, 100, 6, 45, 315, 5 * 64) && label.right <= 316);
    /* The map editor captures Z, injects only an ASCII marker, and supports cancel/empty. */
    text_entry_enable(1, send); nsent = 0;
    text_entry_begin(TEXT_ENTRY_MAP, "note", "", 46, "#003", 20, 150, 270, 30);
    snprintf(input, sizeof input, "%sZ", zh);
    CHECK(text_entry_key(0x2C) && !nsent && text_entry_open() && text_mode);
    CHECK(text_entry_key(0x1C) && nsent && sent[nsent-2] == 0x1C);
    CHECK(text_entry_end() && !strcmp(text_entry_end(), "\xE5\x9C\xB0\xE5\x9C\x96Z"));
    for (i = 0; i < nsent; i++) CHECK(sent[i] != 0x1D); /* no wdialog Ctrl+A/Ctrl+K */
    text_entry_begin(TEXT_ENTRY_MAP, "note", "", 46, "#003", 20, 150, 270, 30);
    snprintf(input, sizeof input, "%s", zh); text_entry_key(0x2C); text_entry_key(0x01);
    CHECK(!text_entry_end() && !text_entry_open());
    nsent = 0; text_entry_begin(TEXT_ENTRY_MAP, "note", "", 46, "#003", 20, 150, 270, 30);
    text_entry_key(0x1C); CHECK(nsent == 2 && !text_entry_end());
    map_notes_marker(0xABC, marker); CHECK(!strcmp(marker, "#ABC"));
    bc_unlink("SAVE0\\NOTES.NEW");
    printf("%s %d map-note checks: sidecars, collisions, levels, recovery, 100 overlapping notes, input and cancellation\n", failures ? "FAIL" : "PASS", checks);
    return failures != 0;
}
