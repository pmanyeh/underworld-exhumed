# UW1 繁體中文翻譯權威詞彙稿

這份詞表為後續翻譯提供一致用詞，目前有 **979 個術語識別、26 個分類**，包含人名、地名、組織、種族、法術、符文、物件、裝備、技能與世界觀名詞。
每個詞條保存英文短詞、中文選詞、遊戲出處、來源層級與審閱狀態；它不是完整遊戲文本，也不會自動進入 runtime catalog。

2026-10-23 依使用者指定的 `Translation_Guide.md` 重新整理：調整 94 個既有譯名，新增 86 個指南詞條。來源以 SHA-256 與文件行號定位；指南自述的官方手冊依據不等於本專案已獨立核對原圖。

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
|[GUIDE-SOURCES.tsv](GUIDE-SOURCES.tsv)|使用者指定指南的檔名、雜湊與來源性質|
|[GUIDE-REVIEW.md](GUIDE-REVIEW.md)|本輪指南對齊結果與重要語意處理|
|[GUIDE-ALIGNMENT.tsv](GUIDE-ALIGNMENT.tsv)|312 筆指南短詞、符文及祈禱語對應，含行號與選詞理由|
|[GUIDE-CHANGES.tsv](GUIDE-CHANGES.tsv)|94 個既有譯名的調整前後|
|[GUIDE-ISSUES.tsv](GUIDE-ISSUES.tsv)|人物身份、英文異稱及歷史法術名稱的四項處理紀錄|
|[REVIEW-NEEDED.tsv](REVIEW-NEEDED.tsv)|目前待核清單為空；已依使用者裁定結案|
|[USER-DECISIONS.tsv](USER-DECISIONS.tsv)|人工核准用詞與暫定人物分列規則，包含決策前譯名|
|[USER-DECISION-SOURCES.tsv](USER-DECISION-SOURCES.tsv)|使用者直接指示的來源、日期及適用範圍|
|[SUMMARY.json](SUMMARY.json)|本次盤點與審閱狀態的計數|

## 目前狀態

- 掃描 7,091 筆遊戲文字，對應 1,129 個結構化標籤；此範圍沒有未對應標籤。
- 129 個人名、46 個地名、71 個法術／效果名稱、20 個技能、八大美德；24 個符文與 24 個符石各自記錄。
- 957 詞為 `project-selected`，22 詞經使用者核准為 `approved`，`review-needed` 為 0。核准的是本專案用詞與暫定身份處理，並非手冊原圖或官方來源驗證。
- 329 詞有直接指南或明示概念推導依據。70 詞標為 `guide-only`，沒有捏造遊戲鍵值。
- 指南所列 40 個符文法術的組合與環級已對照遊戲原始碼。其餘 8 個未列入指南的符文法術仍保留。
- 26 個祈禱語與符文識別碼保持原樣；尚未修改正式遊戲譯文或處理全文翻譯。

後續人工裁定優先於指南：Sir Almric／Baron Almric 均譯「阿姆瑞克男爵」，仍保留英文形式的獨立詞條；Lady Arriana／Arial 暫視為不同人物，維持「雅莉安娜女爵」／「艾莉雅」。其他原待核詞沿用現行翻譯。

這是選詞稿的覆蓋範圍，不代表已找出全文所有專名。只出現一次、全小寫或由動態片段組合的名稱仍可能遺漏。

## 欄位與使用

`term_id` 是分類與英文身份的穩定識別。同英文但不同語意可以分列，例如 Sword 技能與 sword 物件、Door 角色與 door 物件；使用 `category` 與上下文判斷，禁止全文尋找取代。

- `canonical_zh_tw`：本稿目前用來保持一致的選詞；不是「已核准」的同義詞。
- `project_proposal_zh_tw`：手冊對照前的專案提案，供比較與追溯。
- `previous_canonical_zh_tw`：本輪指南整理之前的選詞，與最初提案分開。
- `aliases_zh_tw`：指南允許的異名；依分類選定單一主要名稱，不作全文替換。
- `guide_source_ids`、`guide_lines`、`guide_decision`：指南來源、行號與直接採用／概念推導／身份處理的理由。
- `source_scope`：`game-text` 有英文詞形的遊戲出處；`guide-only` 是指南補充、遊戲出處空白。詞形出現仍不代表語意已獨立確認。
- `authority`：選詞依據；`TW_MANUAL_OCR_PROVISIONAL` 明確表示手冊辨識候選，尚非原圖核准。
- `USER_TRANSLATION_GUIDE`：本輪依使用者指定文件選詞，與 `TW_MANUAL_OCR_PROVISIONAL` 分開，不冒用手冊原圖核准。
- `status`：`project-selected` 是暫定選詞；`review-needed` 有手冊字形／身份差異待核；`approved` 必須有 USER-DECISIONS.tsv 的人工決策，不代表官方手冊原圖已核實。
- `decision_source_ids`、`decision_ids`、`decision_scope`：人工核准的來源、決策編號與適用範圍。重新產生詞表時必須重播，不能用舊指南覆蓋。
- `confidence`：證據信心，與審閱狀態不同；不因出現次數多就自動升為核准。
- `game_keys`：英文詞形出現的 `block:index` 上下文；字面出現不保證就是該詞義。
- `direct_keys`：結構化標籤的直接對應，與一般詞形搜尋分開。
- `manual_source_ids`、`manual_pages`：來源及掃描檔名加 L／R／FULL 方位，**不是已確認的印刷頁碼**。
- `public_source_ids`：對應公開來源表；單篇攻略用例不是長期共識或官方定名。
- `scope`：UW2 共用僅為候選，本次未以 UW2 資料驗證。

未在目前手冊擷取中找到對應，不代表手冊未收錄。手冊只出現相同中文字，也不表示英文身份已確認。
手冊辨識差異原稿保留本機，發布檔只保存目前選詞、來源定位與待核對狀態。

修改主詞表後須同步衍生檢視；正式翻譯仍遵循上層 [譯者說明](../README.md) 的來源雜湊、控制碼與審閱流程。

## 驗證

在 `uw1/` 執行 `python tools/i18n/validate_authority.py`，會檢查詞條身份、來源、分類檢視、輸入碼及指南的法術組合／環級。
這是已納入版本控管之 Authority dataset 的正式驗證工具；從儲存庫根目錄執行 `python3 uw1/tools/i18n/validate_authority.py`（Windows 可用 `python`）不需遊戲資料、手冊圖檔、私人檔案或指南原檔。Linux、macOS、Windows 的 CI 均執行此無參數驗證；待核項目非零或選詞只有空白時，驗證失敗。
可加 `--guide <Translation_Guide.md>` 比對指南雜湊及行號；`--workbook <本機匯出工作表>` 額外核對遊戲鍵值。工具不改寫資料或載入手冊圖檔。
