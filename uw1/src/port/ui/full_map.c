/* Drawing-only automap bytes. Never borrow the exploration array as workspace. */
#include "compat.h"
#include "map.h"
#include "player.h"
#include "ui.h"
#include "file.h"
#include <string.h>
#include <stdio.h>
static unsigned char drawing[MAP_TILES];
static int active;
static uint32_t saved_seed;
uint32_t port_rand_seed(void);
void port_set_rand_seed(uint32_t seed);
static unsigned long le32(const unsigned char *p) { return p[0] | (unsigned long)p[1]<<8 | (unsigned long)p[2]<<16 | (unsigned long)p[3]<<24; }
static int prefix(FILE *f, int block, void *out, int count)
{
    unsigned char b[4]; int n;
    rewind(f);
    if (fread(b, 1, 2, f) != 2) return 0;
    n = b[0] | b[1]<<8;
    if (n > 256 || block < 0 || block >= n || fseek(f, 2L + block*4L, SEEK_SET) || fread(b, 1, 4, f) != 4 || !le32(b)) return 0;
    return !fseek(f, (long)le32(b), SEEK_SET) && fread(out, 1, count, f) == (size_t)count;
}
int port_full_map_begin(int level)
{
    static unsigned char tiles[MAP_TILES*4], txm[TXM_BLOCK_SIZE], terr[1024];
    FILE *f; int i; unsigned floor, code;
    active = 0;
    if (!ENHANCED(ENH_FULL_MAP) || level < 1 || level > 9) return 0;
    /* The current map may include live doors or other changes not yet written to SAVE0. */
    if (level == PlayerLevel) memcpy(tiles, mapdata, sizeof tiles);
    f = fopen("SAVE0\\LEV.ARK", "rb");
    if (!f) return 0;
    if ((level != PlayerLevel && !prefix(f, level-1, tiles, sizeof tiles)) ||
        !prefix(f, level+0x11, txm, sizeof txm)) { fclose(f); return 0; }
    fclose(f);
    f = fopen("DATA\\TERRAIN.DAT", "rb");
    memset(terr, 0, sizeof terr);
    if (f) { fread(terr, 1, sizeof terr, f); fclose(f); }
    for (i = 0; i < MAP_TILES; i++) {
        unsigned v = tiles[i*4] | tiles[i*4+1]<<8;
        code = v & 15; floor = (v>>10)&15;
        if (floor < TXM_FLOORS) {
            unsigned texture = txm[(48+floor)*2] | txm[(48+floor)*2+1]<<8;
            if (texture < 256) code |= terr[512 + texture*2] & 0x30;
        }
        if (v & 0x8000) code |= 0x40;
        drawing[i] = (unsigned char)code;
    }
    saved_seed = port_rand_seed(); active = 1; return 1;
}
void port_full_map_end(void) { if (active) port_set_rand_seed(saved_seed); active = 0; }
int port_full_map_byte(int x, int y)
{
    if (x < 0 || x >= 64 || y < 0 || y >= 64) return 0;
    return active ? drawing[x+y*64] : PlayersMap[y][x];
}
