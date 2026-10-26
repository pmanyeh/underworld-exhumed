#include "compat.h"
#include "ui.h"
#include "file.h"
#include "map.h"
#include "presentation/map_note_overlay.h"
#include "presentation/text_entry.h"
#include "i18n/phrases.h"
#include <string.h>
#include <stdio.h>

char far open_arc(struct Arc far *, char *);
char far close_arc(struct Arc far *);
char far put_arc(struct Arc far *, int, void far *, int);
static int pending_id, entry_open, removed_chinese;
static char pending_marker[5];
int port_map_note_editing(void) { return entry_open; }
static MapNoteKey keys[MAP_NOTE_LIMIT];
static MapNoteKey key_of(const struct ATM *n)
{
    MapNoteKey k;
    memcpy(k.text, n->text, sizeof k.text); k.text[sizeof k.text - 1] = 0;
    k.x = n->x; k.y = n->y; return k;
}
static int collect(const void *ptr, int count)
{
    const struct ATM *notes = ptr;
    int i;
    if (count > MAP_NOTE_LIMIT) count = MAP_NOTE_LIMIT;
    if (count < 0) count = 0;
    for (i = 0; i < count; i++) keys[i] = key_of(&notes[i]);
    return count;
}
void port_map_notes_loaded(int level, const void *notes, int count)
{
    removed_chinese = 0;
    map_notes_load();
    count = collect(notes, count);
    map_notes_reconcile(level, keys, count);
}
void port_map_note_begin(int level, const void *ptr, const void *notes, int count)
{
    const struct ATM *n = ptr;
    int max;
    (void)level;
    entry_open = pending_id = 0;
    if (!port_zh() || !hd_label_enabled()) return;
    count = collect(notes, count);
    pending_id = map_notes_choose(keys, count);
    if (!pending_id) return;
    map_notes_marker(pending_id, pending_marker);
    /* At the smallest readable size, leave four game pixels per Chinese character. */
    max = (316 - n->x) / 4;
    if (max > 46) max = 46;
    if (max < 1) return;
    text_entry_begin(TEXT_ENTRY_MAP, port_zh_lit("map.note_prompt"), "", max,
                      pending_marker, 28, 152, 265, 31);
    entry_open = text_entry_open();
}
void port_map_note_end(int level, const void *ptr, const char *ascii)
{
    const struct ATM *n = ptr;
    const char *zh;
    if (!entry_open) return;
    zh = text_entry_end(); entry_open = 0;
    if (zh && !strcmp(ascii, pending_marker) && map_notes_put(pending_id, level, n->x, n->y, zh))
        fputs("uw1port: map note: cannot keep the Chinese note\n", stderr);
}
int port_map_note_draw(int level, const void *ptr)
{
    const struct ATM *n = ptr;
    MapNoteKey k = key_of(n);
    const char *zh = map_notes_get(level, &k);
    if (!zh && port_zh()) zh = k.text;
    return zh && map_note_overlay_add(zh, n->x, n->y);
}
int port_map_note_width(int level, const void *ptr, int fallback)
{
    const struct ATM *n = ptr;
    MapNoteKey k = key_of(n);
    const char *zh = map_notes_get(level, &k);
    HdLabel l;
    if (!zh && port_zh()) zh = k.text;
    if (zh && !hd_label_build(&l, zh, n->x, n->y, 6, 0x2D, 315, 5 * 64)) return l.right - l.left;
    return fallback;
}
void port_map_note_delete(int level, const void *ptr)
{
    MapNoteKey k = key_of(ptr);
    if (map_notes_remove(level, &k)) removed_chinese = 1;
}
void port_map_notes_save(int level, const void *notes, int count)
{
    /* The original returns without writing when its last note is erased, so its old
       archive block survives. For a Chinese deletion, replace that block with one
       erased ATM record, which DOS already understands; a zero-length block aliases
       another LEV.ARK block (issue 7). ASCII-only sessions retain the original path. */
    if (!count && removed_chinese) {
        struct Arc arc;
        struct ATM empty;
        int ok;
        memset(&empty, 0, sizeof empty); empty.x = -1;
        if (!open_arc(&arc, "SAVE0\\lev.ark")) return;
        ok = put_arc(&arc, level + 0x23, &empty, sizeof empty);
        close_arc(&arc);
        if (!ok) return;
    }
    count = collect(notes, count);
    map_notes_reconcile(level, keys, count);
    if (map_notes_save()) fputs("uw1port: map note: cannot save NOTES.UTF\n", stderr);
    else removed_chinese = 0;
}
