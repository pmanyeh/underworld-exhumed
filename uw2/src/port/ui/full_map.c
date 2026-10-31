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
