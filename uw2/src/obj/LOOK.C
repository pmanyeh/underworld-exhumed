/* target: ovr126 */
/* opts: -mm -1 -G -O -Y -d */
/* Looking at things: the description printed when the player looks at an object, its
   quality, enchantment, owner and charges, books, scrolls, gravestones, bones, keys and
   critters, and the short description used elsewhere: the whole of DOS overlay ovr126, in
   original order.

   What it does in the game: LookAt builds 'You see a <quality> <name> of <spell> with N full
   charges belonging to <race>.' from the object's tables: the quality word from string
   block 5 by the item's quality type and quality band, the name from block 4 (get_name), the
   enchantment from block 6, the owner's race from block 1. Then SpecialLook adds what some
   kinds of object say for themselves: a book or scroll's text, a key's description, whose
   bones these are. Critters are described by CritterLook (attitude, kind and name);
   gravestones and plaques by RectLook. GetObjDesc is the short form used by other messages
   (enchanting, mending, bartering). Callers: the look command (INTERACT.C), the inventory
   (INVPANEL.C), the rune panel (RUNES.C), Name Enchantment (SPELLS.C, lore 3).

   The lore argument says how much the player knows: 2 shows that an object is magical, 3
   (identified, from Name Enchantment or a lore check) names the enchantment and charges and
   shows curses and poisoned potions.
   Function and global names are the originals from the FM Towns symbol table.
   Name: descriptive (looking at things: LookAt, GetObjDesc). */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include "gfx.h"
#include "inv.h"
#include "map.h"
#include "object.h"
#include "player.h"
#include "sys.h"
#include "ui.h"

/* Copy a far string, terminator included, to any address. */
#define far_strcpy(d, s) FAR_COPY(d, s, str_len(s) + 1)
#ifndef __TURBOC__
#include <stdio.h>
#include "i18n/phrases.h"
#include "presentation/scroll_bridge.h"
/* Phase 14, as UW1's P11.2: what do_mods and do_of found, for LookAt's sentence in the
   locale's language */
static int zh_mod;          /* 1 magical, 2 cursed, 3 a blackrock gem's word (zh_gem) */
static int zh_gem;          /* that word's string id */
static int zh_spell;        /* the enchantment's string id, -2 none */
static int zh_charges;      /* the charges do_of printed, -2 none */
static int zh_poison;       /* do_of said ' of Poison' */
static void zh_critter(PortZh *zh, struct Object far *obj);
#endif



/* Prints the look description of obj (see the file comment). The quality word is skipped
   for quality 0 and for a light at quality 1; indestructible items (qualclass 3) use the
   fifth word of their quality type; quality type 13 takes no article. An ownable object
   with an owner 1..30 is 'belonging to' that race (strings 0x172 + owner: ' a worm', ' a
   slug', ... ' a human'). Non-lookable decals go to RectLook. */
