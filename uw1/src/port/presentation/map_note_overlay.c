#include "map_note_overlay.h"
#include <stdatomic.h>
static MapNoteFrame work, published;
static atomic_flag busy = ATOMIC_FLAG_INIT;
static void lock(void) { while (atomic_flag_test_and_set_explicit(&busy, memory_order_acquire)) {} }
static void unlock(void) { atomic_flag_clear_explicit(&busy, memory_order_release); }
void map_note_overlay_begin(void) { work.count = 0; }
int map_note_overlay_add(const char *text, int x, int y)
{
    if (work.count >= MAP_NOTE_LIMIT || hd_label_build(&work.notes[work.count], text, x, y, 6, 0x2D, 315, 5 * 64)) return 0;
    work.count++; return 1;
}
void map_note_overlay_publish(void)
{
    int i;
    for (i = 0; i < work.count; i++) hd_label_refresh(&work.notes[i]);
    lock(); published = work; unlock();
}
void map_note_overlay_hide(void) { work.count = 0; map_note_overlay_publish(); }
void map_note_overlay_read(MapNoteFrame *out) { if (out) { lock(); *out = published; unlock(); } }
