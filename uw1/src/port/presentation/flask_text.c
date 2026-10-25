#include "flask_text.h"
#include <stdatomic.h>
#include <string.h>

static int enabled;
static FlaskFrame work, published;
static atomic_flag busy = ATOMIC_FLAG_INIT;
static uint32_t born[FLASK_FLOATS], last_now;
static int known;                       /* work.value holds a value to compare the next with */

static void lock(void) { while (atomic_flag_test_and_set_explicit(&busy, memory_order_acquire)) {} }
static void unlock(void) { atomic_flag_clear_explicit(&busy, memory_order_release); }

void flask_text_enable(int on) { enabled = on != 0; }
int flask_text_enabled(void) { return enabled; }

static void add_float(int which, int delta, uint32_t now)
{
    int i, oldest = 0;
    if (work.count < FLASK_FLOATS) i = work.count++;
    else {
        for (i = 1; i < FLASK_FLOATS; i++) if (born[i] < born[oldest]) oldest = i;
        i = oldest;
    }
    work.floats[i].which = which;
    work.floats[i].delta = delta;
    work.floats[i].age = 0;
    born[i] = now;
}

void flask_text_update(int hp, int max_hp, int mana, int max_mana, int poisoned, uint32_t now)
{
    int v[2], i, k;
    if (!enabled) return;
    v[0] = hp; v[1] = mana;
    /* a clock that went back is a game restored or started: no floats from the old values */
    if (now < last_now) { known = 0; work.count = 0; }
    last_now = now;
    for (i = 0; i < 2; i++) {
        if (known && v[i] != work.value[i]) add_float(i, v[i] - work.value[i], now);
        work.value[i] = v[i];
    }
    known = 1;
    work.max[0] = max_hp; work.max[1] = max_mana;
    work.poisoned = poisoned != 0;
    for (i = k = 0; i < work.count; i++) {
        uint32_t age = now - born[i];
        if (age >= FLASK_FLOAT_LIFE) continue;
        work.floats[k] = work.floats[i];
        work.floats[k].age = (int)age;
        born[k++] = born[i];
    }
    work.count = k;
    work.active = 1;
    work.generation++;
    lock(); published = work; unlock();
}

void flask_text_read(FlaskFrame *out) { if (out) { lock(); *out = published; unlock(); } }