void far LookAt(struct Object far *obj, int lore)
{
    char far *pos;
    char far *qstr;
    char numbuf[6];
    char first;
    char name[10];
    char plural;
    char far *owner;
    /* match: 80 bytes, but an identified, enchanted, owned item's description runs to 85
       with its terminator (the jewelled sword of Unsurpassed Accuracy belonging to a human):
       in DOS the end lands on owner, plural and name, all used by then; the host's frame is
       the compiler's, so the host's array is long enough (FRAME_LEN) */
    char text[FRAME_LEN(80, 96)];
    char qual[26];
    struct ComObj *com;
    int q;

    plural = 0;
    if (obj == 0)
        return;
    com = &ComObjData[OBJ_ITEM(obj)];
    if (com->lookable) {
        strcpy(text, "You see ");
        first = 0;
        name[0] = 0;
#ifndef __TURBOC__
        zh_mod = zh_poison = 0;
        zh_spell = zh_charges = -2;
#endif
        if (do_mods(obj, lore, name))
            first = name[0];
        qual[0] = 0;
        if (OBJ_MAJOR(obj) == MAJOR_CREATURE) {
            CritterLook(obj, text);
            str_cat(text, ".\n");
#ifndef __TURBOC__
            if (port_zh()) {
                PortZh zh;
                port_zh_begin(&zh);
                port_zh_add(&zh, port_zh_lit("look.you_see"));
                zh_critter(&zh, obj);
                port_zh_add(&zh, port_zh_lit("period"));
                scroll_print(zh.text);
                return;
            }
#endif
            scroll_print(text);
            return;
        }
        q = 0;
        if (obj->qn.f.quality != 0 && (obj->qn.f.quality > 1 || (obj->id & 0x1F8) != FIRST_LIGHT))
            q = com->qualclass != 3 ? (obj->qn.f.quality >> 4) + 1 : 5;
        qstr = get_string((com->qualtype * 6 + q) | STR_OBJLOOK);
        if (qstr != 0 && *qstr != 0) {
            far_strcpy(qual, qstr);
            first = *qstr;
        }
        if (OBJ_ISQUANT(obj) && !(obj->ol.f.link & LINK_SPECIAL) && obj->ol.f.link > 1) {
            plural = 1;
            first = 'x';
            strcat(text, itoa(obj->ol.f.link, numbuf, 10));
            strcat(text, " ");
        } else if (first != 0 && com->qualtype != 13) {
            switch (first) {
            case 'a': case 'e': case 'i': case 'o': case 'u':
                strcat(text, "an ");
                break;
            default:
                strcat(text, "a ");
            }
        }
        if (qual[0] != 0) {
            strcat(text, qual);
            strcat(text, " ");
        }
        if (name[0] != 0)
            strcat(text, name);
        pos = text + strlen(text);
        get_name(pos, obj, first == 0, plural);
        do_of(obj, lore, text);
        if (ComObjData[OBJ_ITEM(obj)].can_own && obj->ol.f.owner > 0 && (obj->ol.f.owner & 0x1F) <= 0x1E) {
            strcat(text, " belonging to");
            owner = get_string(((obj->ol.f.owner & 0x1F) + 0x172) | STR_GAME);
            str_cat(text, owner);
        }
        strcat(text, ".\n");
#ifndef __TURBOC__
        if (port_zh()) {
            /* '[N] [quality] [magical] name [of spell [with charges]] [belonging to]' in the
               locale's order (display only) */
            PortZh zh;
            const char *t;
            char num[160];
            port_zh_begin(&zh);
            port_zh_add(&zh, port_zh_lit("look.you_see"));
            if (plural) {
                snprintf(num, sizeof num, port_zh_lit("count"), obj->ol.f.link);
                port_zh_add(&zh, num);
            }
            if (qual[0] != 0) {
                t = port_zh_string((com->qualtype * 6 + q) | STR_OBJLOOK);
                port_zh_add(&zh, t ? t : qual);
                port_zh_add(&zh, t ? port_zh_lit("of") : " ");
            }
            if (zh_mod == 1) port_zh_add(&zh, port_zh_lit("look.magical"));
            if (zh_mod == 2) port_zh_add(&zh, port_zh_lit("look.cursed"));
            if (zh_mod == 3) port_zh_id(&zh, zh_gem);
            port_zh_add(&zh, port_zh_name(obj, 0, plural));
            if (zh_poison) port_zh_add(&zh, port_zh_lit("look.poison"));
            if (zh_spell >= 0) {
                t = port_zh_string(zh_spell);
                snprintf(num, sizeof num, port_zh_lit("look.spell"), t ? t : "UNNAMED");
                port_zh_add(&zh, num);
                if (zh_charges > 0) {
                    snprintf(num, sizeof num, port_zh_lit("look.charges"), zh_charges);
                    port_zh_add(&zh, num);
                } else if (zh_charges == 0)
                    port_zh_add(&zh, port_zh_lit("look.no_charges"));
            }
            if (ComObjData[OBJ_ITEM(obj)].can_own && obj->ol.f.owner > 0 && (obj->ol.f.owner & 0x1F) <= 0x1E) {
                port_zh_add(&zh, port_zh_lit("look.owner"));
                port_zh_id(&zh, ((obj->ol.f.owner & 0x1F) + 0x172) | STR_GAME);
            }
            port_zh_add(&zh, port_zh_lit("period"));
            scroll_print(zh.text);
        } else
#endif
        scroll_print(text);
    } else if (OBJ_CLASS(obj) == CLASS_DECAL)
        RectLook(obj, lore);
    SpecialLook(obj, lore);
}

/* The words before the name: 'cursed ' (lore 3, a curse, major 9), 'magical ' (lore 2,
   or a magic book of class 8 or below), or for a blackrock gem 'warm ' or 'cool ' by whether
   its owner field is set. Returns 1 when it added a word. */
