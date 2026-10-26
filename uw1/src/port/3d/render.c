/* A separate pixel ownership buffer follows the renderer's writes. Later walls and
   sprites erase ownership as they paint; neither bytecode nor framebuffer is changed. */
#include "compat.h"
#include "object.h"
#include "player.h"
#include "sys.h"
#include "ui.h"
#include "event.h"
#include "view3d.h"
#include "x86/asmrt.h"
#include "presentation/combat_text.h"
#include "i18n/phrases.h"

static uint16_t tags[32768];
static int on(void) { return ENHANCED(ENH_DAMAGE_NUMBERS) || ENHANCED(ENH_ENEMY_HEALTH); }
static int slot_of(const void *p)
{
    uintptr_t off = (uintptr_t)p - (uintptr_t)critdata;
    if (off >= sizeof(struct Object) * NUM_MOBILE || off % sizeof(struct Object)) return 0;
    return (int)(off / sizeof(struct Object));
}
void port_combat_tags_reset(void) { if (on()) memset(tags, 0, sizeof tags); }
void port_render_tag(const void *object, const void *db)
{
    const struct Object *o = object;
    uintptr_t off = (uintptr_t)db - (uintptr_t)seg051;
    if (!on() || off >= sizeof tags || off % 2) return;
    tags[off / 2] = OBJ_MAJOR(o) == MAJOR_CREATURE ? (uint16_t)slot_of(o) : 0;
}
void port_combat_sprite(const void *opcode)
{
    uintptr_t off = (uintptr_t)opcode - (uintptr_t)seg051;
    combat_text_owner(on() && off < sizeof tags && off % 2 == 0 ? tags[off / 2] : 0);
}
void port_combat_damage(const void *object, int damage)
{
    const struct Object *o = object;
    int slot;
    if (!on() || !o || o == ThePlayer || damage <= 0 || (slot = slot_of(o)) < 2) return;
    combat_text_observe(slot, OBJ_ITEM(o), o->hp, Creature[OBJ_INMAJOR(o)].avghit);
    /* Display time must not consume a recorded GAME_TIME() event. */
    combat_text_event(slot, COMBAT_DAMAGE, damage < o->hp ? damage : o->hp, NULL, *Time);
}
/* Called only when an offensive melee, missile or spell animation starts.
   No damage event: misses, blocks and god mode must still reveal the attacker. */
void port_combat_attack(const void *object)
{
    const struct Object *o = object;
    int slot;
    if (!ENHANCED(ENH_ENEMY_HEALTH) || !o || (slot = slot_of(o)) < 2 ||
        OBJ_MAJOR(o) != MAJOR_CREATURE || !o->hp || OBJ_GTARG(o) != 1) return;
    combat_text_observe(slot, OBJ_ITEM(o), o->hp, Creature[OBJ_INMAJOR(o)].avghit);
    combat_text_engage(slot);
}
void port_combat_result(int target, int attacker, int kind)
{
    struct Object *o;
    const char *text;
    if (!ENHANCED(ENH_DAMAGE_NUMBERS) || attacker != 1) return;
    /* A swing into empty space must never reuse COMBAT.C's previous hitobj. */
    if (kind == COMBAT_MISS) target = 0;
    else if (target <= 1 || target >= NUM_MOBILE || OBJ_MAJOR(Obj_IntTMem(target)) != MAJOR_CREATURE) return;
    if (target) {
        o = Obj_IntTMem(target);
        combat_text_observe(target, OBJ_ITEM(o), o->hp, Creature[OBJ_INMAJOR(o)].avghit);
    }
    text = kind == COMBAT_MISS ? (port_zh() ? port_zh_lit("combat.miss") : "Miss")
                               : (port_zh() ? port_zh_lit("combat.block") : "Block");
    combat_text_event(target, kind, 0, text, *Time);
}
void port_combat_forget(const void *object) { if (on()) combat_text_forget(slot_of(object)); }
void port_combat_render_begin(int x, int y, int w, int h)
{
    int i;
    if (!on() || !in_game || scrnum != 0 || (inplist->mode & MODE_CONV)) {
        combat_text_hide();
        return;
    }
    if (PickUp) {
        combat_write_size = 0;
        return;
    }
    for (i = 2; i < NUM_MOBILE; i++) {
        const struct Object *o = Obj_IntTMem(i);
        if (OBJ_MAJOR(o) == MAJOR_CREATURE)
            combat_text_observe(i, OBJ_ITEM(o), o->hp, Creature[OBJ_INMAJOR(o)].avghit);
        else combat_text_forget(i);
    }
    combat_text_begin(asm_segbase(rw(seg048, 0x95A)), x, 199 - y + h - 1, w, h,
                       ENHANCED(ENH_DAMAGE_NUMBERS), ENHANCED(ENH_ENEMY_HEALTH), *Time);
}
