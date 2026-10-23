# P10.11 — 語系與文字參數不影響存檔

**結果：PASS。** 同一段公開 replay 分別以英文、zh-TW（不開 HD）、zh-TW＋HD（預設文字參數）、
zh-TW＋HD（非預設文字參數）執行，所有會存檔的 session，存檔檔案逐 byte 相同；英文模式的存檔也與 DOS golden 相同。

| 項目 | 值 |
| --- | --- |
| 日期 | 2026-10-23（第一輪三種模式；P10T 後補上文字參數，四種模式重跑） |
| 依據 | v2.5 計畫 P10.11；[P10A F1](P10A_BATCH1_REPORT.md#i-findings--blockers)（HD scroll 把排版後的游標寫回遊戲記憶體）；[P10T](P10T_TEXT_SETTINGS_REPORT.md) |
| Catalog | `build/i18n/zh-TW.uwloc`，SHA `582390b5…7f00`（審閱第 1 輪後，231 筆）。第一輪用 P10A 的 `48e68d6e…`，結果相同 |
| 腳本與結果 | [evidence/p10-11/](evidence/p10-11/)（`p10-11-saves.py.txt`、`p10-11-saves.json`：只有雜湊、計數與參數） |

這是存檔相容性檢查，不是暫停中的 HD 錄影回歸：使用的是 `tests/replay` 的公開 replay，不是 P8 HD 錄影。

## 方法

四種模式（`--sound 0,0` 之外）：

| 模式 | 參數 |
| --- | --- |
| `en` | `--locale en` |
| `zh-TW` | `--locale zh-TW --i18n-file <catalog>` |
| `zh-TW-hd` | 同上，加 `--hd-scroll --hd-cursor`；預設文字參數（jf open 粉圓、字級 5、無間距），由 `i18n/zh-TW/text-settings.ini` 讀入 |
| `zh-TW-hd-params` | 同上，玩家設定檔 `uw1port.cfg` 改動所有參數：字級 5.5（訊息捲軸 6、對話框 4.5）、字距 0.3（對話框 -0.2）、中英間距 1、行距 0.5（訊息捲軸 1.5）、訊息捲軸黑框 0.5 且不反鋸齒、對話框改用 Noto Sans CJK TC |

- 每個公開 session（`sound*` 不存檔，略過）與 3 個 setup session 各跑四種模式。
- 需要存檔起點的 session（`load`、`fallpause`、`overpay`、`powerkill`、`tooheavy`），起點取自**同一模式**上游 session 的存檔，
  所以語系或參數的影響若進入存檔，會一路傳下去。
- 比對 `SAVE1`–`SAVE4` 內每個檔案的 SHA-256；`--i18n-log` 與 `--scroll-log` 證明翻譯確實被查到、HD scroll 確實執行並寫回游標；
  port 的記錄列出每個 box 實際採用的參數，確認非預設參數有被讀入。

## 結果

| Session | 存檔檔案 | 四模式相同 | 英文＝DOS golden | 翻譯命中（zh-TW／HD） | HD scroll 事件（預設／非預設） |
| --- | ---: | :---: | :---: | ---: | ---: |
| fallrestore | 4 | ✅ | ✅ | 5／5 | 90／90 |
| items | 4 | ✅ | ✅ | 4／4 | 88／88 |
| savecursor | 4 | ✅ | ✅ | 5／5 | 90／90 |
| setup/headless | 4 | ✅ | —（無 golden 存檔） | 2／2 | 44／44 |
| setup/tidy-heavy | 4 | ✅ | — | 2／2 | 44／44 |
| setup/trader | 4 | ✅ | — | 2／2 | 44／44 |
| fallpause | 4 | ✅ | ✅ | 0／0 | 112／112 |
| load | 4 | ✅ | ✅ | 0／0 | 0／0 |
| overpay | 4 | ✅ | ✅ | 16／16 | 79／79 |
| powerkill | 4 | ✅ | ✅ | 0／0 | 0／0 |
| tooheavy | 4 | ✅ | ✅ | 4／4 | 96／96 |

`female`、`intro`、`newgame`、`talk`、`walk` 不存檔（`talk` 有 14／8 次翻譯命中、55 個 HD scroll 事件，但沒有存檔可比）。
四種模式的程式結束碼都是 0。

這組非預設參數在公開 replay 中沒有多出 [MORE] 翻頁。另以「大字」參數（字級 7、行距 +3）重播 5 個存檔 session，
`tooheavy`、`overpay` 各多出一次 [MORE]，存檔仍與預設 HD 相同（[P10T](P10T_TEXT_SETTINGS_REPORT.md) C 節）。

## 解讀

- 程式碼面：HD scroll 寫回的欄位（`cur_x`、`cur_y`、`start_line`、`font_color`）在 DGROUP 的 `main_scroll`／`npc_scroll`
  （DS:938、DS:94D）；UW1 的存檔只寫 `PLAYER.DAT`、`LEV.ARK`、`BGLOBALS.DAT`、`DESC`，不寫 scroll 狀態。
- 實測：寫回的游標、不同的文字參數與多出的翻頁，都沒有透過遊戲流程間接改變存檔內容。
- F1 仍然成立：寫回的游標會讓 HD 錄影跟 catalog 與文字參數綁在一起（暫停中的錄影規則處理）；但**它不影響存檔**。

## 尚未涵蓋

- 更大量的翻譯：P10B／P10C 每批完成後可重跑（約數分鐘），catalog 變大時確認仍然相同。
- 完整的跨語系存檔 QA（中文存檔用英文讀、反之亦然）仍在 P13。
