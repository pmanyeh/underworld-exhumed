# 按鍵說明頁：F11「按鍵說明」分頁與 Shift＋F1（2026-10-28）

使用者決定（2026-10-28）：在 F11 設定畫面新增一頁「按鍵和操作說明」，快速鍵 Shift＋F1，內容只顯示目前有效的按鍵。
清單內容與用詞經使用者確認；原版 Ctrl＋D／M／F 的作用依原始碼確認後定稿。

## 操作

- 按 **Shift＋F1** 打開設定畫面並直接到「按鍵說明」分頁；在這一頁再按一次 Shift＋F1 關閉。F11、Esc 照常關閉。
- 單按 **F1** 仍是原版的選項畫面，不受影響。
- 方向鍵上／下或滑鼠滾輪捲動內容；左／右、Tab 切換分頁。這一頁沒有可修改的項目。
- 中文模式用中文，英文模式用英文。整頁有非 ASCII 文字時一律用 HD 字型，避免「W」與「A／D」字型不一。

## 內容（依本次執行的設定動態顯示）

按鍵在 `init_player`（`game/PLAYER.C`）依啟動時的強化功能綁定一次，所以說明頁讀同一份 `ENHANCED`：
執行中才在 F11 開啟的強化功能，要重新啟動後才會出現在說明頁（與實際按鍵一致）。

| 段落 | 顯示條件 |
| --- | --- |
| 移動：原版 W／S／X／A／D／Z／C／Q／E、Shift＋字母單步、J 跳、1／2／3 視角 | 未開 `modern-keys` |
| 移動：現代按鍵 WASD、方向鍵轉向與視角、數字鍵盤 5、Ctrl＋方向鍵、空白鍵／左 Shift 跳躍、飛行升降 | 開 `modern-keys` |
| 互動：F1～F10、對話中 1～4 | 一律 |
| 互動：E（使用或交談）、Q、C、X、V／B、Z、R／F、G、H | 開 `modern-keys`；E 在開 `e-takes` 時改為「能撿的收進背包、同類自動疊上」 |
| 戰鬥：. ／ ; ／ P 刺擊／揮砍／敲擊，按住蓄力 | 一律 |
| 滑鼠：左鍵移動、左右鍵跳躍、右鍵查看（拖曳撿起）或 F2～F6 選的動作 | 一律 |
| 滑鼠：Tab、方向鍵移動游標 | 未開 `modern-keys` |
| 滑鼠：反引號／中鍵切換滑鼠視角（上下反轉另註） | 開 `mouse-look`（`invert-look`） |
| 滑鼠：滾輪 | 開 `mouse-wheel` |
| 符文快捷鍵 Ctrl＋Alt＋字母／Backspace／空白鍵 | 開 `rune-keys` |
| 存讀檔與系統：Ctrl＋S／R／M／F／D／Q、F11（Mac：Cmd＋,）、Shift＋F1 | 一律 |
| Ctrl＋F10 釋放滑鼠 | 滑鼠鎖定模式 |
| F12 結束錄影 | 以 `--record` 錄影時 |

依原始碼確認的作用：Ctrl＋S／R／M／F／D／Q 打開選項面板的存檔、讀檔、音樂、音效、畫面細節、離開頁（`ui/OPTIONS.C`）；
F7 切換背包／屬性面板（`pull_chain`）；F8 施放架上的符文；F9 追蹤技能；F10 睡覺（有鋪蓋用鋪蓋，否則露營）；
未選圖示時右鍵是查看、拖曳是撿起（`mous_in_3d`）；E 對人物是交談，其餘是使用，開 `e-takes` 時能撿的先撿。

## 實作

- **Exhume 執行環境**（子模組 `exhume`，分支 `feature/presentation-overlay`）：
  - `ui/settings.h/.c`：新增 `SET_TAB_HELP` 與 `settings_set_help()`；遊戲提供說明行，每次繪製時重新取得。
    未提供時分頁不顯示，UW2 不受影響。另加 `settings_show_tab()`、`settings_tab()`。
  - 分頁寬度改為依名稱寬度分配：八個分頁時英文「Enhancements」不再壓到隔壁。
  - 捲軸與「下面還有 N 項」抽成共用的 `scroll_bar()`，設定列與說明頁共用。
  - `platform/sdl3/plat_sdl3.c`：`handle_key` 自行追蹤左右 Shift（不靠 SDL 修飾鍵），所以輸入腳本與實體鍵盤走同一路徑。
    Shift＋F1 的按下與放開都不送進遊戲。
  - 測試：`runtime/tests/settings_test.c` 新增 9 項（無說明行不顯示分頁、Shift＋F1 開到該頁、捲動、分頁切換與循環、Esc 關閉）；
    `tools/tests/overlay/presentation.c` 補上新函式的替身。
- **UW1**：`src/port/ui/key_help.c/.h` 產生說明行，`main.c` 在 `settings_init` 後登記；
  `settings_text.c` 提供 `settings_text_chinese()`；`settings_words.c` 加分頁名「按鍵說明」。
  `key_help.c` 不引入 `compat.h`：其 `#pragma pack(1)` 會讓 `struct settings_line` 與 `settings.c` 的配置不同（實測曾因此閃退）。
- 只影響設定畫面的顯示，不讀寫遊戲記憶體，不改錄影格式、存檔或 goldens。`src/port/` 以外的原始碼未修改。

## 驗證

- Windows 正常與除錯建置通過；`overlaycheck.py` 26 個合成情境通過。
- `settings_test.c`（本機以 Windows 替身編譯）新增 9 項全部通過；另兩項失敗是替身不支援 symlink 的既有 POSIX 測試，與本次無關。
- 實機截圖（中文經典、中文全部強化開啟、英文全部強化開啟，各捲動到底）：內容完整、不超出版面、分頁不重疊。
- 實際載入存檔的遊戲中：Shift＋F1 打開說明頁；再按 Shift＋F1 關閉後畫面與未操作時相同，F1 沒有送進遊戲；
  單按 F1 仍打開原版選項面板。
- 17 個單元測試程式全部通過（含 `test_cheat_menu` 的設定畫面測試）。
- 16 個 session × 英文／中文／中文 HD／中文 HD 自訂參數：存檔原始雜湊一致，有 DOS golden 的亦吻合；未改寫 golden。
- repocheck、`git diff --check`（Git for Windows，兩個 repo）通過。未執行 UBSan 全套與 DOS 重編譯。

本機測試版只換 EXE：新 SHA-256 `9bda58a89be25292a2c682b0ad0549628aee987be9230e870d0b666784d563d1`，
舊檔備份 `uw1/build/i18n/key-help/previous-playtest-uw1port.exe`；搭配原有 DLL、僅系統 PATH 的重播通過；
玩家 `home` 100 個檔案更新前後雜湊一致。

本機工具 `.poc-tools/key-help-check.py`、`key-help-deliver.py`（單元測試、四模式回歸）；產物 `uw1/build/i18n/key-help/`。
