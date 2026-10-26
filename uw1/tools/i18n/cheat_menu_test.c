/* Exercise the real menu/recovery/store code with a synthetic world and collision answers. */
#include "../../src/port/ui/cheat_menu.c"
#include "../../src/port/ui/full_map.c"
#include "../../src/port/presentation/bright_view.c"
#undef main
#undef exit
#undef fopen
#undef fclose
#undef fread
#undef fseek
#undef fwrite
#include <stdio.h>
#include <stddef.h>
static int checks, failures, placements, level_changes, collision_calls, occupied, no_floor;
static int messages;
static struct Scroll message_scroll;
struct Scroll *scroll = &message_scroll;
#define CHECK(c) do { checks++; if (!(c)) { printf("FAIL %d: %s\n",__LINE__,#c); failures++; } } while (0)
uint32_t enhance_on;
static unsigned char lights[4096], original_lights[4096];
unsigned char *cLightTabs = lights;
int16 smooth_div, smooth_base, smooth_lowpass, curvrad;
struct Gloc glocs[17][33];
int cSqRt(int32 v) { int r=0; while ((r+1)*(r+1)<=v) r++; return r; }
#include "bright_grid_impl.h"
static struct Object actor, held;
struct Object *ThePlayer = &actor, *CursorObjPtr;
static struct Tile world[4096];
struct Tile *mapdata = world;
uint16 TxmTerr[TXM_FLOORS];
struct Phys PN;
int16 PlayerLevel=1, GrSq, PlayerInput, NewPlayerX, NewPlayerY, NewPlayerLevel;
char NewPlyFade;
int16 nvokTerr, nvokHgt, GameInputMode, scrnum;
char in_game=1;
void (*npp_func)(void);
static uint32_t seed=123;
uint32_t port_rand_seed(void) { return seed; }
void port_set_rand_seed(uint32_t s) { seed=s; }
int scroll_print(char *s) { (void)s; CHECK(scroll != NULL); messages++; return 0; }
int port_zh(void) { return 0; }
const char *port_zh_lit(const char *s) { return s; }
void editchng(int n) { (void)n; }
unsigned char can_place(int item,int index,int x,int y,int z,unsigned char fly,unsigned char range)
{
    (void)item;(void)index;(void)z;(void)fly;(void)range;collision_calls++;
    CHECK(x>=8 && x<=503 && y>=8 && y<=503);
    nvokTerr=FOOT_FLOOR;nvokHgt=0;
    return !no_floor && !(occupied && x/8==10 && y/8==10);
}
void player_setup(int x,int y)
{
    placements++;PN.x=x*256+128;PN.y=y*256+128;PN.z=0;GrSq=x+y*64;
    SET_HOMEX(ThePlayer,x);SET_HOMEY(ThePlayer,y);
    PN.terrain=FOOT_FLOOR;
}
char ChangeLevel(int from,int to) { (void)from;level_changes++;PlayerLevel=to;GrSq=-1;return 1; }
void settings_enhancement(int b,int t,void (*fn)(int)) { CHECK((b==ENH_FULL_MAP || b==ENH_GOD_MODE || b==ENH_BRIGHT_VIEW) && t==SET_TAB_CHEATS && fn); }
static int recorded;
void rp_cheat_put(uint32_t poll,const char *s) { CHECK(poll>0 && !strncmp(s,"menu-",5)); recorded++; }
static const struct cheat_game test_game={NULL,0,ready};
/* A synthetic archive containing only tile and texture data. */
FILE *bc_fopen(const char *name,const char *mode)
{
    FILE *f;unsigned char b[20000];int i;(void)mode;
    memset(b,0,sizeof b);
    if (!strcmp(name,"SAVE0\\LEV.ARK")) {
        b[0]=40;b[2]=200; /* level 1 block */
        b[2+18*4]=0xD0;b[3+18*4]=0x41; /* texture block at 16848 */
        for(i=0;i<4096;i++)b[200+i*4]=1;
        b[200+(8+8*64)*4]=0x81; /* height 8, open */
        b[200+(9+8*64)*4+1]=0x80; /* door */
        b[16848+48*2]=1;
    } else if (!strcmp(name,"DATA\\TERRAIN.DAT")) b[512+2]=TERR_WATER;
    else return NULL;
    f=tmpfile();fwrite(b,1,sizeof b,f);rewind(f);return f;
}
size_t bc_fread(void *p,size_t sz,size_t n,FILE *f) {return fread(p,sz,n,f);}
int bc_fseek(FILE *f,long n,int w) {return fseek(f,n,w);}
int bc_fclose(FILE *f) {return fclose(f);}
unsigned char PlayersMap[64][64];
int main(int argc,char **argv)
{
    int old_calls,i;
    actor.hp=35;PN.x=10*256+128;PN.y=10*256+128;PN.terrain=FOOT_FLOOR;GrSq=10+10*64;
    for(i=0;i<4096;i++)world[i].type=TILE_OPEN;
    SET_HOMEX(ThePlayer,10);SET_HOMEY(ThePlayer,10);
    port_god_world_loaded();
    if(argc>2) {
        cheat_init(&test_game);port_cheat_menu_init();
        CHECK(cheat_from_recording(argv[2]) == (!strcmp(argv[1],"public") ? 0 : -1));
        if(!strcmp(argv[1],"public")) { port_cheat_point();CHECK(ENHANCED(ENH_GOD_MODE));port_cheat_point();CHECK(placements==1);port_cheat_point();CHECK(port_bright_strength()==15); }
        else CHECK(!ENHANCED(ENH_GOD_MODE));
        printf("%s menu-only replay permission gate\n",failures?"FAIL":"PASS");return failures!=0;
    }
    CHECK(!port_god_protect(ThePlayer) && port_god_hp(35,2)==2);
    enhance_on=1u<<ENH_GOD_MODE;CHECK(port_god_protect(ThePlayer) && !port_god_protect(&held));
    for(i=0;i<35;i++) CHECK(port_god_hp(35,i)==35);
    CHECK(port_god_hp(35,40)==40);
    /* Startup with god mode already enabled: loading's unlinked sentinel is normal.
       The original physics must get to link the exact saved position itself. */
    {
        struct Phys saved = PN;
        port_god_reset();in_game=0;scroll=NULL;GrSq=-1;
        CHECK(port_god_motion());port_god_tick();
        CHECK(!placements && !messages && !collision_calls && !memcmp(&PN,&saved,sizeof PN));
        port_god_world_loaded();CHECK(port_god_motion());port_god_tick();
        in_game=1;CHECK(port_god_motion());port_god_tick();
        CHECK(!placements && !messages && !collision_calls && !anchor[1].valid && !memcmp(&PN,&saved,sizeof PN));
        scroll=&message_scroll;port_god_tick();
        CHECK(!placements && !messages && !anchor[1].valid && !memcmp(&PN,&saved,sizeof PN));
        GrSq=10+10*64;
    }
    port_god_tick();CHECK(anchor[1].valid && anchor[1].x==10);
    old_calls=collision_calls;port_god_tick();CHECK(collision_calls==old_calls);
    PN.x=-1;PN.vel[2]=-500;PN.acc[2]=-4;NewPlayerX=50;
    CHECK(!port_god_motion() && placements==1 && PN.x==10*256+128 && !PN.vel[2] && !PN.acc[2] && !NewPlayerX);
    occupied=1;PN.z=-9;CHECK(!port_god_motion() && placements==2 && PN.x!=10*256+128);occupied=0;
    /* A long but valid fall, flight or levitation does not trigger a rescue timer. */
    PN.z=500;PN.vel[2]=-200;PN.acc[2]=-4;old_calls=placements;
    for(i=0;i<100;i++)CHECK(port_god_motion());CHECK(placements==old_calls);
    port_god_bad_motion();CHECK(!port_god_motion() && placements==old_calls+1);
    port_god_teleport_begin();PlayerLevel=2;GrSq=-1;
    CHECK(port_god_teleport_failed() && PlayerLevel==1 && level_changes==1 && GrSq>=0);
    CHECK(!port_god_teleport_valid(10,10,10) && !port_god_teleport_valid(1,0,10));
    no_floor=1;PN.x=-20;CHECK(!port_god_motion() && blocked);
    old_calls=collision_calls;CHECK(!port_god_motion() && collision_calls==old_calls);no_floor=0;
    CHECK(port_god_teleport_failed() && !blocked);
    port_god_reset();CHECK(!anchor[1].valid && !recent[1].valid);
    /* The public queue does not expose arbitrary test commands, and waits for the game. */
    cheat_init(&test_game);port_cheat_menu_init();port_cheat_settings();port_god_world_loaded();
    CHECK(cheat_add("menu-god 1")!=NULL); /* test-script gate still applies */
    scrnum=2;CHECK(!cheat_menu_request("menu-god 0"));port_cheat_point();CHECK(ENHANCED(ENH_GOD_MODE));
    scrnum=0;port_cheat_point();CHECK(!ENHANCED(ENH_GOD_MODE) && recorded==1);
    CHECK(!cheat_menu_request("give 1"));port_cheat_point();CHECK(recorded==1);
    CHECK(!cheat_menu_request("menu-god 1"));port_cheat_point();CHECK(ENHANCED(ENH_GOD_MODE) && recorded==2);
    /* Full map is independent of exploration, restores random state, checks floor/door types. */
    memset(PlayersMap,7,sizeof PlayersMap);
    enhance_on=0;CHECK(!port_full_map_begin(2) && port_full_map_byte(8,8)==7);
    enhance_on=1u<<ENH_FULL_MAP;PlayerLevel=2; /* draw level 1 from the archive */
    CHECK(port_full_map_begin(1) && port_full_map_byte(8,8)==(1|TERR_WATER));
    CHECK(port_full_map_byte(9,8)==(1|TERR_WATER|0x40));seed=999;
    port_full_map_end();CHECK(seed==123 && port_full_map_byte(8,8)==7);
    for(i=0;i<4096;i++)CHECK(((unsigned char *)PlayersMap)[i]==7);
    CHECK(!port_full_map_begin(0) && !port_full_map_begin(10));
    /* Keep native colour maps and light-source differences; rebuild the actual
       game's distance curve, preserve invisible cells and restore before picking. */
    for(i=0;i<4096;i++) lights[i]=(unsigned char)(i*19+i/256*11);
    memcpy(original_lights,lights,sizeof lights);
    enhance_on=0;port_bright_begin();CHECK(!memcmp(lights,original_lights,sizeof lights));port_bright_end();
    CHECK(!cheat_menu_request("menu-bright 1"));PlayerLevel=1;port_cheat_point();CHECK(ENHANCED(ENH_BRIGHT_VIEW));
    {
        static const int levels[8][4]={{56,5,-3,3},{46,4,-3,3},{39,2,-2,4},{30,1,0,4},
                                     {28,0,0,5},{21,0,0,6},{18,0,0,7},{18,0,0,7}};
        struct Gloc original[17][33]; int level,x,y,last,first;
        for(level=0;level<8;level++) {
            smooth_div=levels[level][0];smooth_base=levels[level][1];smooth_lowpass=levels[level][2];curvrad=levels[level][3];
            preset_grid(curvrad);
            for(y=0;y<17;y++)for(x=0;x<33;x++) {glocs[y][x].flags=0x87;glocs[y][x].shade|=0x50;}
            memcpy(original,glocs,sizeof glocs);
            enhance_on=1u<<ENH_BRIGHT_VIEW;port_bright_begin();
            CHECK(smooth_div>0 && smooth_div<levels[level][0] && curvrad==levels[level][3]);
            CHECK(!memcmp(lights,original_lights,sizeof lights));
            for(y=0;y<17;y++)for(x=0;x<33;x++) {
                CHECK((glocs[y][x].shade&15)==15 ? (original[y][x].shade&15)==15 : (original[y][x].shade&15)!=15);
                CHECK((glocs[y][x].shade&15)<=(original[y][x].shade&15));
                CHECK((glocs[y][x].shade&0xF0)==0x50 && glocs[y][x].flags==0x87);
            }
            first=last=glocs[0][16].shade&15;
            for(x=1;x<=curvrad;x++) {CHECK((glocs[0][16+x].shade&15)>=last);last=glocs[0][16+x].shade&15;}
            CHECK(last>first); /* real distance falloff remains, rather than full brightness */
            port_bright_begin();port_bright_end();CHECK(smooth_div<levels[level][0]);
            enhance_on=0;port_bright_end();CHECK(smooth_div==levels[level][0] && smooth_base==levels[level][1]);
            CHECK(!memcmp(glocs,original,sizeof glocs));
        }
        /* A newly calculated wall-step bit must survive restoration for the pick frame. */
        enhance_on=1u<<ENH_BRIGHT_VIEW;port_bright_begin();glocs[0][16].shade^=0x20;port_bright_end();
        CHECK(glocs[0][16].shade==(original[0][16].shade^0x20));
        /* The curve cache includes low-pass and radius, including special light levels. */
        smooth_lowpass=-5;port_bright_begin();CHECK(glocs[0][17].shade%16==0);port_bright_end();
        curvrad=2;preset_grid(curvrad);port_bright_begin();CHECK((glocs[0][19].shade&15)==15);port_bright_end();
        /* Initial/special views may retain a different grid radius: never change visibility. */
        preset_grid(7);curvrad=2;port_bright_begin();CHECK((glocs[0][19].shade&15)!=15);port_bright_end();
        preset_grid(2);curvrad=7;port_bright_begin();CHECK((glocs[0][19].shade&15)==15);port_bright_end();
    }
    CHECK(!memcmp(lights,original_lights,sizeof lights));port_bright_end();
    CHECK(!cheat_menu_request("menu-bright 0"));port_cheat_point();CHECK(!ENHANCED(ENH_BRIGHT_VIEW));
    /* Coalesce repeated UI changes while paused; execute and record once at a safe point. */
    {
        int before=recorded;
        scrnum=2;
        for(i=0;i<400;i++)port_cheat_brightness_request(i%16);
        port_cheat_brightness_point();port_cheat_point();CHECK(port_bright_strength()==4 && recorded==before);
        scrnum=0;port_cheat_brightness_point();port_cheat_point();CHECK(port_bright_strength()==15 && recorded==before+1);
        port_cheat_brightness_point();port_cheat_point();CHECK(recorded==before+1);
        CHECK(brightness(1,(char *[]){"16"},0)!=NULL && brightness(1,(char *[]){"-1"},0)!=NULL);
        port_cheat_brightness_request(0);port_cheat_brightness_point();port_cheat_point();CHECK(port_bright_strength()==0);
    }
    /* Every user strength preserves visibility and is monotonic, with exact restoration. */
    {
        struct Gloc original[17][33];unsigned char previous[17][33];int amount,x,y,level;
        static const int levels[8][4]={{56,5,-3,3},{46,4,-3,3},{39,2,-2,4},{30,1,0,4},
                                     {28,0,0,5},{21,0,0,6},{18,0,0,7},{18,0,0,7}};
        enhance_on=1u<<ENH_BRIGHT_VIEW;
        for(level=0;level<8;level++) {
            smooth_div=levels[level][0];smooth_base=levels[level][1];smooth_lowpass=levels[level][2];curvrad=levels[level][3];
            preset_grid(curvrad);memcpy(original,glocs,sizeof glocs);
            for(y=0;y<17;y++)for(x=0;x<33;x++)previous[y][x]=glocs[y][x].shade&15;
            for(amount=0;amount<=15;amount++) {
                port_bright_strength_set(amount);port_bright_begin();
                for(y=0;y<17;y++)for(x=0;x<33;x++) {
                    int shade=glocs[y][x].shade&15;
                    CHECK((shade==15)==((original[y][x].shade&15)==15));
                    CHECK(port_bright_map_shade(&glocs[y][x],shade)==(original[y][x].shade&15));
                    CHECK(shade<=previous[y][x]);previous[y][x]=(unsigned char)shade;
                }
                if(!amount)CHECK(!memcmp(glocs,original,sizeof glocs) && smooth_div==levels[level][0]);
                port_bright_end();CHECK(!memcmp(glocs,original,sizeof glocs) && smooth_div==levels[level][0] && smooth_base==levels[level][1]);
            }
        }
        port_bright_strength_set(-1);CHECK(port_bright_strength()==0);
        port_bright_strength_set(99);CHECK(port_bright_strength()==15);
        port_bright_strength_set(4);
    }
    CHECK(seed==123);
    printf("%s %d cheat-menu checks: health, recovery, blocked positions, rollback, queue, full-map, brightness and RNG\n",failures?"FAIL":"PASS",checks);
    return failures!=0;
}