char far do_mods(struct Object far *obj, int lore, char *s)
{
    int16 major;
    int16 effect;
    unsigned char flag;
    char far *str;

    if (decode_obj_spell(obj, &major, &effect, &flag)) {
        if (lore == 3) {
            if (!flag && major == 9) {
                strcat(s, "cursed ");
#ifndef __TURBOC__
                zh_mod = 2;
#endif
                return 1;
            }
        } else if (lore == 2 || (OBJ_CLASS(obj) == CLASS_BOOK && OBJ_INCLASS(obj) <= 8)) {
            strcat(s, "magical ");
#ifndef __TURBOC__
            zh_mod = 1;
#endif
            return 1;
        }
    }
    if ((obj->id & 0x1F8) == FIRST_BLACKROCK_GEM) {
#ifndef __TURBOC__
        zh_mod = 3;
        zh_gem = ((obj->ol.f.owner > 0) + 0x164) | STR_GAME;
#endif
        str = get_string(((obj->ol.f.owner > 0) + 0x164) | STR_GAME);
        far_strcpy(s + strlen(s), str);
        return 1;
    }
    return 0;
}

/* The words after the name, for an identified object (lore 3): ' of Poison' for a
   potion holding a poison trap object, else ' of <enchantment>' (block 6: weapon and armour
   enchantments from 0x1C0, spells from 0x100, others by major class * 16) and, for an object
   whose spell has charges, ' with N full charges' (a potion always shows 1). */
char far do_of(struct Object far *obj, int lore, char *s)
{
    int16 major;
    int16 effect;
    unsigned char flag;
    char far *name;
    char found;
    union Link far *link;
    struct Object far *spell;
    int charges;

    always_decode = 1;
    if (lore == 3 && OBJ_ITEM(obj) >= FIRST_POTION && OBJ_ITEM(obj) <= ITEM_BROWN_POTION && !OBJ_ISQUANT(obj)) {
        link = &obj->ol.link;
        if (Obj_InList(&link, 0, 6, 0, 0) != 0) {
            strcat(s, " of Poison");
#ifndef __TURBOC__
            zh_poison = 1;
#endif
            return 1;
        }
    }
    found = decode_obj_spell(obj, &major, &effect, &flag);
    always_decode = 0;
    if (found != 0 && lore == 3) {
        if (major == 12) {
            major = 0x1C0;
            if (OBJ_MINOR(obj) > 1)
                effect += 0x10;
            effect += major;
        } else if (major == 9)
            return 0;
        else if (flag != 0 && major <= 0)
            effect += 0x100;
        else
            effect += major << 4;
        name = get_string(effect | STR_SPELLS);
#ifndef __TURBOC__
        zh_spell = effect | STR_SPELLS;
#endif
        if (name == 0 || *name == 0)
            name = "UNNAMED";
        strcat(s, " of ");
        far_strcpy(s + strlen(s), name);
        if (!OBJ_ISQUANT(obj)) {
            charges = -1;
            link = &obj->ol.link;
            spell = Obj_InList(&link, 0, MAJOR_SPEC, 2, 0);
            if (spell != 0 && (spell->id & 0x800))
                charges = spell->qn.f.quality;
            if (charges >= 0) {
                strcat(s, " with ");
#ifndef __TURBOC__
                zh_charges = charges > 0 && OBJ_ITEM(obj) >= FIRST_POTION && OBJ_ITEM(obj) <= ITEM_BROWN_POTION ? 1 : charges;
#endif
                if (charges > 0) {
                    char num[3] = "00";

                    if (OBJ_ITEM(obj) >= FIRST_POTION && OBJ_ITEM(obj) <= ITEM_BROWN_POTION)
                        charges = 1;
                    num[1] = charges % 10 + '0';
                    if (charges > 9) {
                        num[0] = charges / 10 + '0';
                        strcat(s, num);
                    } else
                        strcat(s, &num[1]);
                } else
                    strcat(s, "no");
                strcat(s, " full charge");
                if (charges != 1)
                    strcat(s, "s");
            }
        }
        return 1;
    }
    return 0;
}

/* Reads a book or scroll: a map says 'Enscribed upon the scroll is your map.'; an
   enchanted (spell) scroll says nothing here; a scroll flagged with ID_FLAG10 plays a
   cutscene (0x100 + its text number); otherwise 'You read the <name>...' and its text from
   block 3. Reading text 6 sets bit 2 of quest variable 26. */
