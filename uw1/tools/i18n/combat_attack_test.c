/* Real AI attack starters and the real game bridge, against controlled AI inputs. */
#include "../../src/port/3d/render.c"
#include "combat.h"
#include "combat_attack_impl.h"
#undef main
#undef exit
#include <stdio.h>

static int checks, failures, random_value, random_calls, no_magic, sight = 1;
#define CHECK(c) do { checks++; if (!(c)) { printf("FAIL %d: %s\n",__LINE__,#c); failures++; } } while (0)
uint32_t enhance_on;
static struct Object objects[NUM_MOBILE];
struct Object *critdata = objects, *ThePlayer = objects + 1, *meptr = objects + 7, *mytarget = objects + 1;
struct Creature Creature[NUM_CREATURES], *mycst = Creature + 1;
struct ComObj ComObjData[512];
static struct Player record;
struct Player *player = &record;
int16 PlayerLevel = 1, scrnum, tdx, tdy, myxpost, myypost;
unsigned char myxpos, myypos;
uint16 txpost, typost, tdistsqr = 1;
char in_game = 1;
unsigned char PickUp;
static struct Inplist inputs;
struct Inplist *inplist = &inputs;
unsigned char seg048[SEG048_SIZE], seg051[SEG051_SIZE], *asm_vga_base;
static uint32_t clock_value = 100;
uint32_t *Time = &clock_value;
int port_zh(void) { return 0; }
const char *port_zh_lit(const char *key) { return key; }
struct Object *Obj_IntTMem(int index) { return objects + index; }
uint8_t *asm_segbase(uint16_t seg) { (void)seg; return seg048; }
uint8_t vga_read(uint16_t off) { (void)off; return 0; }
int bc_rand(void) { random_calls++; return random_value; }
unsigned char deltatotheta(char x, char y) { (void)x; (void)y; return 0; }
unsigned char anti_magic_p(int x,int y) { (void)x; (void)y; return (unsigned char)no_magic; }
unsigned char line_of_sight(int x,int y,int z,int a,int b,int c)
{ (void)x; (void)y; (void)z; (void)a; (void)b; (void)c; return (unsigned char)sight; }
unsigned char look_for_target(char how) { (void)how; return 1; }

static void fresh(int target)
{
    memset(objects, 0, sizeof objects); memset(Creature, 0, sizeof Creature);
    meptr->id = 65; meptr->hp = 30; SET_GTARG(meptr,target); SET_SEQ(meptr,SEQ_STAND);
    mycst->avghit = 20; mycst->attacks[0].prob = 100; mycst->caster = 127;
    enhance_on = 1u << ENH_ENEMY_HEALTH;
    random_calls = random_value = no_magic = 0; sight = 1;
    combat_text_reset(); combat_text_observe(7,65,30,20);
}
static void revealed(int yes)
{
    unsigned char fb[12]; CombatFrame f;
    combat_text_begin(fb,0,0,4,1,0,1,100);
    combat_text_owner(7); combat_text_write(fb+2,4); combat_text_end(); combat_text_read(&f);
    CHECK(f.count == 1 && f.actors[0].engaged == yes);
    CHECK(f.actors[0].hp == 30 && f.actors[0].maximum == 30 && !f.floats && meptr->hp == 30);
}
int main(void)
{
    int target;
    /* All three real launch paths, including attacks that have not hit anything. */
    for(target=1;target<=2;target++) {
        fresh(target); crit_attack(0x40); CHECK(OBJ_SEQ(meptr)==SEQ_ATTACK1 && random_calls==3); revealed(target==1);
        fresh(target); CHECK(crit_magik_attack()); CHECK(OBJ_CAST(meptr)==1 && random_calls==2); revealed(target==1);
        fresh(target); CHECK(crit_missile_attack()); CHECK(OBJ_SEQ(meptr)==SEQ_FIRE && random_calls==1); revealed(target==1);
    }
    fresh(1); enhance_on=1u<<ENH_DAMAGE_NUMBERS; crit_attack(0x40); revealed(0);
    fresh(1); enhance_on|=1u<<ENH_GOD_MODE; crit_attack(0x40); revealed(1);
    /* Failed launch rolls, blocked sight, defensive casting and neutral facing do not reveal. */
    fresh(1); random_value=1; crit_attack(0x40); revealed(0);
    fresh(1); mycst->caster=0; CHECK(crit_magik_attack()); revealed(0);
    fresh(1); random_value=1; CHECK(crit_missile_attack()); revealed(0);
    fresh(1); sight=0; CHECK(!crit_missile_attack()); revealed(0);
    fresh(1); no_magic=1; CHECK(!crit_magik_attack()); revealed(0);
    fresh(1); CHECK(maybe_cast_defensive_spell()); CHECK(OBJ_CAST(meptr)==3); revealed(0);
    fresh(1); revealed(0);
    /* The bridge cannot mutate the game object or accept invalid/dead/non-creature actors. */
    fresh(1); { struct Object saved=*meptr; port_combat_attack(meptr); CHECK(!memcmp(&saved,meptr,sizeof saved) && !random_calls); } revealed(1);
    fresh(1); meptr->hp=0; port_combat_attack(meptr); meptr->hp=30; revealed(0);
    fresh(1); meptr->id=1; port_combat_attack(meptr); meptr->id=65; revealed(0);
    fresh(1); port_combat_attack(NULL); port_combat_attack(ThePlayer); port_combat_attack(objects+NUM_MOBILE); revealed(0);
    printf("%s %d attack-bar checks: real melee/missile/spell launch, target, flags, defense, no damage or RNG changes\n",failures?"FAIL":"PASS",checks);
    return failures!=0;
}
