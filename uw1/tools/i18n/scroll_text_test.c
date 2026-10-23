#include "presentation/scroll_text.h"
#include "presentation/scroll_bridge.h"
#include "i18n/localization.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
static int checks,waits,pauses,more_prompts;
#define CHECK(c) do {checks++;if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);exit(1);}}while(0)
static ScrollFrame snapshot;
static int measure(uint32_t c,void *context) { (void)context;return (c<128?4:8)*64; }
static void wait_input(int ticks,void *context)
{
    int i,found=0;(void)context;waits++;
    if(ticks) {CHECK(ticks==200 || ticks==600);pauses++;return;}
    scroll_text_read(&snapshot);
    for(i=0;i<snapshot.boxes[0].count;i++)if(snapshot.boxes[0].cells[i].scalar=='[')found=1;
    CHECK(found);more_prompts++;
}
static void reset(int width,int height)
{
    waits=pauses=more_prompts=0;
    scroll_text_start(measure,NULL,wait_input,NULL);scroll_text_screen(4);
    scroll_text_box(0,10,20,10+width,20+height,8,0x2e);
}
int scroll_print(char *text) { CHECK(port_scroll_write(0,text,1)==0);return scroll_text_lines(0); }
void port_fatal(const char *format,...) { (void)format;CHECK(0);abort(); }
/* the game's and the backend's side of P11 (BABL.C, plat_sdl3.c): no template expansion or
   input method here; the bridge's own logic is what is checked */
