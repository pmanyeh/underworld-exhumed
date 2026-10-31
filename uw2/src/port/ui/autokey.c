/* autokey.c: --enhance auto-key (docs/ENHANCEMENTS.md), as UW1's. Using a locked door or
   container with the key that fits it anywhere in the inventory, a container's too, unlocks it
   as using that key on it does (USEITEMS.C's UseKeyOn: checkLock with the key's owner field,
   which sets off the lock's trap the same way), says so, and the use goes on to open it. Port
   only; called from obj/USEITEMS.C's UseRect and UseCont. */
#include <string.h>
#include "compat.h"
#include "ui.h"
#include "player.h"
#include "sys.h"
#include "inv.h"
#include "object.h"
#include "portgame.h"
#include "i18n/phrases.h"

#define INV_PACK_LAST 18                /* inv.h's slots: the worn ones and the backpack's */

/* o, or a key in it if it is a container (depth first), whose owner field is lock. */
static struct Object far *key_in(struct Object far *o, int lock)
{
    struct Object far *c, *found;
    if (OBJ_ITEM(o) > ITEM_LOCKPICK && OBJ_ITEM(o) < ITEM_LOCK && OBJ_OWNER(o) == lock)
        return o;
    if (OBJ_ISQUANT(o))
        return 0;
    for (c = Obj_PtrTMem(&o->ol.link); c != 0; c = Obj_PtrTMem(&c->qn.link))
        if ((found = key_in(c, lock)) != 0)
            return found;
    return 0;
}

/* 1 when the player carries the key of obj's lock and it unlocked it (checkLock's 3). */
int far port_auto_key(struct Object far *who, struct Object far *obj)
{
    struct Object far *lock, *key = 0;
    union Link far *link;
    char name[40];
    int id, i;
    if (!ENHANCED(ENH_AUTO_KEY) || who != ThePlayer || OBJ_ISQUANT(obj) || obj->ol.f.link == 0)
        return 0;
    link = &obj->ol.link;
    if ((lock = Obj_InList(&link, 0, MAJOR_SPEC, 0, 0xF)) == 0 || (id = lock->ol.f.link & 0x1FF) == 0)
        return 0;
    for (i = 0; i <= INV_PACK_LAST && key == 0; i++) {
        struct Object far *o = Obj_PtrTMem(&Inventory[i]);
        if (o != 0)
            key = key_in(o, id);
    }
    if (key == 0 || checkLock(ThePlayer, obj, id) != 3)
        return 0;
    if (port_zh()) {
        PortZh zh;
        const char *n = port_zh_name(key, 0, 0);
        port_zh_begin(&zh);
        port_zh_add(&zh, port_zh_lit("autokey.with"));
        port_zh_add(&zh, n ? n : "UNNAMED");
        port_zh_add(&zh, port_zh_lit("autokey.unlocked"));
        scroll_print(zh.text);
    } else {
        if (!get_name(name, key, 0, 0))
            strcpy(name, "UNNAMED");
        scroll_print("You unlock it with the ");
        scroll_print(name);
        scroll_print(" you carry.\n");
    }
    return 1;
}
