#include "presentation/combat_text.h"
#include "presentation/hd_font.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static int checks, failures, glyphs;
#define CHECK(c) do { checks++; if (!(c)) { failures++; printf("FAIL %d: %s\n", __LINE__, #c); } } while (0)
static unsigned char fb[65536], rgba[640 * 480 * 4];
static int color_present(int r, int g, int b)
{
    size_t i;
    for (i = 0; i < sizeof rgba; i += 4)
        if (rgba[i] == r && rgba[i+1] == g && rgba[i+2] == b && rgba[i+3]) return 1;
    return 0;
}
int hd_scroll_advance(HdFont *f, uint32_t cp, int em) { (void)f; (void)em; return cp < 128 ? 3 * 64 : 6 * 64; }
int hd_scroll_glyph_border(HdFont *f, const PlatOverlay *frame, uint32_t cp, float x, float y,
                           float em, const float clip[4], HdColor c, float border)
{
    (void)f; (void)frame; (void)cp; (void)em; (void)c; (void)border;
    CHECK(x >= clip[0] && x < clip[2]); CHECK(y >= clip[1] && y < clip[3]);
    glyphs++;
    return HD_OK;
}
static void frame(int n, int h, uint32_t now)
{
    combat_text_begin(fb, 52, 131, 172, 113, n, h, now);
    combat_text_owner(7);
    combat_text_write(fb + 2 + 50 * 174 + 40, 4);
    combat_text_owner(8);
    combat_text_write(fb + 2 + 70 * 174 + 80, 2);
}
int main(void)
{
    CombatFrame f;
    PlatOverlay overlay = {0};
    int i;
    combat_text_reset();
    combat_text_observe(7, 65, 30, 20);
    combat_text_observe(8, 66, 10, 20);
    combat_text_event(1, COMBAT_DAMAGE, 4, NULL, 100); /* player's damage excluded */
    combat_text_event(7, COMBAT_DAMAGE, 5, NULL, 100);
    combat_text_observe(7, 65, 25, 20);
    frame(1, 1, 120); combat_text_end(); combat_text_read(&f);
    CHECK(f.count == 2 && f.floats == 1);
    CHECK(f.left == 52 && f.top == 19 && f.right == 224 && f.bottom == 132);
    CHECK(f.actors[0].hp == 25 && f.actors[0].maximum == 30 && f.actors[0].hurt);
    CHECK(f.actors[0].left == 92 && f.actors[0].right == 96 && f.actors[0].top == 81);
    CHECK(f.actors[1].maximum == 20 && !f.actors[1].hurt);
    CHECK(f.events[0].amount == 5 && f.events[0].age == 20);
    /* A wall paints identical colour bytes: ownership still erases, no colour guessing. */
    frame(1, 1, 130); combat_text_owner(0); combat_text_write(fb + 2 + 50 * 174 + 40, 4);
    combat_text_end(); combat_text_read(&f);
    CHECK(f.count == 1 && f.actors[0].slot == 8 && f.floats == 0);
    /* A foreground creature overwrites part of another: bounds only include surviving pixels. */
    frame(1, 1, 140); combat_text_owner(8); combat_text_write(fb + 2 + 50 * 174 + 40, 2);
    combat_text_end(); combat_text_read(&f);
    CHECK(f.actors[0].left == 94 && f.actors[0].right == 96);
    combat_text_event(0, COMBAT_MISS, 0, "Miss", 145);
    combat_text_event(7, COMBAT_BLOCK, 0, "Block", 145);
    frame(1, 1, 150); combat_text_end(); combat_text_read(&f);
    CHECK(f.floats == 3 && !strcmp(f.events[2].text, "Block"));
    overlay.rgba = rgba; overlay.width = 640; overlay.height = 480; overlay.pitch = 640 * 4;
    overlay.logical_width = 320; overlay.logical_height = 200;
    overlay.viewport_x = 0; overlay.viewport_y = 0; overlay.viewport_w = 640; overlay.viewport_h = 480;
    CHECK(combat_text_draw((HdFont *)1, &overlay, &f, .6f));
    CHECK(glyphs > 0);
    /* Bars stay within the game view at aspect-corrected resolution. */
    for (i = 0; i < 640 * 480; i++) if (rgba[i * 4 + 3]) {
        CHECK(i % 640 >= 104 && i % 640 < 448);
        CHECK(i / 640 >= 45 && i / 640 < 317);
    }
    CHECK(color_present(85, 220, 90));
    f.numbers = 0; f.actors[0].hp = 10; memset(rgba, 0, sizeof rgba);
    combat_text_draw((HdFont *)1, &overlay, &f, .6f);
    CHECK(color_present(255, 205, 65));
    f.actors[0].hp = 5; memset(rgba, 0, sizeof rgba);
    combat_text_draw((HdFont *)1, &overlay, &f, .6f);
    CHECK(color_present(255, 70, 60));
    f.actors[0].hp = 0; memset(rgba, 0, sizeof rgba);
    CHECK(!combat_text_draw((HdFont *)1, &overlay, &f, .6f));
    CHECK(!color_present(255, 70, 60));
    frame(0, 1, 160); combat_text_end(); combat_text_read(&f);
    CHECK(!f.numbers && f.health && f.floats == 0);
    frame(1, 0, 170); combat_text_end(); combat_text_read(&f);
    CHECK(f.numbers && !f.health && f.floats == 3);
    frame(1, 1, 600); combat_text_end(); combat_text_read(&f); CHECK(f.floats == 0);
    for (i = 0; i < 100; i++) combat_text_event(7, COMBAT_DAMAGE, 1, NULL, 610);
    frame(1, 1, 620); combat_text_end(); combat_text_read(&f); CHECK(f.floats == COMBAT_FLOATS);
    combat_text_forget(7); combat_text_observe(7, 65, 10, 12);
    frame(1, 1, 630); combat_text_end(); combat_text_read(&f);
    CHECK(!f.actors[0].hurt && f.actors[0].maximum == 12 && f.floats == 0);
    combat_text_event(7, COMBAT_DAMAGE, 2, NULL, 640);
    combat_text_observe(7, 70, 15, 10); /* reused slot, different creature */
    frame(1, 1, 650); combat_text_end(); combat_text_read(&f);
    CHECK(!f.actors[0].hurt && f.actors[0].maximum == 15 && f.floats == 0);
    combat_text_event(0, COMBAT_MISS, 0, "Miss", 660);
    frame(1, 1, 100); combat_text_end(); combat_text_read(&f); CHECK(f.floats == 0);
    combat_text_hide(); combat_text_read(&f); CHECK(f.count == 0 && f.floats == 0);
    combat_text_reset(); frame(1, 1, 200); combat_text_end(); combat_text_read(&f); CHECK(f.count == 0);
    combat_text_begin(fb, 0, 0, 321, 200, 1, 1, 200);
    combat_text_write(fb + 65535, 100); combat_text_end(); combat_text_read(&f); CHECK(f.count == 0);
    printf("%s %d combat checks: ownership/occlusion, bounds, HP estimates, events, flags, lifecycle, drawing\n", failures ? "FAIL" : "PASS", checks);
    return failures != 0;
}
