/* P11.2: the locale's words for the English the game's code writes itself ("You see ",
   " belonging to"), for sentences composed for display (port_zh in GAMESTRN.C), and for the
   short pieces it prints by themselves (port_zh_literal, matched whole by the HD scroll). The
   game's sources stay ASCII and name a phrase by key; a key without a phrase reads as itself. */
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
    {"look.read", "你讀了"},
    {"look.read_end", "……\n"},
    {"look.bones_adventurer", "一名冒險者的。\n"},
    {"look.bones_end", "的。\n"},
    {"look.named", "，名叫"},
    {"look.texture_end", "。\n"},
    {"stats.out_of", "／"},
    {"food.that", ""},
    {"use.use", "要把"},
    {"use.on_what", "用在什麼上？\n"},
    {"inv.your", "你的"},
    {"inv.destroyed", "壞掉了。\n"},
    {"inv.damaged", "受損了。\n"},
    {"inv.empty", "是空的。\n"},
    {"scroll.more", "[繼續]"},                     /* literals.tsv's [MORE] */
    {"stats.level", "第%d級"},                     /* "1ST" .. "16TH" */
    {"end.level", "第%d級%s"},                     /* game_stats: level, class */
    {"qty.prompt", "要移動幾個？"},                /* "Move how many?" as a panel (qty_dialog.c) */
    {"qty.minus", "－"},
    {"qty.plus", "＋"},
    {"qty.half", "一半"},
    {"qty.all", "全部"},
    {"qty.ok", "確定"},
    {"qty.cancel", "取消"},
    {"opt.save", "儲存遊戲"},                      /* the options panel (OPTBTNS.GR) */
    {"opt.restore", "讀取遊戲"},
    {"opt.music", "音樂"},
    {"opt.sound", "音效"},
    {"opt.detail", "畫面細節"},
    {"opt.return", "返回遊戲"},
    {"opt.quit", "離開遊戲"},
    {"opt.on", "開"},
    {"opt.off", "關"},
    {"opt.cancel", "取消"},
    {"opt.done", "完成"},
    {"opt.low", "低"},
    {"opt.medium", "中"},
    {"opt.high", "高"},
    {"opt.very_high", "極高"},
    {"opt.save_title", "儲存至："},
    {"opt.restore_title", "讀取自："},
    {"opt.quit_title", "離開遊戲"},
    {"opt.sure", "確定嗎？"},
    {"opt.music_on", "音樂：開"},
    {"opt.music_off", "音樂：關"},
    {"opt.sound_on", "音效：開"},
    {"opt.sound_off", "音效：關"},
    {"opt.turn_music", "音樂設為："},
    {"opt.turn_sound", "音效設為："},
    {"opt.set_level", "設為："},
    {"opt.level_low", "細節：低"},
    {"opt.level_medium", "細節：中"},
    {"opt.level_high", "細節：高"},
    {"opt.level_very_high", "細節：極高"},
    {"opt.slot1", "存檔一"},                       /* the slots' numerals I .. IV */
    {"opt.slot2", "存檔二"},
    {"opt.slot3", "存檔三"},
    {"opt.slot4", "存檔四"},
    {"opt.yes", "是"},
    {"opt.no", "否"},
    {"menu.intro", "開場介紹"},                    /* the main menu's buttons */
    {"menu.create", "創造角色"},
    {"menu.credits", "製作群"},
    {"menu.journey", "繼續旅程"},
    {"stats.str", "力量"},
    {"stats.dex", "敏捷"},
    {"stats.int", "智慧"},
    {"stats.vit", "生命力"},
    {"stats.mana", "法力"},
    {"stats.exp", "經驗值"},
};

/* whole pieces the game's code prints by themselves; a name between "The " and the rest is
   shown translated by the HD scroll (port_i18n_named), so "The " reads as nothing */
static const struct { const char *english, *zh; } literals[] = {
    {"The ", ""},
    {" is locked.\n", "鎖住了。\n"},
    {" is too full.\n", "已經滿了。\n"},
    {" on the ", "（在"},
    {" was successfully dearmed.\n", "上）成功解除了。\n"},
    {"Your bumbling attempts have set off the ", "你笨手笨腳地觸發了"},
    {"Unable to defuse trap.\n", "無法解除陷阱。\n"},
    {"The book explodes in your face!\n", "書在你面前爆炸了！\n"},
    {"Sorry, you have no ", "抱歉，你沒有"},
    {"Not a spell\n", "這不是法術\n"},
    {"Yes", "是"},
    {"No", "否"},
    {"Str:", "力量"},                            /* CHARGEN.C's show_atts (P12) */
    {"Dex:", "敏捷"},
    {"Int:", "智慧"},
    {"Vit:", "生命力"},
    {"\\6    Save Game Descriptions", "\\6    存檔說明"},
    {"<not used yet>", "<尚未使用>"},
    {"I- ", "存檔一："},                           /* GAMEWRAP.C's ShowSaveRest */
    {"II- ", "存檔二："},
    {"III- ", "存檔三："},
    {"IV- ", "存檔四："},
    {"Chant the mantra: ", "誦唸祈禱語："},
    {"Mouse look enabled.\n","滑鼠視角已開啟。\n"},
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
