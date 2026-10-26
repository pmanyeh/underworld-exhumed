/* Real wear, item damage and combat rolls. World/feedback services are controlled. */
#include "compat.h"
#include "combat.h"
#include "gfx.h"
#include "inv.h"
#include "player.h"
#include "sys.h"
#include "ui.h"
#include "i18n/phrases.h"
#include <string.h>
#undef main
#undef exit
#include <stdio.h>

static int checks, failures, removals, punts, debris_count, messages, updates, fixes, trap_calls;
static int random_calls, dice_calls, roll_result, random_value, flashes;
#define CHECK(c) do { checks++; if (!(c)) {printf("FAIL %d: %s\n",__LINE__,#c);failures++;} } while(0)
uint32_t enhance_on;
static struct Object objects[272], *slots[28];
struct Object *critdata=objects, *ThePlayer=objects+1;
struct Creature Creature[NUM_CREATURES];
struct ComObj ComObjData[512];
static struct Player record;
struct Player *player=&record;
int16 askill,damage,hitloc,hitobj;
unsigned char hitangle;
signed char cmbModTH[4];
char criti;
struct Object *AskInventory(int slot) {return slot>=0 && slot<28?slots[slot]:NULL;}
struct Object *Obj_IntTMem(int slot) {return objects+slot;}
unsigned char IsMobElem(struct Object *obj) {return obj>=objects && obj<objects+NUM_MOBILE;}
int port_god_protect(const void *obj) {(void)obj;return 0;}
int bc_rand(void) {random_calls++;return random_value;}
int rollem(int n,int sides) {(void)sides;dice_calls++;return n;}
int skill_check(int a,int d) {(void)a;(void)d;return roll_result;}
void fill_FB(int colour) {(void)colour;flashes++;}
char InvRemoveOneObject(struct Object *obj) {int i;removals++;for(i=0;i<28;i++)if(slots[i]==obj)slots[i]=NULL;return 1;}
struct Object *Obj_Punt(union Link *head,struct Object *obj,char force) {(void)head;(void)force;punts++;return obj;}
struct Object *CreateObj(int item,char mobile) {(void)mobile;debris_count++;objects[271].id=(uint16)item;return objects+271;}
unsigned char near_mob_put_at(struct Object *src,struct Object *obj,int range,unsigned char c)
{(void)src;(void)obj;(void)range;(void)c;return 1;}
void FixPlayerEquips(void) {fixes++;}
void RedisplayInvSlot(int slot) {(void)slot;updates++;}
int scroll_print(char *s) {(void)s;messages++;return 0;}
int get_name(char *s,struct Object *obj,char article,char plural)
{(void)obj;(void)article;(void)plural;strcpy(s,"sword");return 5;}
int port_zh(void) {return 0;}
const char *port_zh_lit(const char *s) {return s;}
const char *port_zh_name(struct Object *o,char a,char p) {(void)o;(void)a;(void)p;return "sword";}
void port_zh_begin(PortZh *z) {z->text[0]=0;}
void port_zh_add(PortZh *z,const char *s) {strcat(z->text,s);}
void checkTrap(struct Object *a,struct Object *b,int how,int x,int y)
{(void)a;(void)b;(void)how;(void)x;(void)y;trap_calls++;}
unsigned char damage_critter(struct Object *o,unsigned char d,struct Object *who)
{(void)who;o->hp=o->hp>d?o->hp-d:0;return !o->hp;}
char remove_object(struct Object *o,struct Object *who,char type,int x,int y)
{(void)o;(void)who;(void)type;(void)x;(void)y;return 1;}
#include "no_wear_impl.h"

