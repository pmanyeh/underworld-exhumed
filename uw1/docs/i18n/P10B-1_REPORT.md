# P10B-1 — 第 1 層：Gulik、Hagbard、Ketchaval、Vernix

**結果：PASS。** 254 筆對話譯為 `draft`，2 筆組句片段標為 `grammar-sensitive`；validate、整份 master 的 authority lint
（0 errors、0 warnings）、兩次 build 相同、排版 QA（0 失敗）、中文 lookup 重播都通過。審閱表已產生，等待人工審閱。

| 項目 | 值 |
| --- | --- |
| 日期 | 2026-10-23 |
| 依據 | v2.5 計畫 P10.3（P10B）、[P10B 對照表](P10B_CONVERSATION_MAP.md) 的第 1 批 |
| 範圍 | block 3604 古利克（slot 20）、3603 哈格巴德（19）、3591 凱查瓦爾（7）、3593 維尼克斯（9）：放逐者聚落與兩位惡鬼族國王 |
| Catalog | 485 筆，SHA `3e57452c…cba2`（兩次 build 相同） |
| 證據 | [evidence/p10b-1/](evidence/p10b-1/)（數字摘要、排版 QA；只有 key 與計數） |

## A. 程式讀取稽核

[babl_audit.py](../../tools/i18n/babl_audit.py)：這 4 段對話沒有任何比對（compare／contains）字串，全部可以翻譯。

| 處理 | 筆數 |
| --- | ---: |
| 譯為 draft（NPC 台詞 90、玩家選項 164） | 254 |
| `grammar-sensitive`：3603:1、3603:2（`sex()` 選出的稱呼，代入 3603:3 的 `@SS1` 模板） | 2 |
| 模板句（`@GS8`、`@SS1`），依 P10.10 留給 P11 | 7 |

## B. 翻譯

- 口吻：古利克憤世嫉俗；哈格巴德尖酸；凱查瓦爾粗魯（原文帶口音）；維尼克斯做作、愛說「你知道的」。
- 交易用語（出價、索取、回應）沿用已審閱的 Brawnclan 譯文（3672:16–30），各 NPC 一致。
- 「Gobs」（惡鬼族的俗稱，玩家會反問）譯為「惡鬼仔」，下一句再解釋為惡鬼族。
- 維尼克斯提到的蜥蜴人語「Sseth」「'click」保留原文（玩家日後要分辨是／否）；Authority 的 Sseth 是人名，已註記 `authority-ok`。

**Authority lint 修正（7 個 warning）：** colony／colonize →「殖民聚落」（5 處統一）、courage →「勇氣」、goblins →「惡鬼族」、
Queen →「女王」（原用「王后」）。兩處註記 `authority-ok`：3593:37 的 mouth 是嘴巴，不是生物「巨口」；
3593:38 的 Crux Ansata 是組織名的一部分，見 D 節。

**自我校對（P10.5.1 步驟 2）：** 逐句對照英文後修改 4 句語氣：3603:12、3603:35、3591:60（原文的反諷）、3593:48。

## C. 驗證

| 檢查 | 結果 |
| --- | --- |
| `l10n.py validate` | PASS，680 列，0 problems，0 drift |
| `l10n.py authority`（這 4 個 block／整份 master） | 0 errors、0 warnings（31／56 info） |
| `l10n.py build` ×2 | 485 筆，SHA 相同 |
| 排版 QA（[layout_qa.py](../../tools/i18n/layout_qa.py)，新） | 254 筆 0 失敗；玩家選項 164 筆全在一頁內（3 行）；NPC 台詞 90 筆中 5 筆超過一頁，皆為原文結尾的 `\m` 翻頁 |
| `verify_localized_replay.py` | PASS，16 個 session；公開錄影沒有走到這 4 位 NPC，所以沒有新的命中 |
| master 是否含英文 | 只有刻意保留的 `Sseth`、`'click` |
| 覆蓋率 | production 231 → 485（3.31% → 6.95%）；對話未翻 5,011 → 4,757 |

**排版 QA 工具：** 公開錄影到不了大部分 NPC，以腳本即時操作也不穩定（P10T T2）。`layout_qa.py` 用遊戲的 HD 捲軸排版程式
（`scroll_text.c`）、預設字型與字級，在遊戲的兩個文字框（訊息捲軸 15,169–306,199；NPC 對話框 59,53–220,132）逐句排版，
回報行數、[MORE] 次數與排版失敗；玩家選項與 NPC 台詞依 BABL 稽核分開。之後每批都用它。

## D. Findings

- **B1（Authority 待決）：** Crux Ansata 單獨是「安卡十字」（world-lore），組織 Knights of the Crux Ansata 卻是「安塔南十字騎士團」，
  兩者不一致。這批照組織名的詞條翻，並註記豁免；請決定是否統一（例如「安卡十字騎士團」）。
- **B2：** 這 4 位 NPC 在公開錄影中都不會出現，遊戲內的實際顯示只能由審閱時實機確認，或日後的 HD 錄影（目前暫停）。

## E. 審閱

審閱表 `build/i18n/review/P10B-1_REVIEW.xlsx`（含英文，只在本機）：這批 254 筆，加上審閱第 1 輪修改、待確認的 6 筆，共 260 筆。
多了「角色」（NPC／玩家選項）與「排版」（行數、翻頁）兩欄。
