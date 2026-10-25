# 戰鬥傷害數字與敵人血條（2026-10-25）

F11「強化功能」最下方有兩個獨立開關，預設都關閉：

- `damage-numbers`「戰鬥傷害數字」：生物損失的血量上飄、淡出；格擋顯示「格擋」，空揮顯示視野中央的「揮空」。英文模式為 Block、Miss，也可不開 HD 訊息捲軸。
- `enemy-health`「敵人血條」：只顯示受傷過且仍看得見的生物。綠 >50%、黃 >25%、紅 <=25%；死亡時不畫血條。

玩家自己的血量變化仍由 `flask-numbers` 顯示在血瓶旁。這兩項不改傷害計算、亂數、存檔或原版 VM 的字串。

原版沒有每隻生物的最大血量，血條上限採「最高觀察血量」與 `Creature[].avghit` 較大者，所以是估計比例。讀檔、開始新遊戲、載入樓層時清除；物件釋放或槽位改成另一種生物時也清除。

## 實作

- `AI.C` 的 `damage_critter` 接傷害；`COMBAT.C` 的 `do_miss` 接揮空／格擋，`do_damage` 接護甲扣到零的格擋。掛鉤只送資料，不改遊戲邏輯。
- `src/port/3d/render.c` 把現成 `RENDER_TAG` 的位元碼位置對回 mobile 槽位；`asm2c_spec.py` 的覆寫在繪製生物時設定 owner，結束、裁切放棄或缺圖時清除。不手改產生的 C。
- Exhume `438441b` 新增可選的 `ASM_MEMORY_WRITE` 觀察掛鉤。UW1 只在繪製 3D 時記錄 framebuffer 的像素歸屬；後畫的牆壁、物件、生物會覆蓋歸屬。完全被遮住的生物不進顯示快照，即使遮擋前後的色碼相同也成立。
- `presentation/combat_text.c` 保管有上限的狀態並以原子鎖發佈快照；`combat_text_draw.c` 在呈現執行緒繪製，從不讀遊戲物件。
- 動畫使用 `*Time`，不可加 `GAME_TIME()`：後者會消耗重播事件，讓開啟顯示功能的重播失去同步。
- 地圖、對話、離開遊戲畫面時隱藏；F11 開啟時不畫，快照停止更新也會隱藏。

## 驗證

- Windows 原生版與 UBSan debug 版編譯成功。
- `tools/i18n/test_combat_text.py --sanitize`：624 項通過，含相同色碼的遮擋、部分遮擋、座標、血量上限、事件到期、兩個獨立開關、槽位回收與重設、繪製範圍、綠黃紅門檻與死亡時隱藏血條。
- `tools/i18n/test_*.py` 全部通過；`p10a-b1-text-regression.py` 的 13 組均為 0，含 ASan/UBSan、無 FreeType、文字探測與 port-check。
- `test_hd_cursor.py --window --sanitize`：8 種 SDL 合成、縮放與全螢幕情境通過；補齊此舊測試的現有呈現模組連結與互動面板替身。
- 16 個公開案例：中英文、關閉／開啟戰鬥顯示的存檔一致，有 DOS 存檔 golden 的也一致。3 個 format-6 setup 錄製自帶強化設定，依原設定建立相同起始存檔，再對後續原版錄製開關戰鬥功能。
- `asm2c.py --check`、原版遊戲來源 ASCII 檢查、`repocheck` 通過。
- 本機 DOS 位元組 Gate 未跑成：缺少 Node.js，無法啟動 DOS 後端；不以此宣稱 DOS 重編譯位元組已驗證。

本機遊戲內截圖在 `build/i18n/p12/shots/`，不加入 git：

- `combat-final.png`：受傷敵人、血條、傷害與中文格擋（飄字和血條分開）。
- `combat-en-block.png`：英文 Block 與傷害；英文模式不開 HD 訊息捲軸。
- `combat-off.png`：兩個開關都關閉的原版畫面。
- `combat-health-only.png`：只有血條；`combat-miss.png`：只有數字開關，顯示揮空。
- `combat-lit-debug.png`：UBSan debug 版的敵人、血條與傷害。
- `combat-settings.png`：F11 強化功能的中文開關與說明。

重播報告：`build/i18n/p13-combat/replays/report.json`（本機）；重跑工具 `.poc-tools/combat-replay.py`。

測試專用指令（須 `EXHUME_TEST_CHEATS=1`，讀腳本時只驗參數、不碰遊戲物件）：

- `combat SLOT DAMAGE|miss|block`：使用原版傷害或格擋／揮空處理器。
- `view SLOT [LIGHT]`：轉向生物、回復平視，可選光照 0～7。

實測例：載入既有測試存檔，`goto 1 17 6` → `view 240 7` → `combat 240 5`。不要對使用者的存檔執行測試指令。
