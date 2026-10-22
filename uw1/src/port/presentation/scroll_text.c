#include "scroll_text.h"
#include "i18n/layout.h"
#include "i18n/utf8.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

static int enabled, suspended;
static ScrollFrame work, published;
static atomic_flag busy = ATOMIC_FLAG_INIT;
static ScrollMeasure measure;
static ScrollWait wait_for_input;
static void *measure_context, *wait_context;
static FILE *trace;
static const int colors[] = {0x2e,0x26,0xf1,0x60,0xb4,0xc4,0xd4};

static void lock(void) { while(atomic_flag_test_and_set_explicit(&busy,memory_order_acquire)) {} }
static void unlock(void) { atomic_flag_clear_explicit(&busy,memory_order_release); }
static void publish(void)
{
    work.generation++;
    lock(); published=work;if(suspended)published.boxes[0].count=0;unlock();
}
void scroll_text_enable(int on) { enabled=on!=0; }
int scroll_text_enabled(void) { return enabled && !suspended; }
void scroll_text_start(ScrollMeasure m,void *context,ScrollWait w,void *wc)
{
    memset(&work,0,sizeof work);suspended=0;
    measure=m;measure_context=context;wait_for_input=w;wait_context=wc;publish();
}
int scroll_text_trace_open(const char *path) { trace=path?fopen(path,"wb"):NULL;return path&&!trace?-1:0; }
void scroll_text_trace_close(void) { if(trace)fclose(trace);trace=NULL; }
static void event(const char *name,int which,int value)
{
    if(trace) {
        fprintf(trace,"{\"event\":\"%s\",\"box\":%d,\"value\":%d,\"cells\":%d,\"lines\":%d}\n",
            name,which,value,work.boxes[which].count,work.boxes[which].lines);fflush(trace);
    }
}
void scroll_text_read(ScrollFrame *out) { if(out) {lock();*out=published;unlock();} }
void scroll_text_screen(int mode)
{
    if(!enabled)return;
    if(work.screen==4 && mode!=4) {scroll_text_clear(0);scroll_text_clear(1);}
    work.screen=mode;publish();
}
void scroll_text_clear(int which)
{
    ScrollBox *b;
    if(!enabled || which<0 || which>1)return;
    b=&work.boxes[which];b->count=b->row=b->lines=b->pending=0;b->x=b->left*64;publish();
}
int scroll_text_suspend(int on)
{
    int was=scroll_text_enabled();
    if(!enabled)return 0;
    if(on)suspended++;
    else if(suspended) {
        suspended--;
        if(!suspended)scroll_text_clear(0);
    }
    publish();return was;
}
void scroll_text_box(int which,int left,int top,int right,int bottom,int height,int color)
{
    ScrollBox *b;
    if(!enabled || which<0 || which>1 || left<0 || top<0 || right>320 || bottom>200 ||
        right<=left || bottom<=top || height<1 || height>bottom-top)return;
    b=&work.boxes[which];
    if(b->left!=left || b->top!=top || b->right!=right || b->bottom!=bottom || b->line_height!=height) {
        memset(b,0,sizeof *b);b->left=left;b->top=top;b->right=right;b->bottom=bottom;b->line_height=height;b->x=left*64;
    }
    b->color=color;
}
static int rows(const ScrollBox *b) { return (b->bottom-b->top)/b->line_height; }
static void roll(ScrollBox *b)
{
    int i,n=0;
    for(i=0;i<b->count;i++)if(b->cells[i].y>=b->top+b->line_height) {
        b->cells[n]=b->cells[i];b->cells[n++].y-=b->line_height;
    }
    b->count=n;b->row=rows(b)-1;b->x=b->left*64;b->pending=0;
}
static int put(ScrollBox *b,uint32_t cp,int advance,int choice)
{
    ScrollCell *c;
    if(b->count==SCROLL_CELLS || advance<0 || advance>INT_MAX-b->x)return -1;
    c=&b->cells[b->count++];c->scalar=cp;c->x=b->x;
    c->y=b->top+b->row*b->line_height;c->color=b->color;c->choice=choice;
    b->x+=advance;return 0;
}
static void more(int which)
{
    ScrollBox *b=&work.boxes[which];
    const char *label="[MORE]";int i,n,color=b->color,row=b->row;
    /* Explicit \m may occur halfway down the NPC paper. Roll one line but
       retain that writing row; automatic bottom overflow already uses the last row. */
    roll(b);b->row=row;n=b->count;b->color=0x60;
    for(i=0;label[i];i++)(void)put(b,(unsigned char)label[i],measure((unsigned char)label[i],measure_context),0);
    publish();event("more",which,0);
    if(wait_for_input)wait_for_input(0,wait_context);
    b->count=n;b->color=color;b->x=b->left*64;b->pending=0;publish();
}
static void newline(int which,int *fresh)
{
    ScrollBox *b=&work.boxes[which];
    if(b->row+1<rows(b))b->row++;
    else if(*fresh>=rows(b)-1) {more(which);*fresh=0;}
    else roll(b);
    b->lines++;(*fresh)++;b->x=b->left*64;b->pending=0;
}
static void control(int which,uint32_t cp,int *fresh)
{
    ScrollBox *b=&work.boxes[which];
    if(cp>='0' && cp<='6')b->color=colors[cp-'0'];
    else if(cp=='m') {more(which);*fresh=0;}
    else {
        publish();event("pause",which,cp=='p'?600:200);
        if(wait_for_input)wait_for_input(cp=='p'?600:200,wait_context);
    }
}
int scroll_text_write(int which,const char *text,int escapes,int choice)
{
    ScrollBox *b;
    char *copy;size_t len,pos=0;int fresh=0,status=0;
    if(!scroll_text_enabled() || which<0 || which>1 || !text || !measure)return -1;
    b=&work.boxes[which];
    if(!b->line_height || !rows(b))return -1;
    len=strlen(text);
    if(len>16384 || !utf8_validate(text,len,NULL))return -1;
    copy=malloc(len+1);if(!copy)return -1;memcpy(copy,text,len+1);
    if(trace) {
        const unsigned char *s=(const unsigned char *)copy;
        fprintf(trace,"{\"event\":\"text\",\"box\":%d,\"choice\":%d,\"text\":\"",which,choice);
        for(;*s;s++) {
            if(*s=='"' || *s=='\\') {fputc('\\',trace);fputc(*s,trace);}
            else if(*s<32)fprintf(trace,"\\u%04x",*s);
            else fputc(*s,trace);
        }
        fputs("\"}\n",trace);fflush(trace);
    }
    while(pos<len) {
        struct text_layout layout={0};struct layout_line *line;size_t i,end;
        int available;
        if(b->pending)newline(which,&fresh);
        available=b->right*64-b->x;
        if(available<1) {newline(which,&fresh);available=(b->right-b->left)*64;}
        status=layout_text_controls(copy+pos,len-pos,available,measure,measure_context,escapes,&layout,NULL);
        if(status)break;
        line=&layout.lines[0];
        /* A new English word that fits a full line should not be split merely
           because an earlier print call left a partial line. */
        if(b->x>b->left*64) {
            int64_t word=0;int latin=0;
            for(i=0;i<layout.unit_count;i++) {
                const struct layout_unit *u=&layout.units[i];
                if(u->kind==LAYOUT_CONTROL)continue;
                if(u->kind!=LAYOUT_GLYPH || (u->flags&LAYOUT_CJK))break;
                if(u->flags&LAYOUT_SPACE) {if(latin)break;else continue;}
                latin=1;word+=u->advance;
            }
            if(latin && word>available && word<=(b->right-b->left)*64) {
                layout_free(&layout);newline(which,&fresh);continue;
            }
        }
        if(line->width>available && b->x>b->left*64) {layout_free(&layout);newline(which,&fresh);continue;}
        end=pos;
        for(i=line->unit_start;i<line->unit_start+line->unit_count;i++) {
            struct layout_unit *u=&layout.units[i];
            end=pos+u->byte_offset+u->byte_length;
            if(u->kind==LAYOUT_CONTROL)control(which,u->scalar,&fresh);
            else if(u->kind==LAYOUT_NEWLINE)b->pending=1;
            else if(!(u->flags&LAYOUT_TRIMMED) ||
                ((u->flags&LAYOUT_SPACE) && layout.line_count==1 && !(line->flags&LAYOUT_HARD_BREAK) &&
                 u->advance<=b->right*64-b->x)) {
                if(put(b,u->scalar,u->advance,choice)) {status=-1;break;}
            }
        }
        if(layout.line_count>1)b->pending=1;
        layout_free(&layout);
        if(status || end==pos)break;
        pos=end;publish();
    }
    event(status?"error":"write",which,choice);publish();free(copy);return status?-1:0;
}
int scroll_text_x(int which) { return work.boxes[which].x/64; }
int scroll_text_y(int which) { ScrollBox *b=&work.boxes[which];return b->top+b->row*b->line_height; }
int scroll_text_lines(int which) { return work.boxes[which].lines; }
int scroll_text_color(int which) { return work.boxes[which].color; }
int scroll_text_hit(int x,int y)
{
    ScrollBox *b=&work.boxes[0];int i;
    if(!scroll_text_enabled() || work.screen!=4 || x<b->left || x>=b->right || y<b->top || y>=b->bottom)return -1;
    for(i=b->count-1;i>=0;i--) {
        ScrollCell *c=&b->cells[i];
        if(y>=c->y && y<c->y+b->line_height && c->choice>0)return c->choice;
    }
    return -1;
}
void scroll_text_selection(int option) { if(enabled)event("selection",0,option); }
