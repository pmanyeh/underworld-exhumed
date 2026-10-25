/* Exercise the real menu/recovery/store code with a synthetic world and collision answers. */
#include "../../src/port/ui/cheat_menu.c"
#include "../../src/port/ui/full_map.c"
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
#define CHECK(c) do { checks++; if (!(c)) { printf("FAIL %d: %s\n",__LINE__,#c); failures++; } } while (0)
uint32_t enhance_on;
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
int scroll_print(char *s) { (void)s; return 0; }
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
void settings_enhancement(int b,int t,void (*fn)(int)) { CHECK((b==ENH_FULL_MAP || b==ENH_GOD_MODE) && t==SET_TAB_CHEATS && fn); }
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
        if(!strcmp(argv[1],"public")) { port_cheat_point();CHECK(ENHANCED(ENH_GOD_MODE));port_cheat_point();CHECK(placements==1); }
        else CHECK(!ENHANCED(ENH_GOD_MODE));
        printf("%s menu-only replay permission gate\n",failures?"FAIL":"PASS");return failures!=0;
    }
    CHECK(!port_god_protect(ThePlayer) && port_god_hp(35,2)==2);
    enhance_on=1u<<ENH_GOD_MODE;CHECK(port_god_protect(ThePlayer) && !port_god_protect(&held));
    for(i=0;i<35;i++) CHECK(port_god_hp(35,i)==35);
    CHECK(port_god_hp(35,40)==40);
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
    printf("%s %d cheat-menu checks: health, recovery, blocked positions, rollback, queue, full-map and RNG\n",failures?"FAIL":"PASS",checks);
    return failures!=0;
}
