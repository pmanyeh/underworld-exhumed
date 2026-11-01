/* --enhance full-map (the settings screen's Cheats tab), as UW1's: the map screen draws the
   whole level, the tiles the player has not seen as the game's own Map Area (AUTOMAP.C's
   automap_area) would record them, without changing the player's map. PlayersMap is lent for
   the drawing only and put back at once, and the drawing's random patterns use a private seed,
   so the original map's random consumption is kept. Port only; called from AUTOMAP.C's
   ShowAutoMapLevel. */
#include <stdio.h>
#include <string.h>
#include "compat.h"
#include "map.h"
#include "level.h"
#include "player.h"
#include "file.h"
#include "object.h"
#include "sys.h"
#include "ui.h"
#include "portgame.h"

uint32_t port_rand_seed(void);
void port_set_rand_seed(uint32_t seed);
extern unsigned char PlayersMap[MAP_SIZE][MAP_SIZE];

static unsigned char saved[MAP_SIZE][MAP_SIZE];
static int active;
static uint32_t saved_seed;

/* level lev's tiles and its floors' terrain, from the save's LEV.ARK (else the game's DATA's):
   a level other than the one played, whose blocks are not in memory */
static int other_level(int lev, unsigned char out[MAP_SIZE][MAP_SIZE])
{
    static unsigned char block[0x7E08];
    int16 txm[0x46];
    uint16 terr[0x40];
    FILE *f;
    int i, x, y, n = 0, m = 0;
    if (open_arc(1, HomeDir)) {
        n = get_arc(1, lev - 1, (char far *)block);
        m = get_arc(1, lev + 0x4F, (char far *)txm);
        close_arc(1);
    }
    if ((n != (int)sizeof block || m != 0x86) && open_arc(1, "DATA\\")) {
        n = get_arc(1, lev - 1, (char far *)block);
        m = get_arc(1, lev + 0x4F, (char far *)txm);
        close_arc(1);
    }
    if (n != (int)sizeof block || m != 0x86)
        return 0;
    memset(terr, 0, sizeof terr);
    if ((f = fopen("DATA\\terrain.dat", "rb")) != NULL) {
        for (i = 0; i < 0x40; i++)
            if (fseek(f, (long)(uint16)txm[i] * 2, SEEK_SET) || fread(&terr[i], 2, 1, f) != 1)
                terr[i] = 0;
        fclose(f);
    }
    for (y = 1; y < 0x3F; y++)
        for (x = 1; x < 0x3F; x++) {
            const struct Tile *t = (const struct Tile *)block + x + y * MAP_SIZE;
            out[y][x] = (unsigned char)(t->type | (terr[t->floor] & TERR_CLASS));
        }
    return 1;
}

/* 1 when PlayersMap now holds level lev whole for drawing (port_full_map_end puts it back) */
int port_full_map_begin(int lev)
{
    static unsigned char whole[MAP_SIZE][MAP_SIZE];
    int x, y;
    active = 0;
    if (!ENHANCED(ENH_FULL_MAP) || lev < 1 || lev > NUM_LEVELS)
        return 0;
    memset(whole, 0, sizeof whole);
    if (lev == PlayerLevel) {
        /* the level played: its tiles as they are now, open doors and all */
        for (y = 1; y < 0x3F; y++)
            for (x = 1; x < 0x3F; x++) {
                struct Tile far *t = Map_GetAddr(x, y);
                whole[y][x] = (unsigned char)(t->type | (TxmTerr[t->floor] & TERR_CLASS));
            }
    } else if (!other_level(lev, whole))
        return 0;
    memcpy(saved, PlayersMap, sizeof saved);
    for (y = 1; y < 0x3F; y++)
        for (x = 1; x < 0x3F; x++)
            if (PlayersMap[y][x] == 0)          /* what the player has seen stays as it is */
                PlayersMap[y][x] = whole[y][x];
    saved_seed = port_rand_seed();
    active = 1;
    return 1;
}

void port_full_map_end(void)
{
    if (!active)
        return;
    memcpy(PlayersMap, saved, sizeof saved);
    port_set_rand_seed(saved_seed);
    active = 0;
}

/* Whether the map's arrows may turn to level lev: with full-map only the levels the game's
   LEV.ARK holds, so a world's eight slots stop at its last level (Britannia has five), and
   never 0x47, the map scraps' slot. Without full-map, every slot, as the original. */
int port_full_map_has(int lev)
{
    static signed char known[NUM_LEVELS + 1];   /* 0 not asked, 1 a level, -1 none */
    if (!ENHANCED(ENH_FULL_MAP))
        return 1;
    if (lev < 1 || lev > NUM_LEVELS || lev == 0x47)
        return 0;
    if (lev == PlayerLevel)
        return 1;
    if (!known[lev])
        known[lev] = check_arc(1, "DATA\\", lev - 1) > 0 ? 1 : -1;
    return known[lev] > 0;
}

/* The level the map's arrows turn to from lev, step +1 or -1 within its world: the next slot
   as the original, or with full-map the next real level, skipping gaps (the void has no 70);
   lev itself when there is none */
int port_full_map_step(int lev, int step)
{
    int next = lev;
    if (!ENHANCED(ENH_FULL_MAP))
        return lev + step;
    do {
        next += step;
        if (((next - 1) >> 3) != ((lev - 1) >> 3))
            return lev;
    } while (!port_full_map_has(next));
    return next;
}

/* lev, or with full-map the deepest real level of its world above it: the gem's facets keep
   the depth, which another world may not reach */
int port_full_map_depth(int lev)
{
    while ((lev & 7) != 1 && !port_full_map_has(lev))
        lev--;
    return lev;
}
