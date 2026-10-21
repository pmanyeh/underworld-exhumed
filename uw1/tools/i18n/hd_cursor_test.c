#include "presentation/hd_cursor.h"
#include "plat.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <math.h>
#include <limits.h>
static int checks;
#define CHECK(c) do{checks++;if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);exit(1);}}while(0)
static atomic_int done;
static int writer(void *unused)
{
    int i; uint8_t bits[64];(void)unused;
    for(i=1;i<=50000;i++) {
        memset(bits,1+i%254,sizeof bits);
        hd_cursor_publish(i,i%320,i%200,3,4,8,8,i%16,1,bits);
    }
    atomic_store(&done,1);return 0;
}
static void pixels(int w,int h,float vx,float vy,float vw,float vh,int cx,int cy)
{
    PlatOverlay f={0};HdCursor c;
    uint8_t palette[768]={0},bitmap[63],*allocation;
    int x,y;size_t n;
    double sx=vw/320.0,sy=vh/200.0;
    f.width=w;f.height=h;f.pitch=w*4+16;f.logical_width=320;f.logical_height=200;
    f.viewport_x=vx;f.viewport_y=vy;f.viewport_w=vw;f.viewport_h=vh;
    f.window_width=400;f.window_height=300;
    n=(size_t)f.pitch*h;
    allocation=malloc(n+128);CHECK(allocation);memset(allocation,0xcd,n+128);f.rgba=allocation+64;
    for(y=0;y<9;y++)for(x=0;x<7;x++)bitmap[y*7+x]=(uint8_t)(x==3||y==4?2:(x+y)%3?1:0);
    palette[3]=63;palette[7]=63;
    hd_cursor_publish(0x106c,cx,cy,3,4,7,9,1,0,bitmap);
    hd_cursor_read(&c);memset(bitmap,0,sizeof bitmap); /* published pixels own a copy */
    for(y=0;y<h;y++)for(x=0;x<w;x++) {
        uint8_t *d=f.rgba+(size_t)y*f.pitch+x*4;d[0]=180;d[1]=20;d[2]=100;d[3]=128;
    }
    hd_cursor_draw(&f,&c,palette);
    for(y=0;y<h;y++)for(x=0;x<w;x++) {
        double lx=((double)x+.5-vx)/sx,ly=((double)y+.5-vy)/sy;
        const uint8_t *d=f.rgba+(size_t)y*f.pitch+x*4;
        int index=0;
        /* Reference: raster the source bitmap at logical pixel coordinates,
           then nearest-sample the whole game at the output pixel centre. */
        if(lx>=0&&ly>=0&&lx<320&&ly<200) {
            int gx=(int)lx-(cx-3),gy=(int)ly-(cy-4);
            if(gx>=0&&gy>=0&&gx<7&&gy<9)index=c.pixels[gy*7+gx];
        }
        if(index)CHECK(d[0]==(index==1?255:0)&&d[1]==(index==2?255:0)&&d[2]==0&&d[3]==255);
        else CHECK(d[0]==180&&d[1]==20&&d[2]==100&&d[3]==128);
    }
    for(y=0;y<h;y++)for(x=w*4;x<f.pitch;x++)CHECK(f.rgba[(size_t)y*f.pitch+x]==0xcd);
    for(x=0;x<64;x++)CHECK(allocation[x]==0xcd&&allocation[n+64+x]==0xcd);
    hd_cursor_hide();hd_cursor_read(&c);CHECK(!c.visible);
    free(allocation);
}
int main(void)
{
    HdCursor c;SDL_Thread *thread;uint64_t generation=0;int reads=0;
    hd_cursor_enable(1);
    pixels(320,200,0,0,320,200,100,90);
    pixels(960,720,0,0,960,720,100,90);
    pixels(1001,733,11.833333f,0,977.333333f,733,100,90);
    pixels(240,150,20,0,200,150,-1,-1);
    pixels(1600,1200,160,120,1280,960,319,199);
    /* An intentionally invalid publication hides, never reads an oversized mask. */
    hd_cursor_publish(1,0,0,0,0,256,1,0,0,(const uint8_t*)"x");hd_cursor_read(&c);CHECK(!c.visible);
    thread=SDL_CreateThread(writer,"cursor-producer",NULL);CHECK(thread);
    do {
        hd_cursor_read(&c);CHECK(c.generation>=generation);generation=c.generation;
        if(c.visible){
            int i;CHECK(c.width==8&&c.height==8&&c.x==c.picture%320&&c.y==c.picture%200&&c.mode==c.picture%16);
            for(i=0;i<64;i++)CHECK(c.pixels[i]==1+c.picture%254);
            reads++;
        }
    }while(!atomic_load(&done));
    SDL_WaitThread(thread,NULL);CHECK(reads>0);
    printf("PASS %d assertions: nearest pixels, hotspot, alpha/z-order, aspect/letterbox/HiDPI, clipping, 50000 concurrent snapshots\n",checks);
    return 0;
}
