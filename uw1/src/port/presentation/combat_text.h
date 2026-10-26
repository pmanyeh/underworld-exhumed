/* Display-only combat state. The game owns the producer; presentation reads copies.
   No game pointers enter a snapshot, and none of this state is saved. */
#ifndef UW1_COMBAT_TEXT_H
#define UW1_COMBAT_TEXT_H
#include <stdint.h>
#include <stddef.h>
#define COMBAT_SLOTS 256
#define COMBAT_FLOATS 32
#define COMBAT_LIFE 384u
enum { COMBAT_DAMAGE, COMBAT_MISS, COMBAT_BLOCK };
typedef struct CombatActor {
    int slot, hp, maximum, left, top, right, bottom, engaged;
} CombatActor;
typedef struct CombatFloat {
    int slot, kind, amount, age, lane;
    char text[24];
} CombatFloat;
typedef struct CombatFrame {
    uint64_t generation;
    int numbers, health, count, floats;
    int left, top, right, bottom;
    CombatActor actors[COMBAT_SLOTS];
    CombatFloat events[COMBAT_FLOATS];
} CombatFrame;
void combat_text_reset(void);
void combat_text_hide(void);
void combat_text_forget(int slot);
void combat_text_observe(int slot, int identity, int hp, int average);
/* A creature that took damage or began attacking the player may show its health. */
void combat_text_engage(int slot);
void combat_text_event(int slot, int kind, int amount, const char *text, uint32_t now);
/* framebuffer rows count up; screen rows count down. y is the view's bottom row. */
void combat_text_begin(const void *fb, int x, int y, int width, int height,
                       int numbers, int health, uint32_t now);
void combat_text_owner(int slot);
void combat_text_write(const void *address, size_t length);
void combat_text_end(void);
void combat_text_read(CombatFrame *out);
/* Fast range test for the optional assembly memory-write observer. Game thread only. */
extern uintptr_t combat_write_base;
extern size_t combat_write_size;
struct HdFont;
struct PlatOverlay;
int combat_text_draw(struct HdFont *font, const struct PlatOverlay *frame,
                     const CombatFrame *combat, float border);
/* Game bridge: ASCII hooks in the original sources. */
void port_combat_damage(const void *object, int damage);
void port_combat_attack(const void *object);
void port_combat_result(int target, int attacker, int kind);
void port_combat_forget(const void *object);
void port_combat_tags_reset(void);
void port_combat_sprite(const void *opcode);
void port_combat_render_begin(int x, int y, int w, int h);
#endif