void far BookLook(struct Object far *obj, int print)
{
    char far *str;
    char text[100];

    if (print < 1)
        return;
    if (OBJ_ITEM(obj) == ITEM_MAP) {
        game_sprint(0xA5);  /* 'Enscribed upon the scroll is your map.' */
        return;
    }
    if (OBJ_ITEM(obj) == ITEM_BIT_OF_A_MAP)
        return;
    if ((obj->id & ID_ENCHANT) && OBJ_MAJOR(obj) != MAJOR_RECT)
        return;
    if (obj->id & ID_FLAG10) {
        show_cutscene((obj->ol.f.link & 0x1FF) + 0x100);
        return;
    }
    if ((obj->ol.f.link & 0x1FF) > 0xFF)
#ifndef __TURBOC__
        port_book_print(get_string((obj->ol.f.link & 0x1FF) | STR_BOOKS));   /* names glossed */
#else
        scroll_print(get_string((obj->ol.f.link & 0x1FF) | STR_BOOKS));
#endif
    else {
        strcpy(text, "You read the ");
        get_name(text + strlen(text), obj, 0, 0);
        strcat(text, "...\n");
#ifndef __TURBOC__
        if (port_zh()) {
            PortZh zh;
            const char *n = port_zh_name(obj, 0, 0);
            port_zh_begin(&zh);
            port_zh_add(&zh, port_zh_lit("look.read"));
            port_zh_add(&zh, n ? n : "");
            port_zh_add(&zh, port_zh_lit("look.read_end"));
            scroll_print(zh.text);
        } else
#endif
        scroll_print(text);
        if ((obj->ol.f.link & 0x1FF) == 6)
            player->quests[26] = (player->quests[26] & 0xFFFFFFFBL) + 4;
        str = get_string((obj->ol.f.link & 0x1FF) | STR_BOOKS);
#ifndef __TURBOC__
        port_book_print(str);
#else
        scroll_print(str);
#endif
        scroll_print("\n");
    }
}

/* Looks at a decal: classes 14 and 15 describe the texture behind them (and a shaft
   texture, terrain 5, prompts player_look_shaft), class 4 is a bridge, class 5 a gravestone
   and class 6 a plaque. A gravestone's picture number comes from DATA\GRAVE.DAT, indexed by
   its text number; with one, the scroll is cleared and the gravestone shown
   (player_look_grave), otherwise 'The gravestone reads: ' (or for a plaque 'The plaque
   reads: ') and the inscription from block 8. */
void far RectLook(struct Object far *obj, int look)
{
    int fd;
    unsigned char c;
    unsigned char ok;
    char far *grave;
    char far *str;
    struct Object far *o;
    register int base;
    register int index;

    base = 0x160;
    c = 0;
    switch OBJ_INCLASS(obj) {
    case 14:
    case 15:
        if (look >= 0)
            look_nothing(2, obj->ol.f.owner + 1);
        if (look > 0 && (TxmTerr[obj->ol.f.owner] & 7) == 5)
            player_look_shaft();
        else if (look == -1)
            break;
        break;
    case 4:
        if (OBJ_FLAGS(obj) >= 2)
            look_nothing(2, OBJ_FLAGS(obj) + 0x3F);
        else
            game_sprint(0xBA);  /* 'You see a bridge.' */
        break;
    case 6:
        base += 0x10;
    case 5:
        index = OBJ_ISQUANT(obj) ? obj->ol.f.link & 0x1FF : obj->ol.f.owner;
        if (OBJ_INCLASS(obj) == 5) {
            ok = (fd = open("DATA\\grave.dat", O_RDONLY | O_BINARY)) != -1;
            ok &= lseek(fd, index, 0) != -1L;
            ok &= read(fd, &c, 1) == 1;
            ok &= close(fd) != -1;
            if (!ok)
                break;
        }
        if ((grave = get_string(index | STR_WRITING)) != 0 && c != 0)
            scroll_clear(1);
        if (OBJ_INCLASS(obj) == 6 || c == 0) {
            base = base + OBJ_FLAGS(obj);
            str = get_string(base | STR_WRITING);
            scroll_print(str);
        }
        if (grave != 0) {
            o = obj;
            if (index != 0 || find_obj(6, 2, 3, &o) == 0) {
                grave = fix_name_string(grave, 1, 0);
                scroll_print(grave);
                scroll_print("\n");
            }
        }
        if (c != 0)
            player_look_grave(c);
        break;
    }
}

/* Whose bones: 'It looks to be that of ' (or 'They look to be those of ' for a pile) and
   the creature kind from the owner field (+ FIRST_CREATURE), 'Relk.' for owner 0x3D or 'an
   adventurer.' for 0x3F. Owners 0, 0x28 and 0x3C..0x3E say nothing, so the 'Relk.' case for
   0x3D is never reached. */
