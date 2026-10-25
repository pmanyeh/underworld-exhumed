/* Native-only cursor publication. One game-thread producer, presentation reader.
   Immutable copies cross an acquire/release lock; no borrowed game pointers.
   Enable once before the game thread starts; no runtime mode toggling. */
#ifndef UW1_HD_CURSOR_H
#define UW1_HD_CURSOR_H
#include <stdint.h>
struct PlatOverlay;
#define HD_CURSOR_MAX 255
/* Game headers use pack(1); keep the publication ABI identical in both TUs. */
#pragma pack(push, 8)
typedef struct HdCursor {
    int visible, picture, x, y, hotspot_x, hotspot_y, width, height, mode, path;
    int tracked, box[4];                 /* the actual raster cursor's save-under bounds */
    uint64_t generation;
    uint8_t pixels[HD_CURSOR_MAX * HD_CURSOR_MAX]; /* indexed; 0 transparent */
} HdCursor;
#pragma pack(pop)
void hd_cursor_enable(int on);
int hd_cursor_enabled(void);
void hd_cursor_publish(int picture, int x, int y, int hx, int hy,
                       int width, int height, int mode, int path, const uint8_t *pixels);
void hd_cursor_hide(void);
void hd_cursor_track_box(int x, int y, int hx, int hy, int width, int height);
void hd_cursor_read(HdCursor *out);
/* Optional diagnostics, opened/closed on the game thread only. */
int hd_cursor_trace_open(const char *path);
void hd_cursor_trace_close(void);
int hd_cursor_draw(const struct PlatOverlay *frame, const HdCursor *cursor, const uint8_t rgb6[768]);
#endif
