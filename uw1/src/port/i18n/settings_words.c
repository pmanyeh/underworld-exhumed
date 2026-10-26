/* P12: the settings screen's words in Traditional Chinese (ui/settings.h's settings_text): its
   own strings (Exhume's settings.c), the table's (settab.c) and the enhancements' (enhtab.c).
   An enhancement's help is "about (from source)"; the source stays as it is. */
#include "i18n/settings_words.h"
#include <stdio.h>
#include <string.h>

static const struct { const char *english, *zh; } words[] = {
    {"Cheats", "密技"}, {"Run", "執行"},
    {"full-map", "全地圖"}, {"god-mode", "無敵"}, {"bright-view", "提高亮度"},
    {"Return to safe position", "返回安全位置"},
    {"Remember safe position", "記住安全位置"},
    {"Show the whole dungeon map without changing explored areas", "顯示整層地圖，不修改探索紀錄；關閉設定並回到遊戲後生效"},
    {"Brighten the 3D view without changing light sources or exploration", "提高 3D 畫面亮度，不改變光源或探索範圍"},
    {"No health loss or death, with recovery from invalid positions", "不扣血、不死，異常位置時自動救援；關閉設定並回到遊戲後生效"},
    {"Close settings and return to the game to recover; does not change quest progress", "關閉設定並回到遊戲後，返回本層安全位置"},
    {"Remember this dry, clear floor as a rescue destination; kept only for this session", "把目前乾燥、沒有障礙的地面記為本層救援位置；只保留於本次遊玩"},
    /* the screen's own */
    {"Ultima Underworld", "地下創世紀"},
    {"Sound", "音效"}, {"Controls", "操作"}, {"Display", "畫面"}, {"Enhancements", "強化功能"},
    {"Game", "遊戲"}, {"Language", "語言與文字"},
    {"F11 opens this at any time \xC2\xB7 Esc closes", "隨時按 F11 開啟本畫面 \xC2\xB7 按 Esc 關閉"},
    {"Cmd+, opens this at any time \xC2\xB7 Esc closes", "隨時按 Cmd+, 開啟本畫面 \xC2\xB7 按 Esc 關閉"},
    {"The gear (or F11) opens this at any time \xC2\xB7 Esc closes", "隨時按齒輪（或 F11）開啟本畫面 \xC2\xB7 按 Esc 關閉"},
    {"Restart to apply", "重新啟動後生效"},
    {"v %d more below", "▼ 下面還有 %d 項"},
    {"changes play", "會改變玩法"},
    {"Enter or click to choose a folder", "按 Enter 或點一下，選擇資料夾"},
    {"That folder is not right for this", "這個資料夾不適用"},
    /* the table's */
    {"Music", "音樂"}, {"Speech", "語音"}, {"MT-32 ROMs", "MT-32 ROM"}, {"Volume", "音量"},
    {"Mouse", "滑鼠"}, {"Mouse-look speed", "滑鼠視角速度"}, {"Fullscreen", "全螢幕"},
    {"Window scale", "視窗倍率"}, {"4:3 aspect", "4:3 比例"}, {"Whole-number scaling", "整數倍縮放"},
    {"Game folder", "遊戲資料夾"}, {"Record sessions", "錄製遊玩過程"}, {"Show this at start", "啟動時顯示本畫面"},
    {"None", "無"}, {"PC speaker", "PC 喇叭"}, {"Follow", "跟隨"}, {"Lock", "鎖定"},
    {"found", "已找到"}, {"not found", "未找到"},
    {"That folder does not hold the game", "這個資料夾裡沒有遊戲"},
    {"That folder does not hold MT-32 or CM-32L ROMs", "這個資料夾裡沒有 MT-32 或 CM-32L 的 ROM"},
    {"Game language", "語言"},
    {"Traditional Chinese", "繁體中文"},
    {"The game's text and this screen in the language chosen; a saved game opens in either",
     "遊戲文字與本畫面所用的語言；存檔在兩種語言下都能讀取"},
    {"Chinese font", "中文字型"}, {"jf open huninn", "jf open 粉圓"},
    {"The typeface of the Chinese text", "中文字的字型"},
    {"Text size", "字級"}, {"The Chinese text's size, in the game's pixels", "中文字的大小（以遊戲畫面的像素計）"},
    {"Line spacing", "行距"},
    {"Space added between lines: more, and fewer lines fit a page", "行與行之間多加的間距；越大，一頁能放的行數越少"},
    {"Outline", "黑框"}, {"Thin", "細"}, {"Medium", "中"}, {"Thick", "粗"},
    {"A dark edge round each character", "每個字外圍的深色邊框"},
    {"Smooth edges", "反鋸齒"},
    {"Off: each pixel of a character all or nothing, as a bitmap font", "關閉時，字的每個像素非有即無，像點陣字"},
    {"English names", "專有名詞加註英文"}, {"Off", "關閉"}, {"First time", "每段第一次"}, {"Every time", "每次"},
    {"A name of a person, place or order with its English, in conversations and books",
     "對話與書本中的人名、地名、組織名，後面加上英文原名"},
    {"damage-numbers", "戰鬥傷害數字"}, {"enemy-health", "敵人血條"},
    {"Damage, misses and blocks float above visible enemies", "在看得見的敵人旁顯示傷害、揮空與格擋；空揮時顯示在視野中央"},
    {"Health bars above visible creatures after they take damage or attack you (estimated maximum)", "受傷過或對你發動攻擊的生物顯示血條；只顯示看得見的生物，最大血量為估計值"},
    /* the enhancements' names and what each does */
    {"skip-intro", "略過開場"}, {"wrap-menu", "選單循環"}, {"fast-panels", "面板加速"},
    {"free-heading", "貼牆不轉向"}, {"subtitles", "過場字幕"}, {"skill-messages", "技能提升訊息"},
    {"perspective", "透視校正"}, {"full-sprites", "完整物件大小"}, {"wide-pitch", "加大俯仰角"},
    {"mouse-look", "滑鼠視角"}, {"invert-look", "反轉滑鼠視角"}, {"modern-keys", "現代按鍵"},
    {"rune-keys", "符文快捷鍵"}, {"tidy-objects", "物件欄位整理"}, {"restore-at-rest", "讀檔時靜止"},
    {"flask-numbers", "血瓶法力瓶數值"}, {"auto-key", "自動開鎖"}, {"e-takes", "E 鍵撿起"}, {"mouse-wheel", "滑鼠滾輪"},
    {"The mouse wheel scrolls an open bag and goes on past [MORE]",
     "滑鼠滾輪捲動打開的袋子，也可以翻過「繼續」"},
    {"With modern-keys, E on something you can take puts it in your pack",
     "搭配現代按鍵：對著可以拿的東西按 E，直接放進背包"},
    {"Using a locked door or chest unlocks it with the key that fits, if you carry it",
     "使用上鎖的門或箱子時，背包裡有對的鑰匙就自動開鎖"},
    {"The flasks show their values, 25/32, and each change floats up from them (HD text)",
     "血瓶、法力瓶上顯示數值（例如 25/32），每次增減從瓶口飄出數字（高解析文字）"},
    {"Starts at the main menu, without the title or the introduction", "直接從主選單開始，不播標題與開場動畫"},
    {"The start menu and the saved games wrap from the last item to the first", "開始選單與存檔清單從最後一項接回第一項"},
    {"The right-hand panel slides twice as fast", "右側面板翻轉的速度加倍"},
    {"Sliding along a wall no longer turns your view", "沿著牆滑動時，視角不再跟著轉"},
    {"Cutscenes show their text while the speech plays", "過場動畫播放語音時同時顯示文字"},
    {"Says which skill a skill point or a trainer raised, and to what", "說明技能點數或訓練提升了哪項技能，以及升到多少"},
    {"Floors, ceilings and walls stay straight when the view pitches", "抬頭低頭時，地板、天花板與牆面保持筆直"},
    {"Creatures and objects keep their size when the view pitches", "抬頭低頭時，生物與物件維持原來的大小"},
    {"Look three times as far up and down, with what is behind you drawn", "上下可看的角度變成三倍，並畫出身後的景物"},
    {"The ` key or the middle button toggles mouse-look: the mouse turns the view, clicks act at the crosshair",
     "按 ` 鍵或滑鼠中鍵切換滑鼠視角：移動滑鼠轉動視角，點擊作用在準星上"},
    {"Mouse-look's up and down inverted", "滑鼠視角的上下顛倒"},
    {"UltimaHacks' keys: WASD, arrows to turn and look, Space attacks, Q looks and E uses at the cursor, Z the map, R and F the panels",
     "UltimaHacks 的按鍵：WASD 移動，方向鍵轉向與抬頭低頭，空白鍵攻擊，Q 查看、E 使用游標處，Z 地圖，R、F 切換面板"},
    {"Ctrl+Alt+letter picks a rune you have, Ctrl+Alt+Backspace clears, Ctrl+Alt+Space casts",
     "Ctrl+Alt+字母選擇擁有的符文，Ctrl+Alt+Backspace 清除，Ctrl+Alt+空白鍵施法"},
    {"A stack too heavy to take part of no longer loses an object slot, and lost slots are freed on arrival",
     "拿不動的一疊物品不再佔掉物件欄位，遺失的欄位在進入樓層時釋放"},
    {"A restored game starts at rest, without the motion or fall it was restored in",
     "讀取存檔時角色靜止，不帶著存檔當下的移動或墜落"},
};

const char *port_settings_word(const char *english)
{
    static char composed[512];
    const char *from;
    size_t i;
    if (!english) return NULL;
    for (i = 0; i < sizeof words / sizeof words[0]; i++)
        if (!strcmp(words[i].english, english)) return words[i].zh;
    /* "about (from source)" */
    if ((from = strstr(english, " (from ")) != NULL) {
        char about[400];
        size_t n = (size_t)(from - english);
        const char *zh;
        if (n >= sizeof about) return NULL;
        memcpy(about, english, n); about[n] = 0;
        if ((zh = port_settings_word(about)) == NULL) return NULL;
        snprintf(composed, sizeof composed, "%s（來自 %.*s）", zh, (int)(strlen(from + 7) ? strlen(from + 7) - 1 : 0), from + 7);
        return composed;
    }
    return NULL;
}
