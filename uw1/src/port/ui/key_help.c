/* The settings screen's Keys tab (Exhume's ui/settings.c, SET_TAB_HELP; Shift+F1 opens it): the
   keys and the mouse as this run has them. The key bindings are made once, in init_player
   (game/PLAYER.C), from the enhancements the run started with, so the lines follow ENHANCED:
   modern-keys' keys in place of the original movement keys, e-takes' E, mouse-look, the wheel,
   easy-erase's put-down and the rune keys only when on. In Chinese when the screen is (settings_text.c). Not compat.h: its
   pack(1) would lay struct settings_line out other than settings.c does. */
#include "portgame.h"
#include "sys/enhance.h"
#include "key_help.h"
#include "presentation/settings_text.h"
#include "ui/settings.h"

/* when a line is shown */
enum { ALWAYS, CLASSIC, MODERN, E_USE, E_TAKE, LOOK, INVERT, WHEEL, ERASE, RUNES, LOCK, RECORD };

struct entry {
    int when, heading;
    const char *key, *text;             /* English */
    const char *key_zh, *text_zh;       /* Chinese (NULL key: the English one; NULL text: no line) */
};

#ifdef __APPLE__
#define SETTINGS_KEY "Cmd+,"
#define SETTINGS_KEY_ZH "Cmd＋,"
#else
#define SETTINGS_KEY "F11"
#define SETTINGS_KEY_ZH NULL
#endif

static const struct entry entries[] = {
    { ALWAYS, 1, NULL, "Moving", NULL, "移動" },
    { CLASSIC, 0, "W", "Run forward", NULL, "快跑前進" },
    { CLASSIC, 0, "S", "Walk forward", NULL, "慢走前進" },
    { CLASSIC, 0, "X", "Walk back", NULL, "後退" },
    { CLASSIC, 0, "A / D", "Turn left / right", "A／D", "左轉／右轉" },
    { CLASSIC, 0, "Z / C", "Slide left / right", "Z／C", "向左／向右平移" },
    { CLASSIC, 0, "Q / E", "Down / up when flying or levitating", "Q／E", "飛行或飄浮時下降／上升" },
    { CLASSIC, 0, "Shift+W/A/S/D/X", "One step", "Shift＋W／A／S／D／X", "單步移動" },
    { CLASSIC, 0, "J", "Jump", NULL, "跳躍" },
    { CLASSIC, 0, "Shift+J", "Standing long jump", "Shift＋J", "原地跳遠" },
    { CLASSIC, 0, "1 / 2 / 3", "Look down / ahead / up", "1／2／3", "視角向下／回正／向上" },
    { MODERN, 0, "W / S", "Forward / back", "W／S", "前進／後退" },
    { MODERN, 0, "A / D", "Slide left / right", "A／D", "向左／向右平移" },
    { MODERN, 0, "Left / Right", "Turn left / right", "方向鍵左／右", "左轉／右轉" },
    { MODERN, 0, "Up / Down", "Look up / down", "方向鍵上／下", "視角向上／向下" },
    { MODERN, 0, "1 / 2 / 3", "Look up / ahead / down", "1／2／3", "視角向上／回正／向下" },
    { MODERN, 0, "Keypad 5", "Look ahead", "數字鍵盤 5", "視角回正" },
    { MODERN, 0, "Ctrl+arrows", "One step", "Ctrl＋方向鍵", "單步移動" },
    { MODERN, 0, "Space, left Shift", "Jump", "空白鍵、左 Shift", "跳躍" },
    { MODERN, 0, "Ctrl+Shift", "Standing long jump", "Ctrl＋Shift", "原地跳遠" },
    { MODERN, 0, "Shift / Ctrl", "Up / down when flying or levitating", "左 Shift／左 Ctrl", "飛行或飄浮時上升／下降" },

    { ALWAYS, 1, NULL, "Actions", NULL, "互動" },
    { ALWAYS, 0, "F1", "Options: save, restore, sound, quit", NULL, "選項畫面（存讀檔、音效、離開）" },
    { ALWAYS, 0, "F2 / F3 / F4", "Talk / pick up / look", "F2／F3／F4", "交談／撿起／查看" },
    { ALWAYS, 0, "F5 / F6", "Fight / use", "F5／F6", "戰鬥／使用" },
    { ALWAYS, 0, "F7", "Inventory or statistics panel", NULL, "切換背包／屬性面板" },
    { ALWAYS, 0, "F8", "Cast the runes on the shelf", NULL, "施放選好的符文法術" },
    { ALWAYS, 0, "F9", "Track", NULL, "追蹤技能" },
    { ALWAYS, 0, "F10", "Sleep (on a bedroll if you have one)", NULL, "睡覺（有鋪蓋就用鋪蓋）" },
    { ALWAYS, 0, "1 - 4", "In a conversation: the answer", "1～4", "對話中選擇回答" },
    { E_USE, 0, "E", "Use what is under the pointer, or talk", NULL, "使用游標下的東西，或交談" },
    { E_TAKE, 0, "E", "Take it into the pack, else use or talk", NULL, "能撿的收進背包，否則使用或交談" },
    { E_TAKE, 0, "", "(onto a matching stack in the pack)", NULL, "（背包有同類的一疊就疊上去）" },
    { MODERN, 0, "Q", "Look at what is under the pointer", NULL, "查看游標下的東西" },
    { MODERN, 0, "C", "Draw or sheathe your weapon", NULL, "拔出／收起武器（戰鬥模式）" },
    { MODERN, 0, "X", "Close the open container", NULL, "關閉開著的容器" },
    { MODERN, 0, "V / B", "Scroll the inventory down / up", "V／B", "背包往下／往上捲" },
    { MODERN, 0, "Z", "Map", NULL, "地圖" },
    { MODERN, 0, "R / F", "Rune bag / statistics (again: back)", "R／F", "符文袋／屬性面板（再按回背包）" },
    { MODERN, 0, "G", "Compass", NULL, "指南針" },
    { MODERN, 0, "H", "Health and mana flasks", NULL, "查看血瓶與法力瓶" },

    { ALWAYS, 1, NULL, "Fighting", NULL, "戰鬥" },
    { ALWAYS, 0, ". / ; / P", "Stab / slash / bash", ". ／ ; ／ P", "刺擊／揮砍／敲擊" },
    { ALWAYS, 0, "", "Hold to charge the blow", NULL, "按住不放可以蓄力" },

    { ALWAYS, 1, NULL, "Mouse", NULL, "滑鼠" },
    { ALWAYS, 0, "Left button", "Held in the view: move toward the pointer", "左鍵", "在畫面上按住：往游標方向移動" },
    { ALWAYS, 0, "", "Bottom of the view: slide left, back, right", NULL, "畫面最下方：左移、後退、右移" },
    { ALWAYS, 0, "Both buttons", "Jump", "左右鍵一起按", "跳躍" },
    { ALWAYS, 0, "Right button", "Look (drag: pick up), or the F2 - F6 action", "右鍵", "查看（拖曳是撿起），或 F2～F6 選的動作" },
    { CLASSIC, 0, "Tab, arrows", "Move the pointer", "Tab、方向鍵", "用鍵盤移動游標" },
    { LOOK, 0, "` or middle button", "Mouse-look: the mouse turns the view", "`（反引號）、中鍵", "滑鼠視角：滑鼠轉視角，點擊作用在準星" },
    { LOOK, 0, "", "and a click acts at the crosshair", NULL, NULL },
    { INVERT, 0, "", "(up and down inverted)", NULL, "（上下反轉）" },
    { WHEEL, 0, "Wheel", "Skills, open bags, past [MORE]", "滾輪", "捲動技能、開著的袋子、跳過〔更多〕" },
    { ERASE, 0, "Right button, Esc", "Put the map's eraser down", "右鍵、Esc", "放下地圖的橡皮擦" },

    { RUNES, 1, NULL, "Rune keys", NULL, "符文快捷鍵" },
    { RUNES, 0, "Ctrl+Alt+letter", "Put a rune you have on the shelf", "Ctrl＋Alt＋字母", "選一顆你有的符文" },
    { RUNES, 0, "Ctrl+Alt+Backspace", "Clear the shelf", "Ctrl＋Alt＋Backspace", "清除選好的符文" },
    { RUNES, 0, "Ctrl+Alt+Space", "Cast", "Ctrl＋Alt＋空白鍵", "施法" },

    { ALWAYS, 1, NULL, "Saving and the program", NULL, "存讀檔與系統" },
    { ALWAYS, 0, "Ctrl+S / Ctrl+R", "Save / restore a game", "Ctrl＋S／Ctrl＋R", "存檔／讀檔" },
    { ALWAYS, 0, "Ctrl+M / Ctrl+F", "Music / sound effects on or off", "Ctrl＋M／Ctrl＋F", "音樂／音效開關" },
    { ALWAYS, 0, "Ctrl+D", "Detail", "Ctrl＋D", "畫面細節" },
    { ALWAYS, 0, "Ctrl+Q", "Quit", "Ctrl＋Q", "離開遊戲" },
    { ALWAYS, 0, SETTINGS_KEY, "Settings", SETTINGS_KEY_ZH, "設定" },
    { ALWAYS, 0, "Shift+F1", "This page", "Shift＋F1", "這一頁按鍵說明" },
    { LOCK, 0, "Ctrl+F10", "Let go of the mouse", "Ctrl＋F10", "釋放滑鼠" },
    { RECORD, 0, "F12", "End the recording", NULL, "結束錄影" },
};
#define ENTRIES ((int)(sizeof entries / sizeof entries[0]))

