/* The exact native rp_cheat_put function, extracted by the runner from replay.c. */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
typedef uint32_t uint32;
#define RP_RECORD 1
#define S_CHEAT 10
#define HDR_LEN 12
static int rp_mode=RP_RECORD,log_fd;
#include "menu_record_impl.h"
int main(void)
{
    const int sizes[]={0,1,10,4095,4096,4097,32768,1000000};
    int v,i,k,n,shift;
    unsigned char head[12],*before,*after;
    FILE *f;
    for(v=4;v<=6;v++)for(i=0;i<8;i++) {
        n=sizes[i]+(v>4?3:0);shift=v==4?3:0;
        before=malloc(n+1);after=malloc(n+100);
        for(k=0;k<n;k++)before[k]=(unsigned char)(k*31+5);
        if(v>4){before[0]=9;before[1]=before[2]=0;}
        memset(head,0,sizeof head);memcpy(head,"UWRP",4);head[4]=(unsigned char)v;
        f=tmpfile();if(!f)return 1;log_fd=fileno(f);
        if(write(log_fd,head,12)!=12 || write(log_fd,before,n)!=n)return 2;
        rp_cheat_put(0x12345678u,"menu-god 1");
        rp_cheat_put(0x12345679u,"menu-rescue");
        lseek(log_fd,0,0);if(read(log_fd,head,12)!=12 || head[4]!=6)return 3;
        if(read(log_fd,after,n+shift)!=n+shift)return 4;
        if(v==4 && (after[0]!=9 || after[1] || after[2]))return 5;
        if(memcmp(after+shift,before,n))return 6;
        if(read(log_fd,after,17)!=17 || after[0]!=10 || after[1]!=14 || after[3]!=0x78 || after[6]!=0x12 || memcmp(after+7,"menu-god 1",10))return 7;
        if(read(log_fd,after,18)!=18 || after[0]!=10 || after[1]!=15 || after[3]!=0x79 || memcmp(after+7,"menu-rescue",11))return 8;
        if(read(log_fd,after,1)!=0)return 9;
        fclose(f);free(before);free(after);
    }
    puts("PASS 24 recording promotions: versions 4/5/6, empty and multi-block tails, repeat actions and exact bytes");
    return 0;
}
