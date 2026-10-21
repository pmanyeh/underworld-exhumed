/* Reuse Exhume's synthetic settings/scanout and pre-Present capture fixture;
   exercise UW1's real HD callback through the actual backend compositor. */
#define main generic_fixture_main
#include "../../../exhume/tools/tests/overlay/presentation.c"
#undef main
#include "presentation/hd_demo.h"

int main(int argc, char **argv)
{
    PlatHooks h = {0};
    HdDemo demo = {0};
    HdFontStats a, b;
    HdTextMetrics metrics;
    int i;
    assert(argc==3 && hd_font_init(&demo.font,argv[1])==HD_OK);
    h.scanout=scanout;h.overlay=hd_demo_draw;h.overlay_data=&demo;hooks=&h;
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));
    L.win=SDL_CreateWindow("FreeType HD text resize/fullscreen",640,480,SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    assert(L.win);g_win=L.win;
    L.ren=SDL_CreateRenderer(L.win,NULL);assert(L.ren);
    L.frame_ns=1;L.start=SDL_GetTicks();
    for(i=0;i<6;i++) {
        uint8_t *pixels;
        int w, hh;
        char path[2048];
        if(i<4) {
            SDL_SetWindowSize(L.win,i%2?853:640,i%2?517:480);
            live.aspect=i<2;live.integer_scale=i==0 || i==2;
        } else if(i==4) { assert(SDL_SetWindowFullscreen(L.win,true)); }
        else plat_set_display(0,2,1,1);
        assert(SDL_SyncWindow(L.win));SDL_PumpEvents();
        if(i==4)assert(SDL_GetWindowFlags(L.win) & SDL_WINDOW_FULLSCREEN);
        if(i==5)assert(!(SDL_GetWindowFlags(L.win) & SDL_WINDOW_FULLSCREEN));
        frame(); assert(!demo.reported);
        assert(SDL_GetCurrentRenderOutputSize(L.ren,&w,&hh));
        hd_font_stats(demo.font,&a);frame();hd_font_stats(demo.font,&b);
        assert(a.rasterizations==b.rasterizations && b.hits>a.hits); /* warm frames reuse masks */
        { PlatOverlay f={0};
          f.logical_width=320;f.logical_height=200;f.viewport_w=L.dst.w;f.viewport_h=L.dst.h;
          assert(hd_text_measure_logical(demo.font,&f,"你好，冒險者。",strlen("你好，冒險者。"),8,268,&metrics)==HD_OK);
          assert(metrics.pixel_size==(unsigned)(8*L.dst.h/200+.5f)); }
        pixels=capture();
        snprintf(path,sizeof path,"%s-%d.png",argv[2],i);
        assert(plat_write_png_rgb(path,pixels,w,hh)==0);free(pixels);
        printf("PASS window %d: %dx%d, viewport %.1f %.1f %.1f %.1f, ppem %u, rasters %llu\n",
               i,w,hh,L.dst.x,L.dst.y,L.dst.w,L.dst.h,metrics.pixel_size,(unsigned long long)b.rasterizations);
    }
    hd_font_stats(demo.font,&b);assert(!b.missing_glyphs);
    release();SDL_Quit();hd_font_shutdown(demo.font);
    puts("PASS 6 native-window HD text scenarios: resize, aspect, integer, fullscreen, cache reuse");
    return 0;
}
