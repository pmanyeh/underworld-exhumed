/* Use the current light source's own distance curve and colour maps. Only the
   shading slope and base change during drawing; vision radius, light sources and
   exploration stay as they were. Restore the grid shades before returning to play. */
#include "compat.h"
#include "player.h"
#include "sys.h"

static int16 saved_div, saved_base;
static unsigned char shades[17][33];
static unsigned char draw_shades[17][33];
static int16 cache_div, cache_base, cache_lowpass, cache_radius;
static int cached;
static int active;
static unsigned depth;

void port_bright_begin(void)
{
    int x, y;
    if (depth++) return;
    active = ENHANCED(ENH_BRIGHT_VIEW) && curvrad >= 0 && curvrad < 16;
    if (!active) return;
    saved_div = smooth_div; saved_base = smooth_base;
    for (y = 0; y < 17; ++y) for (x = 0; x < 33; ++x) shades[y][x] = glocs[y][x].shade;
    smooth_div = saved_div * 3 / 4;
    if (smooth_div < 1) smooth_div = 1;
    smooth_base = saved_base > 0 ? saved_base - 1 : saved_base;
    /* Rebuild only when the light changes, not hundreds of square roots per frame. */
    if (!cached || cache_div != smooth_div || cache_base != smooth_base ||
        cache_lowpass != smooth_lowpass || cache_radius != curvrad) {
        preset_grid(curvrad);
        for (y = 0; y < 17; ++y) for (x = 0; x < 33; ++x) draw_shades[y][x] = glocs[y][x].shade & 15;
        cache_div = smooth_div; cache_base = smooth_base;
        cache_lowpass = smooth_lowpass; cache_radius = curvrad; cached = 1;
    }
    for (y = 0; y < 17; ++y) for (x = 0; x < 33; ++x) {
        int original = shades[y][x] & 15, draw = draw_shades[y][x];
        /* Initialization and special views can retain a grid with another radius. */
        if (original == 15 || draw == 15) draw = original;
        glocs[y][x].shade = (shades[y][x] & 0xF0) | draw;
    }
}

void port_bright_end(void)
{
    int x, y;
    if (!depth || --depth) return;
    if (active) {
        smooth_div = saved_div; smooth_base = saved_base;
        /* Upper bits describe the faces of the freshly built frame, used by picking. */
        for (y = 0; y < 17; ++y) for (x = 0; x < 33; ++x)
            glocs[y][x].shade = (glocs[y][x].shade & 0xF0) | (shades[y][x] & 15);
    }
    active = 0;
}
