/* Exercise the actual game's cursor state machine with graphics-only doubles.
   No game assets, EMS, platform input, or reconstructed mouse logic in the test. */
#include "../../src/ui/MOUSE.C"
#undef main
#undef exit
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static int checks, screen_draws, fb_draws, saves, restores, pen;
#define CHECK(c) do { checks++; if (!(c)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c); exit(1); } } while(0)
static struct Bitmap bitmap;
static unsigned char bits[63];
static int16 colors[258];
int16 *Color_data_ptr = colors;
uint16 first_vram = 0x7000;
unsigned char ShowClip, Transparency;
static struct Inplist input;
struct Inplist *inplist = &input;
/* Unused input paths remain in the included translation unit on PE/COFF. */
static int16 edges[4], motion[3];
static unsigned char keys[256], mods[4];
static int button_reads;
unsigned char didMouseInput;
int16 *wleft=&edges[0], *wtop=&edges[1], *wright=&edges[2], *wbot=&edges[3];
int16 *MouseDx=&motion[0], *MouseDy=&motion[1], *MouseOn=&motion[2];
unsigned char *key_on=keys, *Asc=keys, *Alt=&mods[0], *Shift=&mods[1], *Ctrl=&mods[2], *CapsLock=&mods[3];
int16 GameInputMode, PlayerFacing, PlayerPitch;
struct Object *ThePlayer;
uint32_t enhance_on;
int port_wheel_take(void) { return 0; }    /* port/ui/keys.c: no wheel notches here */
void set_the_window(int a,int b,int c,int d) { (void)a;(void)b;(void)c;(void)d; }
void do_changes(void) {}
void mouse_look_mode(int on) { (void)on; }
int port_pitch_bound(void) { return 100; }
void editchng(int bits_) { (void)bits_; }
int scroll_print(char *s) { (void)s;return 0; }
int rp_key(void) { return 0; }
void rp_mouse(void) {}
int rp_mbuttons(void) { return button_reads>0 ? button_reads--,1 : 0; }
uint32 rp_time(void) { return 0; }
int grs_which1(int id) { return id; }
void *seg009_1(int slot) { (void)slot; return &bitmap; }
void *grs_unpack(struct Bitmap *p) { CHECK(p==&bitmap); return bits; }
int valloc(int w,int h) { CHECK(w==40 && h==40); return 1; }
void set_the_color(int color) { pen=color; }
void fbuf_setcolor(int color) { pen=color; }
void rectangle(int a,int b,int c,int d) {
    (void)a;(void)b;(void)c;(void)d;
    if(pen==0x100)saves++;
    if(pen==0x101)restores++;
}
void vcopyfb(int a,int b,int c,int d,int h) { (void)a;(void)b;(void)c;(void)d;(void)h;saves++; }
void pic_to_screen(int id,int x,int y,int h,int w) {
    (void)id; CHECK(x==mouse_x-hotspot_x && y==mouse_y+hotspot_y && h==9 && w==7);screen_draws++;
}
void pic_to_fbuf(int id,int x,int y) {
    (void)id;CHECK(x==mouse_x-hotspot_x-m3dx && y==mouse_y+hotspot_y-m3dy+m3dh-1);fb_draws++;
}
int main(void)
{
    HdCursor c; int region,n;
    bitmap.width=7;bitmap.height=9;memset(bits,2,sizeof bits);input.mode=MODE_GAME;
    hd_cursor_enable(0);CHECK(init_mouse()==0);mouse_show();draw3dMouse();
    CHECK(screen_draws==1 && fb_draws==1);
    hd_cursor_read(&c);
    CHECK(!c.visible && c.tracked && c.box[0]==97 && c.box[2]==106);
    CHECK(c.box[1]==93 && c.box[3]==105);
    mouse_hide();hd_cursor_read(&c);CHECK(!c.tracked);
    hd_cursor_enable(1);mouse_show();hd_cursor_read(&c);
    CHECK(c.visible && c.picture==ICON_CURSORS && c.x==100 && c.y==99);
    CHECK(c.hotspot_x==2 && c.hotspot_y==4 && c.width==7 && c.height==9 && c.pixels[62]==2);
    n=saves;mouse_show();mouse_hide();hd_cursor_read(&c);CHECK(c.visible && saves==n);
    mouse_hide();hd_cursor_read(&c);CHECK(!c.visible && restores>0 && !mouse_saved);
    mouse_show();region=defineMouseRegion(90,90,110,110,0x1077);drawMouse();hd_cursor_read(&c);
    CHECK(region==0 && c.picture==0x1077);
    force_mouse_cursor(0xa0);hd_cursor_read(&c);CHECK(c.picture==0xa0 && force_depth==1);
    force_mouse_cursor(0x1078);force_mouse_cursor(0x1079);force_mouse_cursor(0x107a);
    hd_cursor_read(&c);CHECK(force_depth==3 && c.picture==0x1079);
    unforce_mouse_cursor(3);hd_cursor_read(&c);CHECK(c.picture==0x1078);
    unforce_mouse_cursor(3);hd_cursor_read(&c);CHECK(c.picture==0xa0);
    unforce_mouse_cursor(3);hd_cursor_read(&c);CHECK(c.picture==0x1077 && force_depth==0);
    undefineMouseRegion(region);drawMouse();hd_cursor_read(&c);CHECK(c.picture==ICON_CURSORS);
    unforce_mouse_cursor(3);CHECK(force_depth==0);
    mouse_x=319;mouse_y=0;input.mode=4;m3dt=2;draw3dMouse();hd_cursor_read(&c);
    CHECK(c.visible && c.path==1 && c.x==319 && c.y==199 && c.mode==4 && mouse_saved);
    m3dt=1;input.mode=MODE_GAME;draw3dMouse();CHECK(mouse_saved);
    CHECK(screen_draws==1 && fb_draws==1); /* HD never enters either low-resolution raster */
    mouse_constrain(10,20,15,25);*MouseOn=1;*MouseDx=-1000;*MouseDy=1000;moveMouse();
    hd_cursor_read(&c);CHECK(mouse_x==10 && mouse_y==20 && c.x==10 && c.y==179);
    mouse_freereign();mouse_x=100;mouse_y=100;*MouseDx=8;*MouseDy=0;button_reads=2;
    CHECK(mouse_dragged(0)==1 && mouse_x==108);
    button_reads=0;CHECK(mouse_dragged(0)==0);
    mouse_hide();hd_cursor_read(&c);CHECK(!c.visible && !mouse_saved);
    printf("PASS actual MOUSE.C: %d assertions; nesting, save-under, regions, force stack, object, both raster paths\n",checks);
    return 0;
}
