# 戰鬥傷害數字與敵人血條（2026-10-26 更新）

F11「強化功能」最下方有兩個獨立開關，預設都關閉：

使用者已確認原有兩項功能 OK（2026-10-25）。2026-10-26 追加「敵人對玩家發動攻擊時顯示血條」，已實作、待使用者實測。

- `damage-numbers`「戰鬥傷害數字」：生物損失的血量上飄、淡出；格擋顯示「格擋」，空揮顯示視野中央的「揮空」。英文模式為 Block、Miss，也可不開 HD 訊息捲軸。
- `enemy-health`「敵人血條」：顯示受傷過，或曾對玩家發動近戰、射擊、攻擊性施法，且仍看得見的生物。綠 >50%、黃 >25%、紅 <=25%；死亡時不畫血條。

玩家自己的血量變化仍由 `flask-numbers` 顯示在血瓶旁。這兩項不改傷害計算、亂數、存檔或原版 VM 的字串。

原版存檔沒有每隻生物的最大血量。2026-10-26 依使用者回報修正：
血條以首次觸發時的個體當前 HP 為上限（受傷觸發時，在扣血前記錄），不再與 `Creature[].avghit` 的種類平均值取大。
後續受傷不重設上限；若回復到更高 HP，才上調至新的最高觀察值。
因此首次觸發時若生物已受傷，也會以當時 HP 為基準；這是顯示基準，不宣稱是無法從存檔取得的出生最大血量。讀檔、開始新遊戲、載入樓層時清除；物件釋放或槽位改成另一種生物時也清除。

## 追加攻擊觸發（2026-10-26）

- 在 AI 真正開始近戰攻擊、射擊或攻擊性施法動畫時記錄，不必命中、造成傷害，也不必先傷到敵人。
  揮空、被擋下與玩家開啟無敵時同樣適用；單純面向、追趕玩家、沒有成功啟動攻擊、防禦施法及攻擊其他 NPC 不觸發。
- 沿用原本 `enemy-health`，不新增開關；血條顯示仍受可見像素、存活與 3D 畫面限制。
  顯示資格與原本受傷標記使用相同生命週期，讀檔、換層、物件回收會清除。
- `AI.C` 的 `crit_attack`、`crit_missile_attack`、`crit_magik_attack` 在成功設定攻擊動畫後呼叫 `port_combat_attack`。
  掛鉤檢查血條開關、合法生物槽位、存活及目標為玩家，只更新獨立呈現狀態，不改傷害、AI 或亂數，不製造傷害飄字。
- `test_combat_text.py --sanitize` 現為 637 項呈現檢查與 55 項攻擊檢查；後者執行從實際來源擷取的四個 AI 函式
  與真正的遊戲橋接程式，涵蓋三種啟動路徑、失敗條件、防禦法術、其他目標、獨立開關、無敵、無傷害與無額外亂數。
- 實際遊戲以測試存檔副本令生物進入原版 AI 攻擊流程，HP 保持 43：攻擊前未顯示，開始攻擊後顯示滿血條。
  中文、英文、關閉開關對照與 UBSan 版均通過；測試開著無敵，沒有先攻擊或傷害敵人。
- 本機工具 `.poc-tools/attack-bars-check.py`（含 `--debug`），截圖與報告 `build/i18n/attack-bars/`；
  另以 `.poc-tools/attack-bars-replay.py` 在新輸出目錄重跑 16 個案例的中英文／顯示開關四種模式，存檔互相及與 DOS golden 一致。

## 血量上限修正（2026-10-26）

使用者回報尚未攻擊敵人，敵人發動攻擊時卻已顯示黃條。原先拿種類平均值當上限下限，
會把低於平均但未受傷的個體顯示為缺血。現在 `combat_text_observe` 只接受實際 HP，移除平均值參數。

- 回歸測試令種類平均值為 80、個體 HP 為 30；攻擊觸發後為 30/30，沒有傷害飄字。
- 呈現測試確認初始低 HP 為滿綠條，受傷後固定分母，回復超過舊基準才上調；物件回收與重設仍清除基準。
- 實際遊戲僅在測試存檔副本把個體 HP 設為 3（種類平均 45）：對玩家發動攻擊後為 3/3 綠條，
  兩次真正 `damage_critter` 各扣 1 後為 1/3 黃條。正常與 UBSan 版均驗證。
- 工具 `.poc-tools/light-hp-check.py --hp`（可加 `--debug`），截圖／報告 `build/i18n/light-hp-fix/hp*`。
- 本次照明參數與血條一起啟用的 16 組四模式回歸，按 `P13_CHEATS.md` 的原版註記填充例外比對；
  關閉功能的存檔仍與 DOS golden 原始位元組一致。

## 實作

- `AI.C` 的 `damage_critter` 接傷害；`COMBAT.C` 的 `do_miss` 接揮空／格擋，`do_damage` 接護甲扣到零的格擋。掛鉤只送資料，不改遊戲邏輯。
- `src/port/3d/render.c` 把現成 `RENDER_TAG` 的位元碼位置對回 mobile 槽位；`asm2c_spec.py` 的覆寫在繪製生物時設定 owner，結束、裁切放棄或缺圖時清除。不手改產生的 C。
- Exhume `438441b` 新增可選的 `ASM_MEMORY_WRITE` 觀察掛鉤。UW1 只在繪製 3D 時記錄 framebuffer 的像素歸屬；後畫的牆壁、物件、生物會覆蓋歸屬。完全被遮住的生物不進顯示快照，即使遮擋前後的色碼相同也成立。
- `presentation/combat_text.c` 保管有上限的狀態並以原子鎖發佈快照；`combat_text_draw.c` 在呈現執行緒繪製，從不讀遊戲物件。
- 動畫使用 `*Time`，不可加 `GAME_TIME()`：後者會消耗重播事件，讓開啟顯示功能的重播失去同步。
- 地圖、對話、離開遊戲畫面時隱藏；F11 開啟時不畫，快照停止更新也會隱藏。

## 驗證

- Windows 原生版與 UBSan debug 版編譯成功。
- `tools/i18n/test_combat_text.py --sanitize`：637 項呈現與 55 項攻擊檢查通過，含相同色碼的遮擋、部分遮擋、座標、血量上限、事件到期、兩個獨立開關、槽位回收與重設、繪製範圍、綠黃紅門檻與死亡時隱藏血條。
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
- `combat SLOT attack`：只把生物設為對玩家的攻擊目標，讓原版 AI 自行選擇與開始攻擊，不扣敵人 HP 或直接啟用血條。
- `combat SLOT status`：記錄 HP、可見與血條資格、動畫及目標供比對。
- `view SLOT [LIGHT]`：轉向生物、回復平視，可選光照 0～7。

實測例：載入既有測試存檔，`goto 1 17 6` → `view 240 7` → `combat 240 5`。不要對使用者的存檔執行測試指令。
