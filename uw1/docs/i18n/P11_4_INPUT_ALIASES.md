# P11.4 — 對話中的中文輸入與同義詞對照

**狀態：PASS。自動測試通過，中文輸入法已由使用者實機確認（2026-10-24）。**

## 問題

12 段對話用 `babl_ask` 讓玩家打字，腳本拿輸入和英文字比對（`compare`／`contains`，見 `i18n/input-keys.tsv`）。
比對字依規則不翻，中文玩家卻只看得到中文（例如加拉蒙要的 talisman／volcano 是主線終局）。

## 作法

| 層 | 檔案 | 做什麼 |
| --- | --- | --- |
| 平台（exhume） | `runtime/port/platform/plat.h`、`sdl3/plat_sdl3.c` | `plat_text_input(on)` 開關 SDL 文字輸入（輸入法）；`plat_text_take` 取出已確定的 UTF-8 文字。按鍵照常以按鍵送出 |
| HD 捲軸 | `src/port/presentation/scroll_text.{c,h}` | `scroll_text_mark`／`scroll_text_rewind`：輸入行每次按鍵後重畫 |
| 輸入行 | `src/port/presentation/scroll_bridge.c` 的 `port_scroll_ask` | 在 HD 訊息捲軸顯示 `>` 與輸入內容（中文用 HD 字型）；ASCII 從遊戲原本的按鍵讀，中文從輸入法讀；Enter 確定、Esc 取消、Backspace 刪一個字 |
| 對照 | `src/port/i18n/input_alias.{c,h}`、`i18n/zh-TW/input-aliases.tsv` | 依對話（slot）把中文同義詞換成英文比對字（長的優先），再去掉非 ASCII 字元。只輸入英文時原樣送出 |
| 接點 | `src/conv/CONVERSE.C` 的 `babl_ask` | HD 捲軸開啟且對照表已載入時走 `port_scroll_ask`，否則照原版 `wdialog` |

不變的部分：腳本、比對規則、BABL 語意、存檔；英文版與沒有 `--hd-scroll` 的執行都走原版輸入框；
進入遊戲字串的永遠是 ASCII（P11 gate「no UTF-8 enters legacy VM buffers」）。

## 對照表

`i18n/zh-TW/input-aliases.tsv`：slot、block、index、source_hash、token（比對字）、aliases（`|` 分隔）。
64 列對應 `input-keys.tsv` 的每個比對字串。只收這些單字，不收整句英文。

- 有中文同義詞：所有在中文介面只看得到中文的詞（話題、人名、聖物、火山、通關語的音譯等）。
- 照畫面英文輸入（不配中文）：囚犯教的蜥蜴人語單字（slot 24）、古爾斯坦的暗語 Folanae（slot 187）；
  它們在中文版畫面上本來就以英文出現。數字 726 另可打「七二六」。

## 驗證

- `tools/i18n/test_input_alias.py`：C 模組實測（長詞優先、只套用當段對話、去掉其他文字、截斷、格式錯誤的表不載入），
  以及對照表與 `input-keys.tsv` 一致、同一段對話裡一個中文詞不對到兩個英文字、LF／UTF-8。
- `tools/i18n` 全部 123 個測試 OK；`verify_localized_replay.py` PASS（原版路徑、存檔與 DOS golden 相同）。
- **使用者實機確認 OK（2026-10-24）：** Windows 中文輸入法在遊戲視窗內的選字、送出，以及 Lakshi Longtooth 的話題輸入。
- **實機測試的捷徑（2026-10-24）：** 用測試指令從測試存檔傳送到 NPC 旁邊，開啟對話，停在輸入框後交給玩家（本機工具，不在 repo）。
  預設是第 4 層的 Lakshi Longtooth（slot 18）：選「你能告訴我關於……」後出現 `>` 輸入框，可打「羅斯塔格」「坑」「騎士團」等。
  加拉蒙（slot 27）不在初始地圖上，`talk` 叫不出來；他的分支由對照表與單元測試涵蓋，實機留到 P13 的完整遊玩。

## 尚未涵蓋

- 神殿念祈禱語（`SKILLS.C` 的 `wdialog`）、存檔名稱、物品數量仍是原版輸入框；祈禱語保留英文，畫面上也是英文。

## 測試版與發現

- 2026-10-23 組了本機測試版（`D:\git\Ultima Underworld\UW1-zhTW-playtest`，不在 repo）：本次 build 的 `uw1port.exe`、
  所需 DLL、jf open 粉圓、`text-settings.ini`、`input-aliases.tsv` 與 P10C 的 catalog；啟動參數
  `--locale zh-TW --i18n-file … --hd-scroll`。在只有系統路徑的環境啟動成功，HD 文字與對照表都有載入。
- **發現（已更正）：** 原本記錄「測試版放在含中文的路徑時，截圖與 home 都寫不出來」。複查（2026-10-23）發現那次測試
  的參數裡，含空白的路徑（`Ultima Underworld`）沒加引號，才是失敗原因；舊版程式加上引號後，在繁體中文 Windows
  （系統字碼頁 CP950）上也能寫入中文路徑。
- **仍然修正的部分：** 移植版經由 SDL 取得的路徑（資料夾選擇器、拖放）是 UTF-8，C 執行階段卻以系統字碼頁讀檔名，
  路徑超出該字碼頁、或系統字碼頁不是繁中時會開不了。exhume `6758723` 在 Windows 程式加入 `activeCodePage=UTF-8`
  的資訊清單（Windows 10 1903 起），檔名、命令列、環境變數一律 UTF-8。實測：中文資料夾下的 home（含 SAVE0、設定檔）
  與中文檔名的截圖都正常；16 個公開 session 的存檔與 DOS golden 相同，`verify_localized_replay.py` PASS。

## 之後的改進（2026-10-24 使用者提出，放在後面的改進階段）

1. 打字問話題能否改成選擇：要先分辨每個打字點有沒有解謎成分（加拉蒙的聖物與火山、暗語 Folanae、蜥蜴人語、726），
   解謎類可能要維持打字。
2. 關鍵字不對時 NPC 沒有任何回應，體驗很差：要先查是原版腳本本來就這樣，還是中文詞不在對照表、送出了空字串。

細節記在主計畫書 §P13.7。
