#include <stdio.h>
#include <stdint.h>
#define far
typedef uint16_t uint16;
static struct Object { int hp,x,y,z,fx,fy,target; } target,source,*meptr=&source,*mytarget;
static int txpos,typos,tzpos,txpost,typost,tdx,tdy,tdistsqr,tdisttsqr,myxpos,myypos,myxpost,myypost;
#define OBJ_GTARG(o) ((o)->target)
#define OBJ_HOMEX(o) ((o)->x)
#define OBJ_HOMEY(o) ((o)->y)
#define OBJ_Z(o) ((o)->z)
#define OBJ_FINEX(o) ((o)->fx)
#define OBJ_FINEY(o) ((o)->fy)
static struct Object *Obj_IntTMem(int n) { return n?&target:NULL; }
#include "god_target_impl.h"
int main(void)
{
    if(set_up_target())return 1;
    source.target=1;if(set_up_target())return 2;
    target.hp=10;target.x=2;target.y=3;target.fx=4;
    if(!set_up_target() || txpost!=20 || typost!=24 || tdistsqr!=13)return 3;
    puts("PASS real set_up_target: missing, dead and live targets");return 0;
}
