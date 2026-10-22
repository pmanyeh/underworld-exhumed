# UW1 繁體中文翻譯資料（譯者說明）

這個資料夾是 UW1 繁中翻譯的**唯一來源**。遊戲執行時讀的 `.uwloc` catalog 由工具從這裡產生，
不要手改 catalog，也不要把譯文寫進 C 程式碼。

| 檔案 | 內容 |
| --- | --- |
| `master.tsv` | STRINGS.PAK 的靜態字串，以 resolved `(block,index)` 識別 |
| `literals.tsv` | 程式碼裡的 hard-coded 文字（Phase 12 才接 runtime；目前只保存） |

`master.tsv` 只收**有翻譯工作的列**（有譯文、備註、非預設狀態或人工 flags）。
沒出現的 key 就是尚未翻譯。要找下一批要翻的字串，用工具匯出含英文的工作表（只留本機）：

```sh
python3 tools/i18n/l10n.py export --pak "$UW1_DATA/DATA/STRINGS.PAK" --cnv "$UW1_DATA/DATA/CNV.ARK" \
  --untranslated --category conversation --output build/i18n/p9/workbook.tsv
```

在工作表只改 `target_zh_TW`、`status`、`context`、`notes`、`flags` 五欄，其餘欄位是唯讀參考
（`auto_context` 會標出對話 NPC，`previous_source`／`next_source` 是前後條目，
`runtime_hits`／`runtime_events` 來自實際遊玩 trace）。改完用 `l10n.py import ... workbook.tsv` 合併，
再 `validate`。也可直接編輯 `master.tsv`，之後執行 `l10n.py normalize` 整理排序與 flags。

## 欄位

| 欄位 | 規則 |
| --- | --- |
| `block`, `index` | 十進位、無前導 0；block 0 一律寫成實際的對話／過場 block，`index` ≤ 511 |
| `category` | 由 block 決定，`normalize` 會自動填 |
| `source` | 英文原文，必須與玩家資料完全相同；不同即 **source drift**，build 會拒絕 |
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

工具依原文自動給每個條目一個 handling，人工 flags 只能讓條目變成「特殊處理」：
`direct`（可直接翻）、`template`、`object-name`、`grammar`（片段／形容詞／貼圖描述）、
`bitmap-ui`（角色建立與過場，以低解析 bitmap 字型繪製，P12）、`hardcoded`、`debug-report`
（block 24 的 dungeon report，屬除錯用），以及不計入可翻數量的 `empty` 與 `unaddressable`。
這是第一版靜態分類，人工審閱後可調整規則。
