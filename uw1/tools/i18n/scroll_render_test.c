#include "presentation/scroll_overlay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int checks;
#define CHECK(c) do {checks++;if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);exit(1);}}while(0)
static int measure(uint32_t cp,void *font) { return hd_scroll_advance(font,cp,5*64); }
int main(int argc,char **argv)
{
    HdFont *font=NULL,*metrics=NULL;ScrollFrame text,unchanged;uint8_t palette[768]={0};int test;
    const int dims[][2]={{320,200},{960,720},{1001,733},{1600,1200}};
    CHECK(argc==4);(void)argv[2];CHECK(hd_font_init(&font,argv[1])==0 && hd_font_init(&metrics,argv[1])==0);
    CHECK(hd_scroll_advance(metrics,0x4f60,320)==400); /* 5 logical em, 5/4 horizontal room */
    CHECK(hd_scroll_advance(metrics,'i',320)<hd_scroll_advance(metrics,'W',320));
    scroll_text_enable(1);scroll_text_start(measure,metrics,NULL,NULL);scroll_text_screen(4);
    scroll_text_box(0,15,169,306,199,8,0x2e);scroll_text_box(1,56,53,220,132,8,0x2e);
    CHECK(scroll_text_write(0,"1. 中文選項 English 123\n2. 你好\n",1,1)==0);
    CHECK(scroll_text_write(1,"你好，冒險者。沒有空白的中文段落需要依照 logical pixel width 換行。「標點」不能任意拆開。",1,0)==0);
    scroll_text_read(&text);unchanged=text;
    palette[3*0x2e]=63;palette[3*0x2e + 1]=48;palette[3*0x2e + 2]=24;
    for(test=0;test<4;test++) {
        PlatOverlay f={0};uint8_t *allocation;size_t bytes;int x,y,changed=0;
        HdFontStats a,b;
        f.width=dims[test][0];f.height=dims[test][1];f.pitch=f.width*4+16;
        f.logical_width=320;f.logical_height=200;f.window_width=400;f.window_height=300;
        f.viewport_x=test==2?11.833333f:test==3?160:0;f.viewport_y=test==3?120:0;
        f.viewport_w=test==2?977.333333f:test==3?1280:(float)f.width;
        f.viewport_h=test==3?960:(float)f.height;
        bytes=(size_t)f.pitch*f.height;allocation=malloc(bytes+128);CHECK(allocation);
        memset(allocation,0xcd,bytes+128);f.rgba=allocation+64;
        for(y=0;y<f.height;y++)for(x=0;x<f.width;x++)memset(f.rgba+(size_t)y*f.pitch+x*4,0,4);
        CHECK(scroll_overlay_draw(font,&f,&text,palette));hd_font_stats(font,&a);
        CHECK(scroll_overlay_draw(font,&f,&text,palette));hd_font_stats(font,&b);
        CHECK(a.rasterizations==b.rasterizations && b.hits>a.hits);
        for(y=0;y<f.height;y++)for(x=0;x<f.width;x++) {
            const uint8_t *p=f.rgba+(size_t)y*f.pitch+x*4;
            if(p[3]) {
                int box,inside=0;
                double lx=(x+.5-f.viewport_x)*320/f.viewport_w,ly=(y+.5-f.viewport_y)*200/f.viewport_h;
                changed++;
                for(box=0;box<2;box++) {
                    const ScrollBox *s=&text.boxes[box];
                    if(lx>=s->left && lx<s->right && ly>=s->top && ly<s->bottom)inside=1;
                }
                CHECK(inside && p[0]==255 && p[1]>180 && p[2]>80);
            }
        }
        CHECK(changed>20);
        for(y=0;y<f.height;y++)for(x=f.width*4;x<f.pitch;x++)CHECK(f.rgba[(size_t)y*f.pitch+x]==0xcd);
        for(x=0;x<64;x++)CHECK(allocation[x]==0xcd && allocation[bytes+64+x]==0xcd);
        CHECK(!memcmp(&text,&unchanged,sizeof text));free(allocation);
    }
    {
        /* P10T: the styled draw, at 3x: an outline adds black pixels around the same glyphs,
           antialiasing off leaves no partly covered pixel, a box's em scales its glyphs */
        PlatOverlay f={0};HdFont *fonts[2]={font,font};
        const ScrollDrawStyle none[2]={{0,0},{0,0}},outline[2]={{.5f,0},{.5f,0}},lower[2]={{0,2},{0,2}};
        int top_plain=-1,top_lower=-1;
        int plain=0,inked=0,black=0,partial=0,i,n;uint8_t *px;
        f.width=960;f.height=600;f.pitch=f.width*4;f.logical_width=320;f.logical_height=200;
        f.window_width=960;f.window_height=600;f.viewport_w=960;f.viewport_h=600;
        px=calloc((size_t)f.pitch*f.height,1);CHECK(px);f.rgba=px;n=f.pitch*f.height;
        CHECK(scroll_overlay_draw_styled(fonts,none,&f,&text,palette));
        for(i=3;i<n;i+=4)if(px[i]) {plain++;if(top_plain<0)top_plain=i/f.pitch;}
        memset(px,0,(size_t)n);CHECK(scroll_overlay_draw_styled(fonts,lower,&f,&text,palette));
        for(i=3;i<n;i+=4)if(px[i]) {top_lower=i/f.pitch;break;}
        CHECK(top_lower>top_plain); /* the baseline moves the glyphs down, the box still clips them */
        memset(px,0,(size_t)n);CHECK(scroll_overlay_draw_styled(fonts,outline,&f,&text,palette));
        for(i=3;i<n;i+=4)if(px[i]) {inked++;if(!px[i-3] && !px[i-2] && !px[i-1])black++;}
        CHECK(inked>plain && black>0);
        CHECK(!memcmp(&text,&unchanged,sizeof text));
        hd_font_set_antialiasing(font,0);
        memset(px,0,(size_t)n);CHECK(scroll_overlay_draw_styled(fonts,none,&f,&text,palette));
        for(i=3;i<n;i+=4)if(px[i] && px[i]!=255)partial++;
        CHECK(partial==0);hd_font_set_antialiasing(font,1);
        memset(px,0,(size_t)n);CHECK(scroll_overlay_draw_styled(fonts,none,&f,&text,palette));
        for(i=3,partial=0;i<n;i+=4)if(px[i] && px[i]!=255)partial++;
        CHECK(partial>0);
        text.boxes[0].em64=text.boxes[1].em64=8*64;
        memset(px,0,(size_t)n);CHECK(scroll_overlay_draw_styled(fonts,none,&f,&text,palette));
        for(i=3,inked=0;i<n;i+=4)if(px[i])inked++;
        CHECK(inked>plain);
        free(px);
    }
    {
        /* P10T: a fallback font for what the font lacks (U+4E02, not in jf open huninn) */
        HdFont *fb=NULL;HdFontStats a,b;int filled;
        CHECK(hd_font_init(&fb,argv[1])==0);
        hd_font_stats(fb,&a);CHECK(hd_scroll_advance(fb,0x4e02,320)>=0);hd_font_stats(fb,&b);
        CHECK(b.missing_glyphs==a.missing_glyphs+1);
        CHECK(hd_font_set_fallback(fb,"absent.otf")==HD_FONT);
        CHECK(hd_font_set_fallback(fb,argv[3])==HD_OK);
        hd_font_stats(fb,&a);filled=hd_scroll_advance(fb,0x4e02,320);hd_font_stats(fb,&b);
        CHECK(filled>0 && b.missing_glyphs==a.missing_glyphs);
        CHECK(hd_scroll_advance(fb,0x4f60,320)==hd_scroll_advance(metrics,0x4f60,320)); /* the font's own glyphs unchanged */
        hd_font_shutdown(fb);
    }
    hd_font_shutdown(metrics);hd_font_shutdown(font);
    printf("PASS %d assertions: FreeType logical metrics, stable positions, native masks, warm cache, bounds/guards, 1x/3x/fractional/HiDPI, outline/baseline/antialiasing/em, fallback font\n",checks);
    return 0;
}