void far BonesLook(struct Object far *obj, int print)
{
    struct Object tmp;
    char text[40];
    int id;

    if (print != 0 && obj->ol.f.owner != 0 && obj->ol.f.owner != 0x28
        && (obj->ol.f.owner < 0x3C || obj->ol.f.owner == 0x3F)) {
        id = 0x1A;
        if (OBJ_ITEM(obj) == ITEM_PILE_OF_BONES_C6 || obj->ol.f.link > 1)
            id++;
        game_sprint(id);
        if (obj->ol.f.owner == 0x3D)
            scroll_print("Relk.\n");
        if (obj->ol.f.owner == 0x3F)
#ifndef __TURBOC__
            scroll_print(port_zh() ? (char *)port_zh_lit("look.bones_adventurer") : "an adventurer.\n");
#else
            scroll_print("an adventurer.\n");
#endif
        else {
            SET_ITEM(&tmp, obj->ol.f.owner + FIRST_CREATURE);
            tmp.whoami = 0;
            get_name(text, &tmp, 1, 0);
#ifndef __TURBOC__
            if (port_zh()) {
                PortZh zh;
                port_zh_begin(&zh);
                port_zh_add(&zh, port_zh_name(&tmp, 1, 0));
                port_zh_add(&zh, port_zh_lit("look.bones_end"));
                scroll_print(zh.text);
            } else
#endif
            {
                scroll_print(text);
                scroll_print(".\n");
            }
        }
    }
}

/* A key's description, string 0x64 + owner of block 5 (by lock, e.g. 'The key feels
   unnaturally heavy.'), if there is one. */
void far KeyLook(struct Object far *obj, int print)
{
    char far *str;

    if (print != 0) {
        str = get_string((obj->ol.f.owner + 0x64) | STR_OBJLOOK);
        if (str != 0)
            scroll_print(str);
    }
}

/* Describes a critter into s: 'a <attitude> <kind> named <Name>'. The attitude word is
   block 5 0x60 + attitude (hostile, upset, peaceful, friendly), shown only for whoami below
   0xF0 or 0xFF; the kind is its item name, or for a nameless kind with goal 11 a string
   from 0x115 by its animation sequence; the name is the NPC's conversation name (block 7,
   whoami + 0x10). A name that starts in lower case replaces the kind instead of following
   it. */
void far CritterLook(struct Object far *obj, char *s)
{
    char far *desc;
    char far *name;
    char far *attitude;
    unsigned char who;

    desc = get_string(OBJ_ITEM(obj) | STR_OBJNAMES);
    if (*desc == 0) {
        if (OBJ_GOAL(obj) == 11)
            desc = get_string((OBJ_SEQ(obj) + 0x115) | STR_GAME);
        else
            desc = 0;
    }
    who = obj->whoami;
    if (who < 0xF0 || who == 0xFF) {
        attitude = get_string((OBJ_ATTITUDE(obj) + 0x60) | STR_OBJLOOK);
        if (*attitude == 0)
            attitude = 0;
    } else
        attitude = 0;
    if (who > 0) {
        name = get_string((who + 0x10) | STR_CONV);
        if (*name == 0)
            name = 0;
    } else
        name = 0;
    if (desc != 0 && attitude != 0) {
        switch (*attitude) {
        case 'a': case 'e': case 'i': case 'o': case 'u':
            strcat(s, "an ");
            break;
        default:
            strcat(s, "a ");
        }
        str_cat(s, attitude);
        strcat(s, " ");
    }
    if (desc != 0 && (name == 0 || isupper(*name)))
        str_cat(s, fix_name_string(desc, attitude == 0, 0));
    if (name != 0) {
        if (desc != 0 && isupper(*name))
            strcat(s, " named ");
        str_cat(s, fix_name_string(name, desc == 0, 0));
    }
}

#ifndef __TURBOC__
/* CritterLook's '<attitude> <kind> named <Name>' in the locale's order, its choices made as
   CritterLook makes them (display only) */
