# P11.4 — 對話中的中文輸入與同義詞對照

**狀態：實作完成，自動測試通過；中文輸入法的實際操作待使用者在遊戲內確認。**

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
- **待使用者實機確認：** Windows 中文輸入法在遊戲視窗內的選字、送出；加拉蒙主線、強盜頭目或羅斯塔格的話題輸入。

## 尚未涵蓋

- 神殿念祈禱語（`SKILLS.C` 的 `wdialog`）、存檔名稱、物品數量仍是原版輸入框；祈禱語保留英文，畫面上也是英文。

## 測試版與發現

- 2026-10-23 組了本機測試版（`D:\git\Ultima Underworld\UW1-zhTW-playtest`，不在 repo）：本次 build 的 `uw1port.exe`、
  所需 DLL、jf open 粉圓、`text-settings.ini`、`input-aliases.tsv` 與 P10C 的 catalog；啟動參數
  `--locale zh-TW --i18n-file … --hd-scroll`。在只有系統路徑的環境啟動成功，HD 文字與對照表都有載入。
- **發現（待修）：** 測試版放在含中文的路徑時，截圖與 home（存檔）資料夾都寫不出來——移植版在 Windows 上用
  窄字元路徑開檔，非 ASCII 路徑會失敗。中文玩家很可能把遊戲放在中文資料夾，發行前要改用寬字元（UTF-8 → UTF-16）開檔。
