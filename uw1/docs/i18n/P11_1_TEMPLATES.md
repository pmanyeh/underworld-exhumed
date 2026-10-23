# P11.1 — 含變數的對話句（模板）以中文顯示

**結果：PASS。** 對話中含 `@` 變數的句子，在 HD 中文文字下改顯示中文：中文模板照遊戲同樣的規則填入變數，
填入的稱呼片段也換成中文。只影響顯示，腳本、變數與存檔不變。

## 問題

`convert_string`（BABL.C）把 `@SS1`、`@GS8` 等變數換成值，再交給 say／respond／print 與選單。展開後是新的字串，
沒有「這是哪一句」的對應，所以 P10 規定含 `@` 的句子一律顯示英文展開（玩家回報：Bragit 的「my lad」句仍是英文）。

## 作法

| 檔案 | 做什麼 |
| --- | --- |
| `src/port/i18n/localization.{c,h}` | 取字串時也記下模板的譯文；`port_i18n_template(legacy)` 取得它。`port_i18n_display` 對模板仍回傳英文（原行為不變） |
| `src/conv/BABL.C` | `port_convert_display`：照 `convert_string` 的規則讀變數（G／P／S／C、I／S、索引），填入中文模板；填入的字串有譯文就用譯文（模板再遞迴展開），沒有就用英文展開。結果用 malloc，不進腳本的 heap 與記憶體。`port_template_shown`：HD 文字開啟且有模板譯文時才做 |
| `src/conv/BABL.C` say／respond | 有模板譯文時把中文顯示文字交給 npc_say／play_respond |
| `src/conv/CONVERSE.C` print | 旁白同上 |
| `src/port/presentation/scroll_bridge.c` | 選單選項同上（顯示文字由 bridge 保管到下一個選單） |
| `src/conv/BARTER.C` | 交易評價由三段拼成（把握程度＋「我這是」＋評價），HD 文字下改用三段的譯文拼 |

## 翻譯

269 筆：對話模板 151、對話中的稱呼與組句片段 103、交易評價片段 15。

- 變數照原樣保留，位置依中文語序調整（例如「你好，@GS8@SS1！」＝「你好，Avatar爵士！」）。
- 稱呼片段選成兩種值都通順的詞：lad／lass → 小伙子／姑娘；Sir／Lady → 爵士／女爵；Brother／Sister → 兄弟／姊妹；
  he／she → 他／她；piece(s) → 枚；minute(s) → 分鐘。
- 玩家名字（@GS8）與物品名稱（@SS 指向物品時）仍是英文：名字是玩家輸入的；物品名稱是 P11.2。
- 由腳本 `append` 拼成的動態字串（例如「What about ...」接玩家輸入）仍顯示英文展開；譯文已備好，待之後處理動態字串。

## 驗證

| 檢查 | 結果 |
| --- | --- |
| `l10n.py validate`／authority（整份 master） | PASS；0 errors、0 warnings（4 筆豁免） |
| `l10n.py build` ×2 | 5,370 筆，SHA 相同 |
| 排版 QA | 0 失敗 |
| `test_scroll_text.py` | PASS（61,622 個斷言，含模板譯文取得、輸入行重畫） |
| `tools/i18n` 單元測試 | OK |
| 16 個公開 session，英文與 zh-TW HD 各跑一次 | 存檔兩種模式相同，也與 DOS golden 相同；HD 文字 366 段，殘留未展開變數 0 |
| 遊戲內：與 Bragit 對話（選 1、選 3） | 原本英文的兩句模板顯示為「啊，你是冤枉入獄的？我的小伙子……」「小伙子，別為自己的遭遇感到羞恥……」；交易評價顯示「……我猜我這是一筆絕佳的交易……」 |

證據：[evidence/p11-1/](evidence/p11-1/)（只有 key 與計數）。
