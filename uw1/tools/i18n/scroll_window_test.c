#define main generic_fixture_main
#include "../../../exhume/tools/tests/overlay/presentation.c"
#undef main
#include "presentation/scroll_overlay.h"
static uint8_t text_palette[768];
static ScrollFrame text_frame,original;
static PlatPointer click;
static int logical_measure(uint32_t cp,void *f) { return hd_scroll_advance(f,cp,5*64); }
static void no_wait(int ticks,void *context) { (void)ticks;(void)context; }
static void text_scanout(uint8_t *p,int *w,int *h,uint8_t pal[768])
{
    scanout(p,w,h,pal);pal[3*0x2e]=63;pal[3*0x2e + 1]=48;pal[3*0x2e + 2]=24;
    memcpy(text_palette,pal,768);scroll_text_read(&text_frame);
}
static int text_draw(const PlatOverlay *f,void *font) { return scroll_overlay_draw(font,f,&text_frame,text_palette); }
static void pointer(const PlatPointer *p) { if(p->type==PLAT_POINTER_DOWN)click=*p; }
static int inside(int x,int y)
{
    int box;
    for(box=0;box<2;box++) {
        const ScrollBox *b=&original.boxes[box];
        double lx=(x+.5-L.dst.x)*320/L.dst.w,ly=(y+.5-L.dst.y)*200/L.dst.h;
        if(lx>=b->left && lx<b->right && ly>=b->top && ly<b->bottom)return 1;
    }
    return 0;
}
int main(int argc,char **argv)
{
    PlatHooks h={0};HdFont *font=NULL,*metrics=NULL;int i;
    assert(argc==3 && hd_font_init(&font,argv[1])==0 && hd_font_init(&metrics,argv[1])==0);
    scroll_text_enable(1);scroll_text_start(logical_measure,metrics,no_wait,NULL);scroll_text_screen(4);
    scroll_text_box(0,15,169,306,199,8,0x2e);scroll_text_box(1,56,53,220,132,8,0x2e);
    assert(scroll_text_write(0,"1. 第一個選項 English 123\n",1,1)==0);
    assert(scroll_text_write(0,"2. 第二個選項\n",1,2)==0);
    assert(scroll_text_write(1,"你好，冒險者。這是沒有空白的繁體中文換行測試。「標點」和 longEnglishWordMustWrap 都能顯示。\n第二段。",1,0)==0);
    scroll_text_read(&original);
    h.scanout=text_scanout;h.overlay=text_draw;h.overlay_data=font;h.pointer=pointer;hooks=&h;
    assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS));
    L.win=SDL_CreateWindow("HD Chinese scroll layout",640,480,SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIGH_PIXEL_DENSITY);
    assert(L.win);g_win=L.win;L.ren=SDL_CreateRenderer(L.win,NULL);assert(L.ren);
    L.ltex=SDL_CreateTexture(L.ren,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_STREAMING,SET_W,SET_H);
    assert(L.ltex);SDL_SetTextureBlendMode(L.ltex,SDL_BLENDMODE_BLEND);
    L.frame_ns=1;L.start=SDL_GetTicks();live.scale=2;
    for(i=0;i<8;i++) {
        uint8_t *base,*painted;int w,hh,x,y,changed=0;float wx,wy;char path[2048];
        if(i<2) {SDL_SetWindowSize(L.win,i?960:320,i?720:200);live.aspect=i;live.integer_scale=1;}
        else if(i<6) {int j=i-2;SDL_SetWindowSize(L.win,j%2?853:640,j%2?517:480);live.aspect=j<2;live.integer_scale=j==0||j==2;}
        else if(i==6)assert(SDL_SetWindowFullscreen(L.win,true));
        else plat_set_display(0,2,1,1);
        assert(SDL_SyncWindow(L.win));SDL_PumpEvents();frame();frame();
        assert(SDL_GetCurrentRenderOutputSize(L.ren,&w,&hh));
        h.overlay=NULL;frame();base=capture();h.overlay=text_draw;frame();painted=capture();
        for(y=0;y<hh;y++)for(x=0;x<w;x++) {
            size_t off=((size_t)y*w+x)*3;
            if(memcmp(base+off,painted+off,3)) {changed++;assert(inside(x,y));}
        }
        assert(changed>20);free(base);
        assert(!memcmp(&text_frame,&original,sizeof original)); /* resizing never changes layout */
        snprintf(path,sizeof path,"%s-%d.png",argv[2],i);assert(plat_write_png_rgb(path,painted,w,hh)==0);free(painted);
        assert(SDL_RenderCoordinatesToWindow(L.ren,L.dst.x+20.5f*L.dst.w/320,L.dst.y+173.5f*L.dst.h/200,&wx,&wy));
        click.x=-1;plat_window_click(wx,wy,1);frame();assert(scroll_text_hit((int)click.x,(int)click.y)==1);
        shown=1;frame();painted=capture();
        x=(int)(L.dst.x+20.5f*L.dst.w/320);y=(int)(L.dst.y+173.5f*L.dst.h/200);
        rgb(painted,w,x,y,0,255,0);free(painted);shown=0;
        printf("PASS scroll window %d: %dx%d, %d changed pixels, fixed layout / choice hit / clipping / settings\n",i,w,hh,changed);
    }
    release();SDL_Quit();hd_font_shutdown(metrics);hd_font_shutdown(font);
    puts("PASS 8 FreeType/SDL scroll scenarios: 1x/3x, resize, letterbox, aspect, fullscreen, stable logical wrapping");return 0;
}
