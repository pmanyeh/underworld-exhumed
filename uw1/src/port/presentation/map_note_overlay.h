#ifndef UW1_MAP_NOTE_OVERLAY_H
#define UW1_MAP_NOTE_OVERLAY_H
#include "presentation/hd_label.h"
#include "i18n/map_notes.h"
typedef struct MapNoteFrame { int count; HdLabel notes[MAP_NOTE_LIMIT]; } MapNoteFrame;
void map_note_overlay_begin(void);
int map_note_overlay_add(const char *text, int x, int y);
void map_note_overlay_publish(void);
void map_note_overlay_hide(void);
void map_note_overlay_read(MapNoteFrame *out);
struct HdFont;
struct PlatOverlay;
int map_note_overlay_draw(struct HdFont *, const struct PlatOverlay *, const MapNoteFrame *,
                           const unsigned char *pixels, int w, int h, const unsigned char *palette,
                           const int cursor[4]);
/* ASCII-only hooks from AUTOMAP.C; objects are original ATM records. */
void port_map_notes_loaded(int level, const void *notes, int count);
void port_map_note_begin(int level, const void *note, const void *notes, int count);
void port_map_note_end(int level, const void *note, const char *ascii);
int port_map_note_editing(void);
int port_map_note_draw(int level, const void *note);
int port_map_note_width(int level, const void *note, int fallback);
void port_map_note_delete(int level, const void *note);
void port_map_notes_save(int level, const void *notes, int count);
#endif
