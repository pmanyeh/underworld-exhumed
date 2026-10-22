#include "scroll_bridge.h"
#include "i18n/utf8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int scroll_print(char *text);
void port_fatal(const char *format,...);
static const char *choices[20];
static int choice_context;
int port_scroll_choice(void) { return choice_context; }
void port_scroll_choices_reset(void) { memset(choices,0,sizeof choices); }
void port_scroll_choice_bind(int option,const char *legacy)
{
    const char *translated=port_i18n_display(legacy);
    if(option>0 && option<20)choices[option]=translated!=legacy?translated:NULL;
}
const char *port_scroll_choice_text(int option,const char *expanded)
{ return option>0 && option<20 && choices[option]?choices[option]:expanded; }
int port_scroll_emit(const char *prefix,const char *text,const char *suffix,int choice)
{
    size_t a,b,c;char *copy;int previous=choice_context,result;
    if(!scroll_text_enabled())return -1;
    a=strlen(prefix);b=strlen(text);c=strlen(suffix);
    if(a>16384 || b>16384-a || c>16384-a-b)port_fatal("HD scroll: display text exceeds 16KiB");
    copy=malloc(a+b+c+1);if(!copy)port_fatal("HD scroll: allocation failed");
    memcpy(copy,prefix,a);memcpy(copy+a,text,b);memcpy(copy+a+b,suffix,c+1);
    choice_context=choice;result=scroll_print(copy);choice_context=previous;
    free(copy);return result;
}
int port_scroll_option(int option,const char *expanded)
{
    char number[16];snprintf(number,sizeof number,"%d. ",option);
    return port_scroll_emit(number,port_scroll_choice_text(option,expanded),"\n",option);
}
/* CP437 fallback conversion table is generated from Python's cp437 codec.
   Catalog translations are already strict UTF-8; original high bytes remain
   display-only and never enter the layout as malformed UTF-8. */
static const unsigned short cp437[128]={
    0x00c7,0x00fc,0x00e9,0x00e2,0x00e4,0x00e0,0x00e5,0x00e7,0x00ea,0x00eb,0x00e8,0x00ef,0x00ee,0x00ec,0x00c4,0x00c5,
    0x00c9,0x00e6,0x00c6,0x00f4,0x00f6,0x00f2,0x00fb,0x00f9,0x00ff,0x00d6,0x00dc,0x00a2,0x00a3,0x00a5,0x20a7,0x0192,
    0x00e1,0x00ed,0x00f3,0x00fa,0x00f1,0x00d1,0x00aa,0x00ba,0x00bf,0x2310,0x00ac,0x00bd,0x00bc,0x00a1,0x00ab,0x00bb,
    0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,0x2555,0x2563,0x2551,0x2557,0x255d,0x255c,0x255b,0x2510,
    0x2514,0x2534,0x252c,0x251c,0x2500,0x253c,0x255e,0x255f,0x255a,0x2554,0x2569,0x2566,0x2560,0x2550,0x256c,0x2567,
    0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256b,0x256a,0x2518,0x250c,0x2588,0x2584,0x258c,0x2590,0x2580,
    0x03b1,0x00df,0x0393,0x03c0,0x03a3,0x03c3,0x00b5,0x03c4,0x03a6,0x0398,0x03a9,0x03b4,0x221e,0x03c6,0x03b5,0x2229,
    0x2261,0x00b1,0x2265,0x2264,0x2320,0x2321,0x00f7,0x2248,0x00b0,0x2219,0x00b7,0x221a,0x207f,0x00b2,0x25a0,0x00a0
};
int port_scroll_write(int which,const char *legacy,int escapes)
{
    const char *text=port_i18n_display(legacy);char *converted=NULL;int result;
    size_t len=strlen(text),i,n=0;
    if(len>16384)port_fatal("HD scroll: input exceeds 16KiB");
    if(!utf8_validate(text,len,NULL)) {
        converted=malloc(len*3+1);if(!converted)port_fatal("HD scroll: allocation failed");
        for(i=0;i<len;i++) {
            unsigned cp=(unsigned char)text[i];if(cp>=128)cp=cp437[cp-128];
            if(cp<128)converted[n++]=(char)cp;
            else if(cp<2048) {converted[n++]=(char)(0xc0|(cp>>6));converted[n++]=(char)(0x80|(cp&63));}
            else {converted[n++]=(char)(0xe0|(cp>>12));converted[n++]=(char)(0x80|((cp>>6)&63));converted[n++]=(char)(0x80|(cp&63));}
        }
        converted[n]=0;text=converted;
    }
    result=scroll_text_write(which,text,escapes,choice_context);free(converted);
    if(result<0)port_fatal("HD scroll: invalid text, metrics or display capacity");
    return result;
}
