/* Phase 14: UW2's words for the English its code writes itself, the same API as UW1's
   phrases.c (i18n/phrases.h): by key, for sentences composed for display (port_zh in
   GAMESTRN.C and its callers), and by the English, for the short pieces the code prints by
   themselves (port_zh_literal, matched whole by the HD scroll's bridge). The game's sources
   stay ASCII and name a phrase by key; a key without a phrase reads as itself. */
#include "i18n/phrases.h"
#include <string.h>

static const struct { const char *key, *zh; } phrases[] = {
    {"of", "的"},                                  /* after an adjective: 磨損的 */
    {"period", "。\n"},
    {"count", " %d 個"},
    {"look.you_see", "你看到"},
    {"look.magical", "有魔力的"},
    {"look.cursed", "受詛咒的"},
    {"look.spell", "，附有「%s」"},
    {"look.charges", "，還有 %d 次充能"},
    {"look.no_charges", "，已經沒有充能"},
    {"look.owner", "，屬於"},
    {"look.poison", "（有毒）"},                    /* do_of's " of Poison" */
    {"look.read", "你讀了"},
    {"look.read_end", "……\n"},
    {"look.bones_adventurer", "一名冒險者的。\n"},
    {"look.bones_end", "的。\n"},
    {"look.named", "，名叫"},
    {"food.that", ""},                              /* "That <food>" before its taste's string */
    {"stats.out_of", "／"},                          /* GAMESCR.C: vitality and mana */
    {"stats.level", "第%d級"},                     /* STATS.C: "1ST" .. "16TH" */
    {"stats.str", "力量"},                         /* the statistics page's art */
    {"stats.dex", "敏捷"},
    {"stats.int", "智慧"},
    {"stats.vit", "生命力"},
    {"stats.mana", "法力"},
    {"stats.exp", "經驗值"},
    {"inv.your", "你的"},                          /* INVDATA.C: worn out */
    {"inv.destroyed", "壞掉了。\n"},
    {"inv.damaged", "受損了。\n"},
    {"trap.on", "上的"},                             /* SKILLS.C: "<object>上的<trap>" */
    {"vend.price", "（%d 金幣）"},                    /* WORLDEV.C: a vending machine's price */
    {"cheat.applied", "密技設定已生效。\n"},        /* ui/cheat_menu.c, as UW1's */
    {"enhance.applied", "設定已生效。\n"},
    {"conv.unknown", "我不知道這是什麼……\n"},   /* --enhance unknown-topic (conv/CONVERSE.C) */
    {"autokey.with", "你用背包裡的"},             /* --enhance auto-key (port/ui/autokey.c) */
    {"autokey.unlocked", "打開了鎖。\n"},
    {"take.pack_full", "背包沒有空位了。\n"},     /* --enhance e-takes (port/ui/keys.c) */
    {"map.note_prompt", "地圖註記（Enter 完成，Esc 取消）"},   /* port/ui/map_notes_bridge.c, as UW1's */
    {"use.use", "要把"},                             /* OBJUSE.C's UseThing, as UW1's */
    {"use.on_what", "用在什麼上？\n"},
    {"study.is", "是"},                             /* SPELLS2.C: Study Monster's "This creature is " */
    {"list.sep", "、"},                             /* between the items of a list */
    {"list.and", "和"},                             /* before a list's last item */
    {"path.and", "，"},                             /* MISCUTIL.C's print_path_to: direction and depth */
    {"path.near", "就在附近"},
    {"scroll.more", "[繼續]"},
    {"qty.prompt", "要移動幾個？"},                /* "Move how many?" as a panel (qty_dialog.c), as UW1's */
    {"qty.minus", "－"},
    {"qty.plus", "＋"},
    {"qty.half", "一半"},
    {"qty.all", "全部"},
    {"qty.ok", "確定"},
    {"qty.cancel", "取消"},
    {"opt.save", "儲存遊戲"},                      /* the options panel (WRAPPER.C), as UW1's */
    {"opt.restore", "讀取遊戲"},
    {"opt.music", "音樂"},
    {"opt.sound", "音效"},
    {"opt.detail", "畫面細節"},
    {"opt.quit", "離開遊戲"},
    {"opt.return", "返回遊戲"},
    {"opt.set_level", "設為："},
    {"opt.low", "低"},
    {"opt.medium", "中"},
    {"opt.high", "高"},
    {"opt.very_high", "極高"},
    {"opt.done", "完成"},
    {"opt.level_low", "細節：低"},
    {"opt.level_medium", "細節：中"},
    {"opt.level_high", "細節：高"},
    {"opt.level_very_high", "細節：極高"},
    {"opt.quit_title", "離開遊戲"},
    {"opt.sure", "確定嗎？"},
    {"opt.yes", "是"},
    {"opt.no", "否"},
    {"opt.save_title", "儲存至："},
    {"opt.restore_title", "讀取自："},
    {"opt.slot1", "存檔一"},
    {"opt.slot2", "存檔二"},
    {"opt.slot3", "存檔三"},
    {"opt.slot4", "存檔四"},
    {"opt.cancel", "取消"},
    {"opt.music_on", "音樂：開"},
    {"opt.music_off", "音樂：關"},
    {"opt.sound_on", "音效：開"},
    {"opt.sound_off", "音效：關"},
    {"opt.turn_music", "音樂設為："},
    {"opt.turn_sound", "音效設為："},
    {"opt.on", "開"},
    {"opt.off", "關"},
    {"menu.intro", "開場介紹"},                    /* the main menu's buttons, as UW1's */
    {"menu.create", "創造角色"},
    {"menu.credits", "製作群"},
    {"menu.journey", "繼續旅程"},
};

/* whole pieces the game's code prints by themselves; a name between "The " and the rest is
   shown translated by the HD scroll (port_i18n_named), so "The " reads as nothing */
static const struct { const char *english, *zh; } literals[] = {
    {"The ", ""},
    {"Not a spell\n", "這不是法術\n"},             /* RUNES.C */
    {"trap", "陷阱"},                              /* SKILLS.C's names for a trap that has none */
    {"poison trap", "毒陷阱"},
    {"You see ", "你看到"},
    {"a ", ""},                                    /* SPELLS2.C: before an object's description */
    {"Str:", "力量"},                              /* CHARGEN.C's attributes, as UW1's */
    {"Dex:", "敏捷"},
    {"Int:", "智慧"},
    {"Vit:", "生命力"},
    {"Yes", "是"},                                 /* SCROLLIO.C's wyorn */
    {"No", "否"},
    {"<not used yet>", "<尚未使用>"},               /* GAMEWRAP.C's save list */
    {"I- ", "存檔一："},
    {"II- ", "存檔二："},
    {"III- ", "存檔三："},
    {"IV- ", "存檔四："},
    {"Mouse look enabled.\n", "滑鼠視角已開啟。\n"},   /* MOUSE.C, --enhance mouse-look */
    {"Mouse look disabled.\n", "滑鼠視角已關閉。\n"},
};

const char *port_zh_lit(const char *key)
{
    size_t i;
    for (i = 0; i < sizeof phrases / sizeof phrases[0]; i++)
        if (!strcmp(phrases[i].key, key)) return phrases[i].zh;
    return key;
}

const char *port_zh_literal(const char *english)
{
    size_t i;
    for (i = 0; i < sizeof literals / sizeof literals[0]; i++)
        if (!strcmp(literals[i].english, english)) return literals[i].zh;
    return 0;
}
