# UW1 繁體中文翻譯資料（譯者說明）

這個資料夾是 UW1 繁中翻譯的**唯一來源**。遊戲執行時讀的 `.uwloc` catalog 由工具從這裡產生，
不要手改 catalog，也不要把譯文寫進 C 程式碼。

| 檔案 | 內容 |
| --- | --- |
| `master.tsv` | STRINGS.PAK 的靜態字串，以 resolved `(block,index)` 識別 |
| `literals.tsv` | 程式碼裡的 hard-coded 文字（Phase 12 才接 runtime；目前只保存） |
| `proper-names.tsv` | 對話與書本裡加註英文的專有名詞（人名、地名、組織），由 `tools/i18n/proper_names.py` 從 authority 產生，不要手改；改了 authority 或譯文後重新產生（`test_name_gloss.py` 會檢查是否最新） |
| [authority/](authority/README.md) | 980 個術語的權威選詞稿，已依使用者指南整理；尚未套入正式遊戲譯文 |

`master.tsv` 只收**有翻譯工作的列**（有譯文、備註、非預設狀態或人工 flags）。
沒出現的 key 就是尚未翻譯。要找下一批要翻的字串，用工具匯出含英文的工作表（只留本機）：

```sh
python3 tools/i18n/l10n.py export --pak "$UW1_DATA/DATA/STRINGS.PAK" --cnv "$UW1_DATA/DATA/CNV.ARK" \
  --untranslated --category conversation --output build/i18n/p9/workbook.tsv
```

在工作表只改 `target_zh_TW`、`status`、`context`、`notes`、`flags` 五欄，其餘欄位是唯讀參考
（`auto_context` 會標出對話 NPC，`previous_source`／`next_source` 是前後條目，
`runtime_hits`／`runtime_events` 來自實際遊玩 trace）。改完用 `l10n.py import ... workbook.tsv` 合併，
再 `validate`。import 會比對工作表的英文與目前玩家資料，不同就整份拒絕（過期工作表），
相同才寫入該列的 `source_hash`。也可直接編輯 `master.tsv` 的譯文欄，之後執行
`l10n.py normalize --pak ...` 整理排序與 flags；新增列時 `source_hash` 留空，normalize 會依資料補上。

**Source drift**：遊戲資料的英文改變時，`validate`／`build` 回報 `SOURCE_DRIFT` 並失敗，不會自動更新 hash。
匯出工作表時這些列的 `source_drift` 欄為 `yes`；審閱譯文後把它清空再 import，才會記錄新的 hash。
（或在審閱後執行 `normalize --pak ... --accept-source-drift` 一次接受全部。）

## 欄位

| 欄位 | 規則 |
| --- | --- |
| `block`, `index` | 十進位、無前導 0；block 0 一律寫成實際的對話／過場 block，`index` ≤ 511 |
| `category` | 由 block 決定，`normalize` 會自動填 |
| `source_hash` | 英文原文的 SHA-256（遊戲解出的 CP437 bytes），**master 不存完整英文來源**；與玩家資料不符即 **SOURCE_DRIFT**，build 會拒絕。`authority/` 僅另存辨識術語所需的短詞／名稱，並非完整遊戲文本 |
| `target_zh_TW` | 譯文；UTF-8，不可含 NUL、tab 或 CR（換行用 `\n`） |
| `status` | `untranslated`／`draft`／`reviewed`／`approved`／`blocked` |
| `context`, `notes` | 給譯者／審稿者的說明；不影響遊戲 |
| `flags` | 逗號分隔，順序由工具決定 |

文字跳脫：TSV 內 `\n` 是真正換行，`\\` 是一個反斜線。遊戲的暫停碼 `\m` 在檔案中寫成 `\\m`。

## Status

- `untranslated`：沒有譯文（只有備註時使用）。
- `draft`：初稿，會進一般 build 以便遊玩測試。
- `reviewed`：已校對；`approved`：定稿。`l10n.py build --min-status reviewed|approved` 可只打包較高狀態。
- `blocked`：因技術或脈絡問題暫停；即使有譯文也不打包，請在 `notes` 寫原因。

## Flags

| flag | 來源 | 意義 |
| --- | --- | --- |
| `dynamic-template` | 自動 | 原文含 BABL `@` 變數（如 `@SS1`、`@GS8`）；runtime 目前仍以英文顯示，留待 P11 |
| `object-name` | 自動 | block 4／block 7 ≥ 256 的物品／生物名，含 `冠詞_名稱&複數` 語法，留待 P11 |
| `control-code` | 自動 | 原文含 `\p`、`\P`、`\m`、`\0`–`\6` |
| `grammar-sensitive` | 人工 | 會被拼接或代入其他句子的片段（如 Bragit 的 lad／lass） |
| `hardcoded-related` | 人工 | 與程式內寫死的文字拼接 |
| `showcase` | 人工 | Showcase milestone 會用到 |
| `needs-context` | 人工 | 需要遊戲畫面或劇情確認 |

## 必須保留的東西（validator 會檢查）

- `@` 模板：每個 `@SS1`、`@GI2C3` 等出現次數與完整寫法相同，可以移動位置。
- 控制碼：`\p`、`\P`、`\m`、`\0`–`\6` 的種類與順序相同；結尾的 `\m`／`\p` 必須仍在結尾。
- 明示換行數相同；不要自行加入 `[MORE]` 字樣。
- 原文空白開頭或結尾的條目是拼接片段，翻譯時保留拼接需要的空白，並加 `grammar-sensitive` 或寫 `notes`。

## 處理分類（coverage report 用）

每個條目有一個 **scope**：`production`（玩家版中文化範圍，完成率的分母）、`debug`
（block 24 的 dungeon report，以及 P10A 稽核確認沒有程式讀取的 block 1 七筆：開發者字串、試玩版訊息、
被 UW1 改用其他文字取代的提示；遊戲程式從不讀取，仍保留在 inventory，可翻但不計入完成率）、
`empty`、`unaddressable`。這七筆列在 `l10n.py` 的 `UNREAD_KEYS`，只在原文 hash 與稽核時相同時適用。

以及一個 **handling**，回答「它現在能不能直接走既有的 HD 顯示路徑」。
人工 flags 只能讓條目變成特殊處理：

| handling | 意義 |
| --- | --- |
| `direct` | 解出的字串原樣送到 scroll，HD 路徑可直接顯示譯文 |
| `template` | 含 BABL `@`，留待 P11 |
| `object-name` | 冠詞／複數語法的物品與生物名，留待 P11 |
| `grammar` | 被複製或拼接後才顯示的片段（形容詞、貼圖描述、交易估價片段、`... of <法術>`），HD 路徑目前會退回英文 |
| `bitmap-ui` | 以 `string_to_screen` 畫在低解析畫面：角色建立、過場、主選單讀檔訊息、結局畫面、對話標題的 NPC 名稱，留待 P12 |
| `input-token` | 咒語（mantra）：與玩家鍵入的文字比對，翻譯會讓咒語失效 |
| `hardcoded`、`needs-context` | 由人工 flags 指定 |
| `debug-report` | block 24 |

規則與依據的程式位置見 [Phase 9 report](../../docs/i18n/LOCALIZATION-PRODUCTION.md#p9-review-fixup--pre-p10-gate)。
匯出時可用 `--scope production` 只取玩家範圍。
