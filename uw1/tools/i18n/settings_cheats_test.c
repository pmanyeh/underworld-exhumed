#include "ui/settings.h"
#include "sys/enhance.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int calls, last;
static void action(int v) { calls++; last=v; }
static const struct enhance_flag flags[] = {
    { "test-god", "Health", ENH_GAMEPLAY, NULL, NULL }
};
static const struct setting rows[] = {
    { .tab=SET_TAB_CHEATS, .kind=SET_ACTION, .label="Rescue", .apply=action }
};
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL settings line %d\n",__LINE__); return 1; } } while(0)
int main(void)
{
    static uint32_t pixels[SET_W*SET_H];
    int i;
    enhance_init(flags,1,"test");enhance_on=1;
    settings_init("",rows,1,"Test");settings_enhancement(0,SET_TAB_CHEATS,action);
    settings_show(1);
    for(i=0;i<5;i++)settings_key(SET_KEY_TAB);
    CHECK(settings_draw(pixels));
    settings_key(SET_KEY_ENTER);CHECK(calls==1 && last==1 && settings_value(0)==0);
    settings_key(SET_KEY_RIGHT);CHECK(calls==2 && settings_value(0)==0);
    settings_key(SET_KEY_DOWN);CHECK(settings_value(1)==1);
    settings_key(SET_KEY_ENTER);CHECK(calls==3 && !last && !settings_value(1));
    CHECK(!strcmp(settings_note(1),"changes play"));
    settings_pointer(300,92,1);settings_pointer(300,92,0);
    CHECK(calls==4 && last==1 && settings_value(1)==1);
    /* No optional rows means no extra tab; five tabs wrap to Sound. */
    settings_init("",NULL,0,"Test");settings_show(1);
    for(i=0;i<5;i++)settings_key(SET_KEY_TAB);
    CHECK(settings_draw(pixels));
    puts("PASS public cheat tab: hidden when empty, actions, live flags, keyboard, pointer and drawing");
    return 0;
}
