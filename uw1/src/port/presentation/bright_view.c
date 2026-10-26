/* Brighten only the drawing tables, not the vision grid or the player's light.
   Flat faces use these while building the database; textures and sprites use them
   during rasterization. Preserve row zero's colour effects and row 15's darkness. */
#include "compat.h"
#include "sys.h"
#include <string.h>

static unsigned char saved[4096];
static unsigned char *table;
static unsigned depth;

void port_bright_begin(void)
{
    unsigned row;
    if (depth++) return;
    table = ENHANCED(ENH_BRIGHT_VIEW) ? cLightTabs : NULL;
    if (!table) return;
    memcpy(saved, table, sizeof saved);
    for (row = 1; row < 15; ++row) memcpy(table + row * 256, saved, 256);
}

void port_bright_end(void)
{
    if (!depth || --depth) return;
    if (table) memcpy(table, saved, sizeof saved);
    table = NULL;
}