char *port_template_shown(const char *source) { (void)source;return NULL; }
void plat_text_input(int on) { (void)on; }
size_t plat_text_take(char *out,size_t n) { if(n)out[0]=0;return 0; }
void plat_sleep_ns(uint64_t ns) { (void)ns; }
static void bounds(void)
{
    int i;ScrollBox *b;
    scroll_text_read(&snapshot);b=&snapshot.boxes[0];
    CHECK(b->count<=SCROLL_CELLS);
    for(i=0;i<b->count;i++) {
        const ScrollCell *c=&b->cells[i];
        CHECK(c->x>=b->left*64 && c->x+measure(c->scalar,NULL)<=b->right*64);
        CHECK(c->y>=b->top && c->y+b->line_height<=b->bottom);
        CHECK(c->scalar<0x110000 && !(c->scalar>=0xd800 && c->scalar<=0xdfff));
    }
}
/* P10T: a box's style. Measure: 4 for ASCII, 8 otherwise, in whole logical pixels. */
static void styles(void)
{
    ScrollBox *b;int i;
    /* defaults: a 5 em, the game's own line height */
    reset(96,40);CHECK(scroll_text_write(0,"一",1,0)==0);scroll_text_read(&snapshot);b=&snapshot.boxes[0];
    CHECK(b->em64==5*64 && b->line64==8*64);
    /* line spacing 2.5: a 10.5 pitch, 3 rows of 40, y64 exact, y the whole part */
    scroll_text_start(measure,NULL,wait_input,NULL);scroll_text_style(0,6*64,0,0,160);scroll_text_screen(4);
    scroll_text_box(0,10,20,106,60,8,0x2e);
    CHECK(scroll_text_write(0,"一\n二\n三",1,0)==0);scroll_text_read(&snapshot);b=&snapshot.boxes[0];
    CHECK(b->em64==6*64 && b->line64==8*64+160 && b->count==3);
    for(i=0;i<3;i++)CHECK(b->cells[i].y64==20*64+i*(8*64+160) && b->cells[i].y==b->cells[i].y64/64);
    CHECK(scroll_text_y(0)==(20*64+2*(8*64+160))/64);
    /* a fourth line rolls: the box holds three of 10.5 in 40 */
    CHECK(scroll_text_write(0,"\n四",1,0)==0);scroll_text_read(&snapshot);b=&snapshot.boxes[0];
    CHECK(b->count==3 && b->cells[0].scalar==0x4e8c && b->cells[2].y64==20*64+2*(8*64+160));
    /* choices hit by the spaced pitch */
    scroll_text_clear(0);CHECK(scroll_text_write(0,"甲\n",1,1)==0 && scroll_text_write(0,"乙\n",1,2)==0);
    CHECK(scroll_text_hit(12,20)==1 && scroll_text_hit(12,30)==1 && scroll_text_hit(12,31)==2 && scroll_text_hit(12,41)==-1);
    /* letter spacing after each glyph, Latin/Chinese spacing at the boundary, across two writes too */
    scroll_text_start(measure,NULL,wait_input,NULL);scroll_text_style(0,5*64,64,3*64,0);scroll_text_screen(4);
    scroll_text_box(0,10,20,200,44,8,0x2e);
    CHECK(scroll_text_write(0,"中文A",1,0)==0);scroll_text_read(&snapshot);b=&snapshot.boxes[0];
    CHECK(b->cells[0].x==10*64 && b->cells[1].x==10*64+9*64 && b->cells[2].x==10*64+18*64+3*64);
    CHECK(scroll_text_write(0,"字",1,0)==0);scroll_text_read(&snapshot);b=&snapshot.boxes[0];
    CHECK(b->cells[3].x==b->cells[2].x+5*64+3*64);
    CHECK(scroll_text_write(0," 字",1,0)==0);scroll_text_read(&snapshot);b=&snapshot.boxes[0];
    CHECK(b->cells[5].x==b->cells[4].x+4*64); /* after a space: no boundary */
    /* the other box keeps its own style */
    scroll_text_box(1,10,100,200,140,8,0x2e);CHECK(scroll_text_write(1,"一",1,0)==0);scroll_text_read(&snapshot);
    CHECK(snapshot.boxes[1].em64==5*64 && snapshot.boxes[1].line64==8*64);
    scroll_text_style(0,5*64,0,0,0);
}
static atomic_int done;
static int producer(void *unused)
{
    int i;(void)unused;
    for(i=0;i<5000;i++) {
        char s[2]={(char)('A'+i%26),0};
        scroll_text_clear(0);scroll_text_box(0,10,20,90,44,8,i%26);
        if(scroll_text_write(0,s,1,i%26+1))abort();
    }
    atomic_store(&done,1);return 0;
}
int main(int argc,char **argv)
{
    char original[512]="Hail",other[]="Hail",templ[]="Hello @SS1",bad[]={ (char)0xe4,(char)0xbd,0 };
    int i,seen=0;uint64_t generation=0;SDL_Thread *thread;
    CHECK(argc==2);scroll_text_enable(1);
    reset(96,40);CHECK(scroll_text_write(0,"你好，冒險者。",1,0)==0);bounds();
    CHECK(snapshot.boxes[0].count==7 && snapshot.boxes[0].cells[0].scalar==0x4f60);
    reset(32,24);
    CHECK(scroll_text_write(0,"中文沒有空白也能換行中文沒有空白也能換行中文沒有空白也能換行",1,0)==0);
    bounds();CHECK(more_prompts>0);
    reset(40,40);CHECK(scroll_text_write(0,"English + 中文 123",1,0)==0);bounds();
    reset(32,40);CHECK(scroll_text_write(0,"中文「標點」中文，文字。",1,0)==0);bounds();
    for(i=0;i<snapshot.boxes[0].count;i++)if(snapshot.boxes[0].cells[i].scalar==0x300c)
        CHECK(snapshot.boxes[0].cells[i].x<=(snapshot.boxes[0].right-16)*64);
    reset(32,40);CHECK(scroll_text_write(0,"Supercalifragilisticexpialidocious",1,0)==0);bounds();
    reset(32,40);CHECK(scroll_text_write(0,"hello ",1,0)==0);CHECK(scroll_text_x(0)==34);
    CHECK(scroll_text_write(0,"world",1,0)==0);bounds();CHECK(snapshot.boxes[0].cells[6].y==28);
    reset(96,40);CHECK(scroll_text_write(0,"甲\n乙\r\n丙",1,0)==0);bounds();
    CHECK(snapshot.boxes[0].cells[0].y==20 && snapshot.boxes[0].cells[1].y==28 && snapshot.boxes[0].cells[2].y==36);
    reset(96,40);CHECK(scroll_text_write(0,"\\0甲\\1乙\\2丙\\3丁\\4戊\\5己\\6庚\\p\\P\\m辛",1,0)==0);
    bounds();CHECK(waits==3 && pauses==2 && more_prompts==1 && scroll_text_color(0)==0xd4);
    for(i=0;i<snapshot.boxes[0].count;i++)CHECK(snapshot.boxes[0].cells[i].scalar!='\\');
    reset(96,40);CHECK(scroll_text_write(0,"甲\n乙\\m丙",1,0)==0);bounds();
    CHECK(more_prompts==1 && snapshot.boxes[0].count==2 && snapshot.boxes[0].cells[1].y==28);
    reset(96,40);CHECK(scroll_text_write(0,"\\m",0,0)==0);bounds();CHECK(waits==0 && snapshot.boxes[0].count==2);
    generation=snapshot.generation;CHECK(scroll_text_write(0,bad,1,0)==-1);
    scroll_text_read(&snapshot);CHECK(snapshot.generation==generation);
    CHECK(scroll_text_write(0,"",1,0)==0);bounds();CHECK(snapshot.boxes[0].count==2);
    reset(96,40);CHECK(scroll_text_write(0,"1. 第一個選項\n",1,1)==0);
    CHECK(scroll_text_write(0,"2. 第二個選項\n",1,2)==0);bounds();
    CHECK(scroll_text_hit(12,22)==1 && scroll_text_hit(12,30)==2 && scroll_text_hit(12,38)==-1);
    scroll_text_screen(2);CHECK(scroll_text_hit(12,22)==-1);scroll_text_read(&snapshot);CHECK(snapshot.boxes[0].count==0);
    scroll_text_screen(4);CHECK(scroll_text_write(0,"編輯前的文字",1,0)==0);
    CHECK(scroll_text_suspend(1));scroll_text_read(&snapshot);CHECK(!snapshot.boxes[0].count && !scroll_text_enabled());
    scroll_text_suspend(0);CHECK(scroll_text_enabled());
    reset(160,40);CHECK(port_i18n_init("zh-TW",argv[1])==0);
    CHECK(port_i18n_observe(3651,3,original)==original && !strcmp(original,"Hail"));
    CHECK(!strcmp(port_i18n_display(original),"你好"));
    CHECK(port_i18n_display(other)==other); /* Equal contents without key provenance do not translate. */
    port_scroll_choice_bind(1,original);CHECK(port_scroll_option(1,"Hail")==0);bounds();
    CHECK(snapshot.boxes[0].cells[3].scalar==0x4f60);
    port_i18n_forget(original);CHECK(port_i18n_display(original)==original); /* dynamic pointer invalidation */
    port_i18n_observe(3651,3,original);original[0]='X';CHECK(port_i18n_display(original)==original);original[0]='H';
    port_i18n_observe(3651,4,templ);CHECK(port_i18n_display(templ)==templ); /* templates retain expanded English */
    CHECK(port_i18n_template(templ) && !strcmp(port_i18n_template(templ),"你好@SS1")); /* P11.1: the template for display */
    CHECK(port_i18n_template(original)==NULL && port_i18n_template(other)==NULL);
    {   /* P11.4: a typed line drawn again from the mark */
        ScrollBox mark;int count;
        reset(160,40);CHECK(scroll_text_write(0,"前文\n",1,0)==0);bounds();count=snapshot.boxes[0].count;
        scroll_text_mark(0,&mark);CHECK(scroll_text_write(0,">聖物_",0,0)==0);
        scroll_text_rewind(0,&mark);bounds();CHECK(snapshot.boxes[0].count==count);
        CHECK(scroll_text_write(0,">聖",0,0)==0);bounds();CHECK(snapshot.boxes[0].count==count+2);
    }
    CHECK(port_scroll_write(0,"\x82",0)==0);bounds();CHECK(snapshot.boxes[0].cells[snapshot.boxes[0].count-1].scalar==0xe9);
    port_i18n_shutdown();CHECK(port_i18n_display(original)==original);port_scroll_choices_reset();
    styles();
    reset(80,24);generation=0;thread=SDL_CreateThread(producer,"scroll-producer",NULL);CHECK(thread);
    do {
        scroll_text_read(&snapshot);CHECK(snapshot.generation>=generation);generation=snapshot.generation;
        if(snapshot.boxes[0].count) {
            const ScrollCell *c=&snapshot.boxes[0].cells[0];
            CHECK(snapshot.boxes[0].count==1 && c->scalar==(uint32_t)('A'+c->color) && c->choice==c->color+1);seen++;
        }
    }while(!atomic_load(&done));
    SDL_WaitThread(thread,NULL);CHECK(seen>0);
    printf("PASS %d assertions: UTF-8, CJK/English wrap, punctuation, append, pages/MORE, controls, choices, fallback, styles, 5000 concurrent snapshots\n",checks);
    return 0;
}