static void fresh(int on,int lefty,int item,int slot,int quality)
{
    memset(objects,0,sizeof objects);memset(slots,0,sizeof slots);memset(&record,0,sizeof record);
    memset(Creature,0,sizeof Creature);memset(ComObjData,0,sizeof ComObjData);
    removals=punts=debris_count=messages=updates=fixes=trap_calls=0;
    random_calls=dice_calls=flashes=random_value=0;player->lefty=lefty;
    enhance_on=on?1u<<ENH_NO_WEAR:0;
    objects[256].id=(uint16)item;objects[256].qn.f.quality=(uint16)quality;slots[slot]=objects+256;
    objects[1].id=127;objects[1].hp=35;objects[2].id=65;hitobj=2;hitloc=0;damage=10;askill=30;roll_result=-1;
}
int main(void)
{
    int on,lefty,quality,slot,q;
    const int armour[]={ITEM_LEATHER_CAP,ITEM_LEATHER_VEST,ITEM_LEATHER_GLOVES,ITEM_LEATHER_LEGGINGS,ITEM_LEATHER_BOOTS};
    /* All conditions including nearly broken items, both hands, each armour slot. */
    for(on=0;on<=1;on++)for(lefty=0;lefty<=1;lefty++)for(quality=1;quality<=63;quality++) {
        fresh(on,lefty,ITEM_LONGSWORD,8-lefty,quality);
        CHECK(DamageInventory(8-lefty,2,DMG_PHYSICAL,0,1)==(on?-1:quality<=2?1:0));
        CHECK(objects[256].qn.f.quality==(on?quality:quality>2?quality-2:0));
        if(on)CHECK(!removals&&!punts&&!debris_count&&!messages&&!updates&&!fixes);
        for(slot=0;slot<=4;slot++) {
            fresh(on,lefty,armour[slot],slot,quality);
            CHECK(DamageInventory(slot,2,DMG_PHYSICAL,1,1)==(on?-1:quality<=2?1:0));
            CHECK(objects[256].qn.f.quality==(on?quality:quality>2?quality-2:0));
        }
        fresh(on,lefty,ITEM_WOODEN_SHIELD,7+lefty,quality);
        CHECK(DamageInventory(7+lefty,2,DMG_PHYSICAL,1,1)==(on?-1:quality<=2?1:0));
        CHECK(objects[256].qn.f.quality==(on?quality:quality>2?quality-2:0));
    }
    /* Correct return contracts, intentional inventory damage, and world objects unchanged. */
    fresh(1,0,ITEM_LONGSWORD,8,20);CHECK(DamageInventory(6,2,4,0,1)==-2);
    CHECK(DamageInventory(8,2,4,1,1)==-2);CHECK(DamageInventory(8,2,4,2,1)==0 && objects[256].qn.f.quality==18);
    CHECK(damage_item(objects+256,NULL,10,10,255,4)==1 && objects[256].qn.f.quality==0 && trap_calls==1);
    fresh(0,0,ITEM_LONGSWORD,8,1);CHECK(DamageInventory(8,2,4,0,1)==1);
    CHECK(removals==1 && punts==1 && debris_count==1 && messages==1 && updates==1 && fixes==1);
    /* Real critical hits/misses/door bashing still roll and report the same result. */
    for(on=0;on<=1;on++)for(lefty=0;lefty<=1;lefty++) {
        fresh(on,lefty,ITEM_LONGSWORD,8-lefty,20);q=frp_check(1,2);CHECK(q==2 && dice_calls==1);
        CHECK(objects[256].qn.f.quality==(on?20:18));
        fresh(on,lefty,ITEM_LONGSWORD,8-lefty,20);objects[3].id=0x141;q=frp_check(1,3);
        CHECK(q==0 && dice_calls==1 && random_calls==1 && objects[256].qn.f.quality==(on?20:18));
        fresh(on,lefty,ITEM_WOODEN_SHIELD,7+lefty,20);roll_result=2;hitobj=1;q=frp_check(2,1);
        CHECK(q==0 && criti==1 && flashes==1 && dice_calls==1 && random_calls==1);
        CHECK(objects[256].qn.f.quality==(on?20:18) && ThePlayer->hp==35);
    }
    printf("%s %d no-wear checks: actual wear/damage/combat, 63 conditions, hands, armour, shield, destruction and RNG\n",failures?"FAIL":"PASS",checks);
    return failures!=0;
}
