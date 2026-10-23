# UW1 繁體中文翻譯權威詞彙稿

這份詞表為後續翻譯提供一致用詞，目前有 **894 個術語識別、26 個分類**，包含人名、地名、組織、種族、法術、符文、物件、裝備、技能與世界觀名詞。
每個詞條保存英文短詞、中文選詞、遊戲出處、來源層級與審閱狀態；它不是完整遊戲文本，也不會自動進入 runtime catalog。

使用者於 **2026-10-23 明確要求上傳權威檔**，因此將權威選詞與使用規則納入版本控管。手冊圖檔、原始 OCR、辨識差異原稿、本機工作表、私人路徑與執行紀錄仍留在被本機排除的 `private-authority/`。

## 檔案

|檔案|用途|
|---|---|
|[GLOSSARY.tsv](GLOSSARY.tsv)|單一權威詞彙來源，修改與決策以此為準|
|[PROPER-NOUNS.tsv](PROPER-NOUNS.tsv)|專名衍生檢視|
|[SPELLS.tsv](SPELLS.tsv)|法術、效果、符文與附魔的衍生檢視|
|[OBJECTS.tsv](OBJECTS.tsv)|物件、武器、防具、聖物與環境的衍生檢視|
|[LOCALIZATION-AUTHORITY.md](LOCALIZATION-AUTHORITY.md)|來源優先順序、採詞與修訂規則|
|[STYLE_GUIDE.md](STYLE_GUIDE.md)|台灣繁中用語、標點、對話與命名方式|
|[DECISIONS.md](DECISIONS.md)|重要選詞與語意差異的決策紀錄|
|[MANUAL-SOURCES.tsv](MANUAL-SOURCES.tsv)|中文手冊來源識別與圖檔集合雜湊；未確認的版次／年份不猜填|
|[PUBLIC-REFERENCES.tsv](PUBLIC-REFERENCES.tsv)|公開參考網址、用途、來源層級與限制|
|[SUMMARY.json](SUMMARY.json)|本次盤點與審閱狀態的計數|

## 目前狀態

- 掃描 7,091 筆遊戲文字，對應 1,129 個結構化標籤；此範圍沒有未對應標籤。
- 116 個人名、17 個地名、71 個法術／效果名稱、20 個技能；24 個符文與 24 個符石各自記錄。
- 828 詞為 `project-selected`，66 詞為 `review-needed`；**沒有人工或官方核准詞**。
- 94 個詞有手冊雙語 OCR 候選；其中 33 詞暫採與獨立語意候選相符的手冊字形，仍待原圖核實。
- 26 個祈禱語與符文識別碼保持原樣；尚未修改正式遊戲譯文或處理全文翻譯。

這是選詞稿的覆蓋範圍，不代表已找出全文所有專名。只出現一次、全小寫或由動態片段組合的名稱仍可能遺漏。

## 欄位與使用

`term_id` 是分類與英文身份的穩定識別。同英文但不同語意可以分列，例如 Sword 技能與 sword 物件、Door 角色與 door 物件；使用 `category` 與上下文判斷，禁止全文尋找取代。

- `canonical_zh_tw`：本稿目前用來保持一致的選詞；不是「已核准」的同義詞。
- `project_proposal_zh_tw`：手冊對照前的專案提案，供比較與追溯。
- `authority`：選詞依據；`TW_MANUAL_OCR_PROVISIONAL` 明確表示手冊辨識候選，尚非原圖核准。
- `status`：`project-selected` 是暫定選詞；`review-needed` 有手冊字形／身份差異待核。未來人工核准須記錄決策後才可標 `approved`。
- `confidence`：證據信心，與審閱狀態不同；不因出現次數多就自動升為核准。
- `game_keys`：英文詞形出現的 `block:index` 上下文；字面出現不保證就是該詞義。
- `direct_keys`：結構化標籤的直接對應，與一般詞形搜尋分開。
- `manual_source_ids`、`manual_pages`：來源及掃描檔名加 L／R／FULL 方位，**不是已確認的印刷頁碼**。
- `public_source_ids`：對應公開來源表；單篇攻略用例不是長期共識或官方定名。
- `scope`：UW2 共用僅為候選，本次未以 UW2 資料驗證。

未在目前手冊擷取中找到對應，不代表手冊未收錄。手冊只出現相同中文字，也不表示英文身份已確認。
手冊辨識差異原稿保留本機，發布檔只保存目前選詞、來源定位與待核對狀態。

修改主詞表後須同步衍生檢視；正式翻譯仍遵循上層 [譯者說明](../README.md) 的來源雜湊、控制碼與審閱流程。
