/* Prove that the optional build has no link-time FreeType requirement. */
#include "presentation/hd_font.h"
#include <stdio.h>
#include <string.h>
int main(void)
{
    HdFont *f = NULL;
    HdTextMetrics m = {1,2,3,4};
    HdFontStats stats;
    HdColor c = {255,255,255,255};
    if (hd_font_init(&f,"absent.otf") != HD_UNAVAILABLE || f) return 1;
    if (hd_text_measure_native(f,"x",1,16,100,&m) != HD_UNAVAILABLE || m.width != 1) return 2;
    if (hd_text_measure_logical(f,NULL,"x",1,8,100,&m) != HD_UNAVAILABLE) return 3;
    if (hd_text_draw(f,NULL,"x",1,0,0,8,100,c) != HD_UNAVAILABLE) return 4;
    memset(&stats,255,sizeof stats);hd_font_stats(f,&stats);
    if (stats.entries || stats.bitmap_bytes || stats.rasterizations) return 5;
    hd_font_shutdown(NULL);
    puts("PASS optional build: no FreeType link; explicit HD_UNAVAILABLE; unchanged metrics");
    return 0;
}
