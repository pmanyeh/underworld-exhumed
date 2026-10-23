# Translation_Guide.md 對齊報告

指南 SHA-256：`6699c932e8bac71de8434eb7fd3c29547f700183edc33644d9b39f36e0522aac`。來源是使用者指定文件；其官方手冊來源聲明沒有因此被視為已獨立核圖。

本輪調整 **94 個既有譯名**，補入 **86 個指南詞條**，目前共 **980 個詞條**。

329 詞有直接指南或明示概念推導依據；70 詞僅作指南補充，不宣稱已在遊戲文本找到。

目前 969 詞為專案暫定、11 詞由使用者核准、0 詞待核。人工裁定見 USER-DECISIONS.tsv；GUIDE-ISSUES.tsv 的四筆保留差異處理歷史，並非未解問題數。

## 閱讀與核對

- `GUIDE-CHANGES.tsv`：既有詞的改名前後與指南行號。
- `GUIDE-ALIGNMENT.tsv`：每個指南短詞／符文對應的詞條、候選及採用理由。
- `GUIDE-ISSUES.tsv`：英文名稱差異、同形異義與實際效果註記。
- `GLOSSARY.tsv`：統一後的主詞表；其餘分類表同步產生。

## 重要處理

Speed 採指南「降速術」的歷史名稱，並註明它對施術者啟用 Hasted／世界物理更新減半；不把它當作指定敵人的獨立 Slow 法術。
Lightning 與 Electrical Bolt、Resist Fire 與 Flameproof 以符文組合確認對應；保留遊戲原始英文名稱。
Book of Truth 與遊戲 Book of Honesty 依 Ravenhurst 作者與八聖物語境對應為同一聖物，採指南「真實之書」，不把 Honesty 與 Truth 的一般語意合併。
Paladin 選指南候選「聖騎士」，Ranger 採「巡林漢」；Shadow Beast 選「暗影獸」，避免與 Shadowlords 的「影魔」混在一起。
Seers of the Moonstone 依分類分為「月之石預言者」（組織）與「月石先知會所」（地點）。
使用者已裁定 Sir Almric 與 Baron Almric 均譯阿姆瑞克男爵；Lady Arriana 與 Arial 暫視為不同人物，保留雅莉安娜女爵／艾莉雅。其餘原待核詞沿用現行譯名，待核清單已清空。Chief Goldsword 沒有合併為 Goldthirst。

本次只整理權威詞彙與規則；master.tsv、literals.tsv、runtime、符文及祈禱語輸入碼保持原樣。手冊圖檔、原始 OCR 及私有工作紀錄未納入版本控管。
