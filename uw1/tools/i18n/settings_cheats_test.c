#include "ui/settings.h"
#include "sys/enhance.h"
#include "bright_view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int calls, last;
static void action(int v) { calls++; last=v; }
#define port_cheat_brightness_request action
#include "brightness_setting_impl.h"
#define R_BRIGHT_STRENGTH 0
#include "brightness_start_impl.h"
#undef port_cheat_brightness_request
int port_config_get(const char *,const char *,char *,size_t);
int port_config_set(const char *,const char *,const char *);
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
    /* Real game's row: numeric range, keyboard, pointer, persistence and bad config. */
    {
        char saved[32];
        settings_init("",brightness_rows,1,"Test");settings_show(1);
        for(i=0;i<5;i++)settings_key(SET_KEY_TAB);
        CHECK(settings_value(0)==4 && !settings_note(0));
        settings_key(SET_KEY_RIGHT);CHECK(settings_value(0)==5 && last==5);
        for(i=0;i<30;i++)settings_key(SET_KEY_RIGHT);
        CHECK(settings_value(0)==15 && last==15);
        CHECK(!port_config_get("","bright-strength",saved,sizeof saved) && !strcmp(saved,"15"));
        settings_init("",brightness_rows,1,"Test");CHECK(settings_value(0)==15);
        settings_show(1);for(i=0;i<5;i++)settings_key(SET_KEY_TAB);
        for(i=0;i<30;i++)settings_key(SET_KEY_LEFT);
        CHECK(settings_value(0)==0 && last==0);
        settings_pointer(408,82,1);settings_pointer(408,82,0);CHECK(settings_value(0)==15 && last==15);
        settings_pointer(408,82,1);settings_pointer(248,82,1);settings_pointer(248,82,0);CHECK(settings_value(0)==0 && last==0);
        CHECK(settings_draw(pixels));
        port_config_set("","bright-strength","99");settings_init("",brightness_rows,1,"Test");CHECK(settings_value(0)==15);
        port_config_set("","bright-strength","-3");settings_init("",brightness_rows,1,"Test");CHECK(settings_value(0)==0);
        port_config_set("","bright-strength","garbage");settings_init("",brightness_rows,1,"Test");CHECK(settings_value(0)==4);
        i=calls;port_settings_brightness(0);CHECK(calls==i); /* default keeps old recordings unchanged */
        port_config_set("","bright-strength","15");settings_init("",brightness_rows,1,"Test");
        port_settings_brightness(0);CHECK(last==15 && calls==i+1);
        i=calls;port_settings_brightness(1);CHECK(settings_value(0)==4 && calls==i);
    }
    puts("PASS public cheat tab: hidden when empty, actions, live flags, keyboard, pointer and drawing");
    return 0;
}
