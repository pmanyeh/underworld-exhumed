# P10.11 — 語系不影響存檔

**結果：PASS。** 同一段公開 replay 分別以英文、zh-TW（不開 HD）、zh-TW＋HD scroll＋HD 游標執行，
所有會存檔的 session，存檔檔案逐 byte 相同；英文模式的存檔也與 DOS golden 相同。

| 項目 | 值 |
| --- | --- |
| 日期 | 2026-10-23 |
| 依據 | v2.5 計畫 P10.11；[P10A F1](P10A_BATCH1_REPORT.md#i-findings--blockers)（HD scroll 把排版後的游標寫回遊戲記憶體） |
| Catalog | `build/i18n/zh-TW.uwloc`，SHA `48e68d6e…`（P10A Batch 1 的最終版，231 筆） |
| 腳本與結果 | [evidence/p10-11/](evidence/p10-11/)（`p10-11-saves.py.txt`、`p10-11-saves.json`：只有雜湊與計數） |

這是存檔相容性檢查，不是暫停中的 HD 錄影回歸：使用的是 `tests/replay` 的公開 replay，不是 P8 HD 錄影。

## 方法

三種模式（`--sound 0,0` 之外）：

| 模式 | 參數 |
| --- | --- |
| `en` | `--locale en` |
| `zh-TW` | `--locale zh-TW --i18n-file <catalog>` |
| `zh-TW-hd` | 同上，加 `--hd-scroll --hd-cursor --font NotoSansCJKtc-Regular.otf` |

- 每個公開 session（`sound*` 不存檔，略過）與 3 個 setup session 各跑三種模式。
- 需要存檔起點的 session（`load`、`fallpause`、`overpay`、`powerkill`、`tooheavy`），起點取自**同一模式**上游 session 的存檔，
  所以語系的影響若進入存檔，會一路傳下去。
- 比對 `SAVE1`–`SAVE4` 內每個檔案的 SHA-256；`--i18n-log` 與 `--scroll-log` 證明翻譯確實被查到、HD scroll 確實執行並寫回游標。

## 結果

| Session | 存檔檔案 | 三模式相同 | 英文＝DOS golden | 翻譯命中（zh-TW／HD） | HD scroll 事件 |
| --- | ---: | :---: | :---: | ---: | ---: |
| fallrestore | 4 | ✅ | ✅ | 5／5 | 90 |
| items | 4 | ✅ | ✅ | 4／4 | 88 |
| savecursor | 4 | ✅ | ✅ | 5／5 | 90 |
| setup/headless | 4 | ✅ | —（無 golden 存檔） | 2／2 | 44 |
| setup/tidy-heavy | 4 | ✅ | — | 2／2 | 44 |
| setup/trader | 4 | ✅ | — | 2／2 | 44 |
| fallpause | 4 | ✅ | ✅ | 0／0 | 112 |
| load | 4 | ✅ | ✅ | 0／0 | 0 |
| overpay | 4 | ✅ | ✅ | 16／16 | 79 |
| powerkill | 4 | ✅ | ✅ | 0／0 | 0 |
| tooheavy | 4 | ✅ | ✅ | 4／4 | 96 |

`female`、`intro`、`newgame`、`talk`、`walk` 不存檔（`talk` 有 14／8 次翻譯命中、55 個 HD scroll 事件，但沒有存檔可比）。
三種模式的程式結束碼都是 0。

## 解讀

- 程式碼面：HD scroll 寫回的欄位（`cur_x`、`cur_y`、`start_line`、`font_color`）在 DGROUP 的 `main_scroll`／`npc_scroll`
  （DS:938、DS:94D）；UW1 的存檔只寫 `PLAYER.DAT`、`LEV.ARK`、`BGLOBALS.DAT`、`DESC`，不寫 scroll 狀態。
- 實測：寫回的游標也沒有透過遊戲流程間接改變存檔內容。
- F1 仍然成立：寫回的游標會讓 HD 錄影跟 catalog 綁在一起（暫停中的錄影規則處理）；但**它不影響存檔**。

## 尚未涵蓋

- **不同文字參數**（字型、字級、行距）：P10T 完成後用同一份腳本加一個模式重跑。
- 更大量的翻譯：P10B／P10C 每批完成後可重跑（約數分鐘），catalog 變大時確認仍然相同。
- 完整的跨語系存檔 QA（中文存檔用英文讀、反之亦然）仍在 P13。
