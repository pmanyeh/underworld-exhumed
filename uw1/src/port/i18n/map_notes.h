/* UTF-8 map notes beside the original ASCII ATM records, never inside them. */
#ifndef UW1_MAP_NOTES_H
#define UW1_MAP_NOTES_H
#define MAP_NOTE_BYTES 160
#define MAP_NOTE_LIMIT 100
typedef struct MapNoteKey { char text[50]; int x, y; } MapNoteKey;
int map_notes_load(void);
int map_notes_save(void);
int map_notes_choose(const MapNoteKey *notes, int count);
void map_notes_marker(int id, char out[5]);
int map_notes_put(int id, int level, int x, int y, const char *text);
const char *map_notes_get(int level, const MapNoteKey *note);
int map_notes_remove(int level, const MapNoteKey *note);
void map_notes_reconcile(int level, const MapNoteKey *notes, int count);
#endif
