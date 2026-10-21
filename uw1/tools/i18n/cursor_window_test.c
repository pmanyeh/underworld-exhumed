/* Exercise the actual UW1 compositor and SDL backend, with synthetic scanout. */
#define main generic_fixture_main
#include "../../../exhume/tools/tests/overlay/presentation.c"
#undef main
#include "presentation/presentation.h"
#include "presentation/hd_cursor.h"
void vga_scanout(uint8_t *p,int *w,int *h,uint8_t pal[768]) {
    scanout(p,w,h,pal);pal[6]=63;pal[7]=0;pal[8]=0;
}
static PlatPointer received;
static void pointer(const PlatPointer *p) { if(p->type==PLAT_POINTER_DOWN)received=*p; }
int main(int argc,char **argv)
{
    PlatHooks h={0};PortPresentation demo={0};uint8_t bits[63];int i;
    assert(argc==3 && hd_font_init(&demo.demo.font,argv[1])==HD_OK);
    memset(bits,2,sizeof bits);hd_cursor_enable(1);demo.cursor=1;
    /* Anchor lies inside the opaque HD text panel, so red must cover text. */
    hd_cursor_publish(0x106c,80,60,3,4,7,9,1,0,bits);
    h.scanout=presentation_scanout;h.overlay=presentation_draw;h.overlay_data=&demo;
    h.pointer=pointer;hooks=&h;
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));
    L.win=SDL_CreateWindow("HD cursor resize/fullscreen",640,480,SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    assert(L.win);g_win=L.win;L.ren=SDL_CreateRenderer(L.win,NULL);assert(L.ren);
    L.ltex=SDL_CreateTexture(L.ren,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_STREAMING,SET_W,SET_H);
    assert(L.ltex);SDL_SetTextureBlendMode(L.ltex,SDL_BLENDMODE_BLEND);
    L.frame_ns=1;L.start=SDL_GetTicks();
    live.scale=2; /* settings must not temporarily resize this test's window */
    for(i=0;i<8;i++) {
        uint8_t *p;int w,hh,x,y;char path[2048];
        if(i<2) {
            SDL_SetWindowSize(L.win,i?960:320,i?720:200);
            live.aspect=i;live.integer_scale=1;
        } else if(i<6) {
            int j=i-2;
            SDL_SetWindowSize(L.win,j%2?853:640,j%2?517:480);
            live.aspect=j<2;live.integer_scale=j==0 || j==2;
        } else if(i==6) assert(SDL_SetWindowFullscreen(L.win,true));
        else plat_set_display(0,2,1,1);
        assert(SDL_SyncWindow(L.win));SDL_PumpEvents();frame();frame();
        assert(SDL_GetCurrentRenderOutputSize(L.ren,&w,&hh));
        x=(int)(L.dst.x+80.5f*L.dst.w/320);y=(int)(L.dst.y+60.5f*L.dst.h/200);
        p=capture();rgb(p,w,x,y,255,0,0);
        snprintf(path,sizeof path,"%s-%d.png",argv[2],i);
        assert(plat_write_png_rgb(path,p,w,hh)==0);free(p);
        /* A click at the visible hotspot goes through real window->logical mapping. */
        { float wx,wy;assert(SDL_RenderCoordinatesToWindow(L.ren,
            L.dst.x+80.5f*L.dst.w/320,L.dst.y+60.5f*L.dst.h/200,&wx,&wy));
          received.x=-1;plat_window_click(wx,wy,1); }
        frame();
        if((int)received.x!=80 || (int)received.y!=60)fprintf(stderr,"click %d: %.2f %.2f\n",i,received.x,received.y);
        assert((int)received.x==80 && (int)received.y==60);
        shown=1;frame();p=capture();rgb(p,w,x,y,0,255,0);free(p);shown=0;
        hd_cursor_hide();frame();p=capture();
        assert(!(p[((size_t)y*w+x)*3]==255 && p[((size_t)y*w+x)*3+1]==0));free(p);
        hd_cursor_publish(0x106c,80,60,3,4,7,9,1,0,bits);
        printf("PASS cursor window %d: %dx%d, viewport %.1f %.1f %.1f %.1f\n",i,w,hh,L.dst.x,L.dst.y,L.dst.w,L.dst.h);
    }
    release();SDL_Quit();hd_font_shutdown(demo.demo.font);
    puts("PASS 8 SDL cursor scenarios: 1x/3x click hotspot, HD text order, settings, hide, resize, fullscreen");return 0;
}
