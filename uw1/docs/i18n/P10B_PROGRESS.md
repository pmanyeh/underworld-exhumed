# P10B 進度（對話翻譯）

依使用者 2026-10-23 的指示：P10B-2 起連續翻譯，不逐批停下等確認，也不產生審閱工作表；譯文合不合適，留待遊戲內實際檢視。
每批仍跑同一套檢查，譯文維持 `draft`（`reviewed` 只來自人工審閱紀錄）。批次與 NPC 見 [P10B 對照表](P10B_CONVERSATION_MAP.md)，
第 1 批的完整報告見 [P10B-1_REPORT.md](P10B-1_REPORT.md)。

每批的檢查：`l10n.py validate`、該批與整份 master 的 `l10n.py authority`、[layout_qa.py](../../tools/i18n/layout_qa.py)、
`l10n.py build` 兩次比對 SHA、`l10n.py coverage`。證據（只有 key 與計數）在 `evidence/<批次>/`。

| 批次 | NPC | 譯為 draft | grammar-sensitive | 豁免 | 排版（訊息捲軸／對話框，超過一頁） | Catalog | 覆蓋率 |
| --- | --- | ---: | ---: | ---: | --- | ---: | ---: |
| P10B-1 | 古利克、哈格巴德、凱查瓦爾、維尼克斯 | 254 | 2 | 2 | 164／90，0／5（原文 `\m`） | 485 | 6.95% |
| P10B-2 | 拉努戈、艾布、賈卡爾、瑞緹查爾、德洛格 | 132 | 4 | 1 | 74／58，0／0 | 617 | 8.84% |

## 各批備註

- **P10B-2：** 拉努戈的 he／she／himself／herself 是代入 3594:13 模板的稱呼片段，標 `grammar-sensitive` 留給 P11。
  3594:35 的 writ 是「寫下」，不是道具「洛恩的文書」，註記豁免。惡鬼族守衛（艾布、賈卡爾、德洛格）的破英文譯為口語、簡短的中文。