static void zh_critter(PortZh *zh, struct Object far *obj)
{
    char far *desc, far *name, far *attitude;
    const char *t;
    int desc_id = OBJ_ITEM(obj) | STR_OBJNAMES;
    unsigned char who = obj->whoami;
    desc = get_string(desc_id);
    if (*desc == 0) {
        if (OBJ_GOAL(obj) == 11) desc = get_string(desc_id = (OBJ_SEQ(obj) + 0x115) | STR_GAME);
        else desc = 0;
    }
    attitude = who < 0xF0 || who == 0xFF ? get_string((OBJ_ATTITUDE(obj) + 0x60) | STR_OBJLOOK) : 0;
    if (attitude && *attitude == 0) attitude = 0;
    name = who > 0 ? get_string((who + 0x10) | STR_CONV) : 0;
    if (name && *name == 0) name = 0;
    if (desc != 0 && attitude != 0) {
        t = port_zh_string((OBJ_ATTITUDE(obj) + 0x60) | STR_OBJLOOK);
        port_zh_add(zh, t ? t : attitude);
        port_zh_add(zh, t ? port_zh_lit("of") : " ");
    }
    if (desc != 0 && (name == 0 || isupper(*name))) {
        t = port_zh_string(desc_id);
        port_zh_add(zh, t ? t : fix_name_string(desc, attitude == 0, 0));
    }
    if (name != 0) {
        t = port_zh_string((who + 0x10) | STR_CONV);
        if (desc != 0 && isupper(*name)) port_zh_add(zh, port_zh_lit("look.named"));
        port_zh_add(zh, t ? t : fix_name_string(name, desc == 0, 0));
    }
}
#endif

/* The second line of a look for books and scrolls (BookLook), keys (KeyLook), bones
   (BonesLook) and doors: a closed door whose owner bit 0 is set prints string 0x91, which is
   empty in the game's STRINGS.PAK. */
void far SpecialLook(struct Object far *obj, int print)
{
    int minor;

    minor = OBJ_MINOR(obj);
    switch (OBJ_MAJOR(obj)) {
    case MAJOR_SPEC:
        if (minor == 3 && OBJ_INCLASS(obj) < 10)
            BookLook(obj, print);
        else if (minor == 0)
            KeyLook(obj, print);
        break;
    case MAJOR_STUFF:
        if (minor == 0 && OBJ_ITEM(obj) >= ITEM_SKULL_C2 && OBJ_ITEM(obj) <= ITEM_PILE_OF_BONES_C6)
            BonesLook(obj, print);
        break;
    case MAJOR_RECT:
        if (minor == 0 && OBJ_INCLASS(obj) < 8 && (obj->ol.f.owner & 1))
            game_sprint(0x91);  /* an empty string */
        break;
    }
}

/* The short description of obj into s: quality word and name (plural for a stack), or a
   critter's CritterLook. Returns the quantity. */
int far GetObjDesc(struct Object far *obj, int lore, char *s)
{
    char far *qstr;
    char *end;
    int qty;
    int q;
    char qual[26];
    struct ComObj *com;

    qty = 1;
    com = &ComObjData[OBJ_ITEM(obj)];
    q = 0;
    *s = 0;
    if (OBJ_MAJOR(obj) == MAJOR_CREATURE)
        CritterLook(obj, s);
    else {
        if (obj->qn.f.quality != 0)
            q = com->qualclass != 3 ? (obj->qn.f.quality >> 4) + 1 : 5;
        qstr = get_string((com->qualtype * 6 + q) | STR_OBJLOOK);
        if (qstr != 0 && *qstr != 0)
            str_copy(qual, qstr);
        else
            qual[0] = 0;
        if (OBJ_ISQUANT(obj) && !(obj->ol.f.link & LINK_SPECIAL))
            qty = obj->ol.f.link;
        if (qual[0] != 0) {
            str_cat(s, qual);
            str_cat(s, " ");
        }
        end = s + strlen(s);
        get_name(end, obj, 0, qty > 1);
    }
#ifndef __TURBOC__
    if (port_zh()) {
        /* Phase 14: the description's translation beside it (port_i18n_note_name), for the
           callers that print it by itself (SPELLS2.C's identify and enchant messages); eight
           kept, as the notes are */
        static char shown[8][160];
        static int next;
        char *z = shown[next++ & 7];
        PortZh zh;
        const char *t;
        port_zh_begin(&zh);
        if (OBJ_MAJOR(obj) == MAJOR_CREATURE)
            zh_critter(&zh, obj);
        else {
            if (qual[0] != 0) {
                t = port_zh_string((com->qualtype * 6 + q) | STR_OBJLOOK);
                port_zh_add(&zh, t ? t : qual);
                port_zh_add(&zh, t ? port_zh_lit("of") : " ");
            }
            port_zh_add(&zh, port_zh_name(obj, 0, qty > 1));
        }
        snprintf(z, sizeof shown[0], "%s", zh.text);
        port_i18n_note_name(s, z);
    }
#endif
    return qty;
}
