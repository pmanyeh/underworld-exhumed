#include "combat_text.h"
#include <stdatomic.h>
#include <string.h>
#include <stdio.h>

static struct { int identity, known, hp, maximum, engaged; } actors[COMBAT_SLOTS];
static struct { CombatFloat event; uint32_t born; } events[COMBAT_FLOATS];
static int count, owner;
static uint16_t pixels[65536];
static uint32_t clock_now;
static CombatFrame work, published;
static atomic_flag busy = ATOMIC_FLAG_INIT;
uintptr_t combat_write_base;
size_t combat_write_size;
static void lock(void) { while (atomic_flag_test_and_set_explicit(&busy, memory_order_acquire)) {} }
static void unlock(void) { atomic_flag_clear_explicit(&busy, memory_order_release); }
static int valid(int slot) { return slot > 1 && slot < COMBAT_SLOTS; }
static void publish(void) { work.generation++; lock(); published = work; unlock(); }

void combat_text_hide(void)
{
    combat_write_size = 0; owner = 0;
    work.count = work.floats = 0;
    publish();
}
void combat_text_reset(void)
{
    memset(actors, 0, sizeof actors);
    memset(events, 0, sizeof events);
    count = 0; clock_now = 0;
    combat_text_hide();
}
void combat_text_forget(int slot)
{
    int i, k;
    if (!valid(slot)) return;
    memset(&actors[slot], 0, sizeof actors[slot]);
    for (i = k = 0; i < count; i++)
        if (events[i].event.slot != slot) events[k++] = events[i];
    count = k;
}
void combat_text_observe(int slot, int identity, int hp)
{
    if (!valid(slot)) return;
    if (!actors[slot].known || actors[slot].identity != identity) {
        combat_text_forget(slot);
        actors[slot].known = 1;
        actors[slot].identity = identity;
        actors[slot].maximum = hp;
    }
    actors[slot].hp = hp;
    if (hp > actors[slot].maximum) actors[slot].maximum = hp;
    if (actors[slot].maximum < 1) actors[slot].maximum = 1;
}
void combat_text_engage(int slot)
{
    if (!valid(slot) || !actors[slot].known) return;
    /* The render scan can have observed this actor long before combat began.
       Establish the baseline once, at the first trigger (damage observes pre-hit HP). */
    if (!actors[slot].engaged) actors[slot].maximum = actors[slot].hp > 0 ? actors[slot].hp : 1;
    actors[slot].engaged = 1;
}
static void age_events(uint32_t now)
{
    int i, k;
    /* A restored/started game or a wrapped clock cannot retain old combat floats. */
    if (now < clock_now) { count = 0; }
    clock_now = now;
    for (i = k = 0; i < count; i++) {
        uint32_t age = now - events[i].born;
        if (age >= COMBAT_LIFE) continue;
        events[i].event.age = (int)age;
        events[k++] = events[i];
    }
    count = k;
}
void combat_text_event(int slot, int kind, int amount, const char *text, uint32_t now)
{
    int i, lane = 0;
    CombatFloat *e;
    if ((slot && !valid(slot)) || kind < COMBAT_DAMAGE || kind > COMBAT_BLOCK) return;
    if (kind == COMBAT_DAMAGE && amount <= 0) return;
    age_events(now);
    for (i = 0; i < count; i++) if (events[i].event.slot == slot) lane++;
    if (count == COMBAT_FLOATS) {
        memmove(events, events + 1, sizeof events[0] * (COMBAT_FLOATS - 1));
        count--;
    }
    e = &events[count].event;
    memset(e, 0, sizeof *e);
    e->slot = slot; e->kind = kind; e->amount = amount; e->lane = lane % 3;
    snprintf(e->text, sizeof e->text, "%s", text ? text : "");
    events[count++].born = now;
    if (kind == COMBAT_DAMAGE) combat_text_engage(slot);
}
void combat_text_begin(const void *fb, int x, int y, int w, int h,
                       int numbers, int health, uint32_t now)
{
    combat_write_size = 0; owner = 0;
    work.count = work.floats = 0;
    work.numbers = numbers; work.health = health;
    work.left = x; work.top = y - h + 1; work.right = x + w; work.bottom = y + 1;
    age_events(now);
    if (!fb || w <= 0 || w > 320 || h <= 0 || h > 200 || (w + 2) * h + 2 > 65536) return;
    memset(pixels, 0, sizeof pixels);
    combat_write_base = (uintptr_t)fb;
    combat_write_size = (size_t)(w + 2) * h + 2;
}
void combat_text_owner(int slot) { owner = valid(slot) ? slot : 0; }
void combat_text_write(const void *p, size_t length)
{
    size_t off = (uintptr_t)p - combat_write_base, i;
    if (off >= combat_write_size) return;
    if (length > combat_write_size - off) length = combat_write_size - off;
    for (i = 0; i < length; i++) pixels[off + i] = (uint16_t)owner;
}
void combat_text_end(void)
{
    int i, x, y, w = work.right - work.left, h = work.bottom - work.top;
    CombatActor bounds[COMBAT_SLOTS];
    if (!combat_write_size) { combat_text_hide(); return; }
    combat_write_size = 0; owner = 0;
    memset(bounds, 0, sizeof bounds);
    for (y = 0; y < h; y++) for (x = 0; x < w; x++) {
        int slot = pixels[2 + y * (w + 2) + x];
        CombatActor *a = &bounds[slot];
        int sx = work.left + x, sy = work.bottom - 1 - y;
        if (!valid(slot) || !actors[slot].known) continue;
        if (!a->slot) {
            a->slot = slot; a->left = sx; a->right = sx + 1; a->top = sy; a->bottom = sy + 1;
        } else {
            if (sx < a->left) a->left = sx;
            if (sx >= a->right) a->right = sx + 1;
            if (sy < a->top) a->top = sy;
            if (sy >= a->bottom) a->bottom = sy + 1;
        }
    }
    for (i = 2; i < COMBAT_SLOTS; i++) if (bounds[i].slot) {
        CombatActor *a = &work.actors[work.count++];
        *a = bounds[i]; a->hp = actors[i].hp; a->maximum = actors[i].maximum; a->engaged = actors[i].engaged;
    }
    if (work.numbers) for (i = 0; i < count; i++) {
        int slot = events[i].event.slot;
        if (!slot || bounds[slot].slot) work.events[work.floats++] = events[i].event;
    }
    publish();
}
void combat_text_read(CombatFrame *out) { if (out) { lock(); *out = published; unlock(); } }