static int mouse_lock, recording;

static int shown(int when)
{
    int modern = ENHANCED(ENH_MODERN_KEYS) != 0;
    switch (when) {
    case CLASSIC: return !modern;
    case MODERN:  return modern;
    case E_USE:   return modern && !ENHANCED(ENH_E_TAKES);
    case E_TAKE:  return modern && ENHANCED(ENH_E_TAKES);
    case LOOK:    return ENHANCED(ENH_MOUSE_LOOK) != 0;
    case INVERT:  return ENHANCED(ENH_MOUSE_LOOK) && ENHANCED(ENH_INVERT_LOOK);
    case WHEEL:   return ENHANCED(ENH_MOUSE_WHEEL) != 0;
    case ERASE:   return ENHANCED(ENH_EASY_ERASE) != 0;
    case RUNES:   return ENHANCED(ENH_RUNE_KEYS) != 0;
    case LOCK:    return mouse_lock;
    case RECORD:  return recording;
    default:      return 1;
    }
}

/* the lines, a blank one before each heading after the first */
static int lines(const struct settings_line **out)
{
    static struct settings_line l[2 * ENTRIES];
    int i, n = 0, zh = settings_text_chinese();
    for (i = 0; i < ENTRIES; i++) {
        const struct entry *e = &entries[i];
        if (!shown(e->when) || (zh && !e->text_zh)) continue;     /* no Chinese: said on the line before */
        if (e->heading && n) {
            l[n].key = l[n].text = "";
            l[n++].heading = 0;
        }
        l[n].key = zh && e->key_zh ? e->key_zh : e->key;
        l[n].text = zh ? e->text_zh : e->text;
        l[n++].heading = e->heading;
    }
    *out = l;
    return n;
}

void port_key_help_init(int lock, int record)
{
    mouse_lock = lock;
    recording = record;
    settings_set_help(lines);
}
