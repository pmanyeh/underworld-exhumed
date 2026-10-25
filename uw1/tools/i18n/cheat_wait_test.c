#include <stdio.h>
#define PIN_NONE 0
static int enabled,calls,limit,stop_after,PlayerInput,fiz_update;
static struct { int speed,vel[3],acc[3]; } PN;
#define ENH_GOD_MODE 0
#define ENHANCED(i) enabled
static void move_player(int dt) { (void)dt;if(++calls==stop_after)PN.speed=0; }
static void port_god_wait_limit(void) { limit++;PN.speed=0; }
#include "god_wait_impl.h"
int main(void)
{
    enabled=1;PN.speed=1;stop_after=100;finish_player();if(calls!=100 || limit)return 1;
    calls=0;PN.speed=1;stop_after=0;finish_player();if(calls!=4096 || limit!=1)return 2;
    enabled=0;calls=limit=0;PN.speed=1;stop_after=5000;finish_player();if(calls!=5000 || limit)return 3;
    puts("PASS real finish_player: normal flight settles, abnormal wait is bounded, disabled behavior preserved");
    return 0;
}
