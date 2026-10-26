/* Real wheel routing, stats scrolling and original arrows, without game assets. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define far
typedef int16_t int16;
#define PANEL_INV 0
#define PANEL_RUNES 1
#define PANEL_STATS 2
static unsigned char skill_top, fg, bg;
static unsigned char *foreground_color = &fg, *background_color = &bg;
static int RightPanel, OpenBag, bag_up, bag_down, redraws, hidden, released, bad, last_font;
static int16 cursor_x, cursor_y;
static struct { int cmd, x, y; } input, *inplist = &input;
static void mouse_getxy(int16 *x, int16 *y) { *x = cursor_x; *y = cursor_y; }
static void ScrollItemsDown(void) { bag_down++; }
static void ScrollItemsUp(void) { bag_up++; }
static void grfx_load_font(const char *s) { last_font = !strcmp(s, "font5x6i.sys") ? 1 : 2; }
static void mouse_hide(void) { hidden++; }
static void mouse_show(void) { hidden--; }
static void mouse_release(int n) { released += n; }
static void sp_skill(unsigned char row)
{
    if (row >= 6 || row + skill_top >= 20 || hidden != 1 || last_font != 1 || fg != 0x68 || bg != 0x68) bad++;
    redraws++;
}
#include "stats_wheel_impl.h"
static int checks;
#define CHECK(c) do { checks++; if (!(c)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void)
{
    int top, down;
    /* Every starting row and direction must match the real button handler. */
    for (top = 0; top <= 14; top++) for (down = 0; down <= 1; down++) {
        int expected, drawn;
        skill_top = (unsigned char)top; redraws = released = 0;
        inplist->x = down ? 40 : 10; inplist->y = 2;
        mous_in_stat(); expected = skill_top; drawn = redraws;
        CHECK(released == 1 && !hidden && last_font == 2);
        skill_top = (unsigned char)top; redraws = released = 0;
        port_stats_scroll(down);
        CHECK(skill_top == expected && redraws == drawn);
        CHECK(!released && !hidden && !bad);
        if (drawn) CHECK(last_font == 2);
    }
    /* Wheel hover includes the six rows and their arrows, excludes header and flasks. */
    RightPanel = PANEL_STATS; OpenBag = 1;
    {
        static const int points[][3] = {
            {240,82,1}, {315,129,1}, {280,110,1},
            {239,100,0}, {316,100,0}, {280,81,0}, {280,130,0}, {100,100,0}
        };
        unsigned i;
        for (i = 0; i < sizeof points / sizeof points[0]; i++) {
            cursor_x = points[i][0]; cursor_y = points[i][1]; skill_top = 5;
            port_key_wheel(1); CHECK(skill_top == 5 + points[i][2]);
            CHECK(!bag_up && !bag_down && !released);
        }
    }
    cursor_x = 280; cursor_y = 110;
    for (top = 0; top < 100; top++) port_key_wheel(1);
    CHECK(skill_top == 14 && !hidden && !released);
    for (top = 0; top < 100; top++) port_key_wheel(0);
    CHECK(skill_top == 0 && !hidden && !released);
    RightPanel = PANEL_INV;
    port_key_wheel(1); port_key_wheel(0); CHECK(bag_up == 1 && bag_down == 1);
    OpenBag = 0; port_key_wheel(1); CHECK(bag_down == 1 && !skill_top);
    RightPanel = PANEL_RUNES; port_key_wheel(0); CHECK(bag_up == 1 && !skill_top);
    CHECK(!bad && !hidden && !released);
    printf("PASS %d stats wheel checks: original arrows, bounds, hover, redraw and bags\n", checks);
    return 0;
}
