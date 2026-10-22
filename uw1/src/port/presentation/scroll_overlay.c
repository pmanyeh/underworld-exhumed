#include "scroll_overlay.h"
int scroll_overlay_draw(HdFont *font,const PlatOverlay *frame,const ScrollFrame *text,const uint8_t pal[768])
{
    int box,i,drawn=0;
    if(!font || !frame || !text || !pal || (text->screen!=1 && text->screen!=4))return 0;
    for(box=0;box<2;box++) {
        const ScrollBox *b=&text->boxes[box];
        float clip[4]={(float)b->left,(float)b->top,(float)b->right,(float)b->bottom};
        if((box && text->screen!=4) || !b->line_height)continue;
        for(i=0;i<b->count && i<SCROLL_CELLS;i++) {
            const ScrollCell *c=&b->cells[i];const uint8_t *rgb=pal+((unsigned)c->color&255)*3;
            HdColor color={(uint8_t)((rgb[0]<<2)|(rgb[0]>>4)),(uint8_t)((rgb[1]<<2)|(rgb[1]>>4)),
                           (uint8_t)((rgb[2]<<2)|(rgb[2]>>4)),255};
            if (c->scalar==' ' || c->scalar=='\t') continue;
            if(hd_scroll_glyph(font,frame,c->scalar,c->x/64.0f,(float)c->y,5,clip,color)==HD_OK)drawn=1;
        }
    }
    return drawn;
}
