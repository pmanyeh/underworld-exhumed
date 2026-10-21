/* Public API checks against real FreeType and a redistributable CJK font.
   No game assets, SDL, game thread or reference screenshots are needed. */
#include "presentation/hd_font.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(c) do { checks++; if (!(c)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c); exit(1); } } while (0)
static int checks;
static const char text[] = "你好，冒險者。\nUltima Underworld 中文測試\nEnglish + 中文 123";
static const HdColor white = {255,240,190,255};

static void save(const PlatOverlay *p, const char *path)
{
    uint8_t *rgb = malloc((size_t)p->width * p->height * 3);
    int x, y, k;
    CHECK(rgb);
    for (y = 0; y < p->height; y++) for (x = 0; x < p->width; x++) {
        const uint8_t *s = p->rgba + (size_t)y * p->pitch + x * 4;
        for (k = 0; k < 3; k++) rgb[((size_t)y * p->width + x) * 3 + k] =
            (uint8_t)((s[k] * s[3] + 18 * (255 - s[3]) + 127) / 255);
    }
    CHECK(plat_write_png_rgb(path, rgb, p->width, p->height) == 0);
    free(rgb);
}
int main(int argc, char **argv)
{
    HdFont *f = NULL, *other = NULL;
    HdFontStats a, b;
    HdTextMetrics n, logical, unchanged = {123,456,7,8};
    PlatOverlay p = {0};
    uint8_t *first, *guard;
    size_t bytes, i;
    unsigned partial = 0, visible = 0;
    int row, col, k;
    CHECK(argc == 3);
    CHECK(hd_font_init(&f,"this-font-does-not-exist.otf") == HD_FONT && !f);
    CHECK(hd_font_init(NULL,argv[1]) == HD_ARGUMENT);
    CHECK(hd_font_init(&f,argv[1]) == HD_OK && f);
    CHECK(hd_font_init(&f,argv[1]) == HD_ARGUMENT);
    p.width = 1001; p.height = 733; p.pitch = p.width * 4;
    p.window_width = 500; p.window_height = 366;
    p.logical_width = 320; p.logical_height = 200;
    p.viewport_x = 20.5f; p.viewport_y = 6.5f; p.viewport_w = 960; p.viewport_h = 720;
    bytes = (size_t)p.pitch * p.height;
    p.rgba = calloc(1,bytes); first = malloc(bytes); CHECK(p.rgba && first);
    CHECK(hd_text_measure_logical(f,&p,text,strlen(text),8,268,&logical) == HD_OK);
    CHECK(logical.pixel_size == 29 && logical.lines == 3);
    CHECK(hd_text_measure_native(f,text,strlen(text),29,804,&n) == HD_OK);
    CHECK(fabsf(logical.width * 3 - n.width) < .01f);
    CHECK(fabsf(logical.height * 3.6f - n.height) < .01f);
    hd_font_stats(f,&a); CHECK(a.rasterizations > 20 && a.missing_glyphs == 0);
    CHECK(hd_text_draw(f,&p,text,strlen(text),10,10,8,268,white) == HD_OK);
    hd_font_stats(f,&b); CHECK(a.rasterizations == b.rasterizations && b.hits > a.hits);
    memcpy(first,p.rgba,bytes);
    for (i = 0; i < bytes; i += 4) if (first[i+3]) {
        visible++; if (first[i+3] < 255) partial++;
        CHECK(first[i] == 255 && first[i+1] == 240 && first[i+2] == 190);
    }
    CHECK(visible > 1000 && partial > 100); /* grayscale anti-aliasing */
    memset(p.rgba,0,bytes);
    CHECK(hd_text_draw(f,&p,text,strlen(text),10,10,8,268,white) == HD_OK);
    CHECK(!memcmp(first,p.rgba,bytes));
    save(&p,argv[2]);
    /* One game unit moves exactly three output pixels with this X scale.
       Compare the complete image, not a copy of the coordinate formula. */
    memset(p.rgba,0,bytes);
    CHECK(hd_text_draw(f,&p,text,strlen(text),11,10,8,268,white)==HD_OK);
    for(row=0;row<p.height;row++)for(col=0;col<p.width;col++) {
        static const uint8_t zero[4]={0};
        const uint8_t *expected=col>=3?first+(size_t)row*p.pitch+(col-3)*4:zero;
        CHECK(!memcmp(p.rgba+(size_t)row*p.pitch+col*4,expected,4));
    }
    memset(p.rgba,0,bytes);
    { HdColor half={255,240,190,128};
      CHECK(hd_text_draw(f,&p,text,strlen(text),10,10,8,268,half)==HD_OK); }
    for(i=0;i<bytes;i+=4)CHECK(abs(p.rgba[i+3]-(int)((first[i+3]*128u+127)/255))<=1);
    /* Compositing over an existing translucent layer: compare against the
       independently captured mask alpha; RGB must stay straight, not premultiplied. */
    for (i = 0; i < bytes; i += 4) { p.rgba[i]=10; p.rgba[i+1]=30; p.rgba[i+2]=70; p.rgba[i+3]=90; }
    CHECK(hd_text_draw(f,&p,text,strlen(text),10,10,8,268,white) == HD_OK);
    for (i = 0; i < bytes; i += 4) {
        unsigned sa = first[i+3], alpha = sa * 255 + 90 * (255-sa);
        const unsigned back[] = {10,30,70}, front[] = {255,240,190};
        CHECK(abs(p.rgba[i+3] - (int)((alpha+127)/255)) <= 1);
        for (k=0;k<3;k++) CHECK(abs(p.rgba[i+k]-(int)((front[k]*sa*255+back[k]*90*(255-sa)+alpha/2)/alpha))<=1);
    }
    /* Invalid UTF-8 / geometry leave pixels and output metrics untouched. */
    memcpy(first,p.rgba,bytes);
    CHECK(hd_text_draw(f,&p,"\xc0\x80",2,0,0,8,100,white) == HD_TEXT);
    CHECK(!memcmp(first,p.rgba,bytes));
    CHECK(hd_text_measure_native(f,"\xed\xa0\x80",3,20,100,&unchanged) == HD_TEXT);
    CHECK(unchanged.width==123 && unchanged.height==456 && unchanged.pixel_size==7 && unchanged.lines==8);
    CHECK(hd_text_draw(f,&p,text,strlen(text),NAN,0,8,100,white) == HD_ARGUMENT);
    CHECK(hd_text_measure_native(f,text,strlen(text),257,100,&n) == HD_ARGUMENT);
    CHECK(hd_text_measure_logical(f,&p,text,strlen(text),INFINITY,100,&n)==HD_ARGUMENT);
    CHECK(hd_text_measure_native(f,"a\0b",3,20,100,&n)==HD_TEXT);
    CHECK(hd_text_measure_native(f,"",0,20,100,&n)==HD_OK && n.width==0 && n.lines==1);
    CHECK(hd_text_measure_native(f,"甲，乙（丙）丁",strlen("甲，乙（丙）丁"),20,40,&n)==HD_OK && n.lines>1);
    CHECK(hd_text_measure_native(f,"A\tB\r\nC\\pD",9,20,200,&n)==HD_OK && n.lines==2);
    CHECK(hd_text_measure_native(f,"\xf4\x8f\xbf\xbf",4,20,100,&n)==HD_OK);
    hd_font_stats(f,&b); CHECK(b.missing_glyphs==1);
    /* Separate face ownership, cache eviction and changed-size rerasterization. */
    CHECK(hd_font_init(&other,argv[1])==HD_OK);
    hd_font_stats(other,&a); CHECK(a.entries==0);
    CHECK(hd_text_measure_native(other,text,strlen(text),29,804,&n)==HD_OK);
    hd_font_stats(other,&a); CHECK(a.rasterizations>0); hd_font_shutdown(other);
    hd_font_stats(f,&a);
    CHECK(hd_text_measure_native(f,text,strlen(text),41,1000,&n)==HD_OK);
    hd_font_stats(f,&b); CHECK(b.rasterizations>a.rasterizations);
    for (i=0;i<800;i++) {
        unsigned cp=0x4e00+(unsigned)i;
        char s[3]={(char)(0xe0|(cp>>12)),(char)(0x80|((cp>>6)&63)),(char)(0x80|(cp&63))};
        CHECK(hd_text_measure_native(f,s,3,i<600?12:256,0,&n)==HD_OK);
    }
    hd_font_stats(f,&b); CHECK(b.evictions>0 && b.entries<=512 && b.bitmap_bytes<=8*1024*1024);
    /* Hostile clipping on a pitched surface: preserve guard bytes and padding. */
    free(p.rgba); free(first);
    p.width=71;p.height=43;p.pitch=71*4+16;
    p.viewport_x=0;p.viewport_y=0;p.viewport_w=320;p.viewport_h=200;
    bytes=(size_t)p.pitch*p.height;
    guard=malloc(bytes+128);CHECK(guard);memset(guard,0xcd,bytes+128);p.rgba=guard+64;
    for(row=0;row<p.height;row++)memset(p.rgba+(size_t)row*p.pitch,0,p.width*4);
    CHECK(hd_text_draw(f,&p,text,strlen(text),-8,-15,32,0,white)==HD_OK);
    CHECK(hd_text_draw(f,&p,text,strlen(text),1000000,1000000,32,0,white)==HD_OK);
    CHECK(hd_text_draw(f,&p,text,strlen(text),-1000000,-1000000,32,0,white)==HD_OK);
    for(i=0;i<64;i++)CHECK(guard[i]==0xcd && guard[bytes+64+i]==0xcd);
    for(row=0;row<p.height;row++)for(col=p.width*4;col<p.pitch;col++)CHECK(p.rgba[(size_t)row*p.pitch+col]==0xcd);
    free(guard); hd_font_shutdown(f); hd_font_shutdown(NULL);
    printf("PASS %d assertions: CJK/AA, metrics, cache hit/eviction/face/size, alpha, malformed input, wrapping, clipping\n",checks);
    return 0;
}
