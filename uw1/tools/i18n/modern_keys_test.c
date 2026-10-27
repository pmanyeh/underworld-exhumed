/* Real scan-code queue, input reader and movement code, without game assets. */
#include "sys.h"
#include "ui.h"
#include "motion.h"
#include "player.h"
#include "port.h"
#include <stdio.h>
#include <string.h>
#include "../../src/port/sys/keyqueue.c"
#undef main
#undef exit

unsigned char seg063[SEG063_SIZE];
unsigned char *Asc = seg063 + 0x10, *key_on = seg063 + 0x1EB;
unsigned char *Shift = seg063 + 0x1E2, *Alt = seg063 + 0x1E3;
unsigned char *Ctrl = seg063 + 0x1E4, *CapsLock = seg063 + 0x1E8;
uint32_t enhance_on;
unsigned char motionbits;
int16 PlayerInput, ForwInpRate, TurnInpRate;
static struct Inplist input;
struct Inplist *inplist = &input;
static uint32 clock_ticks, key_time;
static int key_index, checks, wheel;
#define CHECK(c) do { checks++; if (!(c)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #c); return 1; } } while (0)

/* The replay boundary carries the real KEY result and its original key-state bytes. */
static struct { int result; unsigned char state[0x89]; } events[2048];
static int recording, replaying, event_count, event_pos;
int rp_key(void)
{
    int result;
    if (replaying) {
        if (event_pos >= event_count) return 0;
        memcpy(Shift, events[event_pos].state, 0x89);
        return events[event_pos++].result;
    }
    result = key();
    if (recording && event_count < 2048) {
        events[event_count].result = result;
        memcpy(events[event_count++].state, Shift, 0x89);
    }
    return result;
}
uint32 rp_time(void) { return clock_ticks; }
int port_wheel_take(void) { int result = wheel; wheel = 0; return result; }
#include "modern_keys_impl.h"

static void reset_input(int modern, int mode)
{
    static const unsigned char scans[] = {0x10,0x11,0x12,0x13,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x29,0x2C,0x2D,0x2E,0x2F,0x30,0x39};
    static const char chars[] = "qwerasdfghj`zxcvb ";
    unsigned i;
    memset(seg063, 0, sizeof seg063);
    for (i = 0; i < sizeof scans; i++) {
        Asc[scans[i]] = (unsigned char)chars[i];
        Asc[scans[i] + 128] = chars[i] >= 'a' && chars[i] <= 'z' ? chars[i] - 'a' + 'A' : chars[i];
    }
    /* Real modifier lookup tables: two scan codes per modifier, layout 2. */
    SETW(0x29B, 0x2A); SETW(0x2A3, 0x36);
    SETW(0x2AB, 0x38); SETW(0x2B3, 0x6B);
    SETW(0x2BB, 0x1D); SETW(0x2C3, 0x6C);
    atomic_store(&write_ptr, 0x300); SETW(0x26C, 0x300); SETW(0x1E0, 1);
    held = held_e0 = last_e0 = 0; held_ms = 0; installed = 0; seg019_598();
    enhance_on = modern ? 1u << ENH_MODERN_KEYS : 0;
    input.mode = mode; key_index = 0; key_time = clock_ticks = 0;
    PlayerInput = ForwInpRate = TurnInpRate = motionbits = 0;
}

int main(void)
{
    int i, n, scan, expected, outputs[600];
    static const int actions[] = {0x10,0x12,0x13,0x21,0x22,0x23,0x24,0x29,0x2C,0x2D,0x2E,0x39};
    for (i = 0; i < (int)(sizeof actions / sizeof actions[0]); i++) {
        reset_input(1, MODE_GAME); scan = actions[i]; expected = Asc[scan];
        kbd_byte((uint8_t)scan);
        CHECK(do_keyboard_input(1) == expected); /* immediate, no 30-tick delay */
        for (n = 0; n < 400; n++) {
            clock_ticks += 92; kbd_tick_ms(92);
            CHECK(do_keyboard_input(1) == -1);
        }
        CHECK(key_on[scan] == 255); /* holding >30s cannot wrap into another press */
        kbd_byte((uint8_t)(scan | 0x80)); kbd_byte((uint8_t)scan);
        CHECK(do_keyboard_input(1) == expected); /* release/repress between polls */
    }
    /* Holding Z through a screen change must not immediately close the map. */
    reset_input(1, MODE_GAME); kbd_byte(0x2C); CHECK(do_keyboard_input(1) == 'z');
    input.mode = MODE_MAP;
    for (i = 0; i < 20; i++) { clock_ticks += 92; kbd_tick_ms(92); CHECK(do_keyboard_input(1) == -1); }
    kbd_byte(0xAC); kbd_byte(0x2C); CHECK(do_keyboard_input(1) == 'z');
    /* Text entry keeps repeats, including a map note's Z and spaces. */
    reset_input(1, MODE_MAP); kbd_byte(0x2C); CHECK(do_keyboard_input(0) == 'z');
    kbd_tick_ms(500); CHECK(do_keyboard_input(0) == 'z');
    /* Rune casting keeps Ctrl+Alt+Space, with the same modifier path. */
    reset_input(1, MODE_GAME); kbd_byte(0x1D); kbd_byte(0x38); kbd_byte(0x39);
    CHECK(do_keyboard_input(1) == -1); CHECK(do_keyboard_input(1) == -1);
    CHECK(do_keyboard_input(1) == (KEY_CTRL | KEY_ALT | ' '));
    /* Repeating controls and the original layout retain the held-key path. */
    reset_input(1, MODE_GAME); kbd_byte(0x2F); clock_ticks = 40;
    CHECK(do_keyboard_input(1) == 'v'); clock_ticks += 40; CHECK(do_keyboard_input(1) == 'v');
    reset_input(0, MODE_GAME); kbd_byte(0x2E); clock_ticks = 40;
    CHECK(do_keyboard_input(1) == 'c'); clock_ticks += 40; CHECK(do_keyboard_input(1) == 'c');
    /* X is exclusively close-container in modern mode; other motion still works. */
    reset_input(1, MODE_GAME); key_on[0x2D] = 1; do_player_keyboard();
    CHECK(PlayerInput == 0 && ForwInpRate == 0 && TurnInpRate == 0);
    key_on[0x11] = 1; do_player_keyboard(); CHECK(PlayerInput == PIN_FORWARD && ForwInpRate == 0x70);
    key_on[0x11] = 0; key_on[0x1F] = 1; do_player_keyboard(); CHECK(PlayerInput == PIN_BACK);
    reset_input(0, MODE_GAME); key_on[0x2D] = 1; do_player_keyboard(); CHECK(PlayerInput == PIN_BACK);
    /* Identical recorded KEY/state bytes must produce identical actions in replay. */
    reset_input(1, MODE_GAME); recording = 1; event_count = 0; kbd_byte(0x2E);
    for (i = 0; i < 600; i++) {
        if (i == 300) { kbd_byte(0xAE); kbd_byte(0x2E); }
        clock_ticks += 92; kbd_tick_ms(92); outputs[i] = do_keyboard_input(1);
    }
    recording = 0; CHECK(outputs[0] == 'c' && outputs[300] == 'c');
    reset_input(1, MODE_GAME); replaying = 1; event_pos = 0;
    for (i = 0; i < 600; i++) { clock_ticks += 92; CHECK(do_keyboard_input(1) == outputs[i]); }
    CHECK(event_pos == event_count);
    printf("PASS %d modern key checks: first press, hold, re-press, map, text, runes, motion, replay\n", checks);
    return 0;
}
