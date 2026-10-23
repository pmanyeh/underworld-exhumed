# P10C — 書本、文字、陷阱訊息與剩餘的遊戲訊息

**結果：PASS。** 可直接翻譯而尚未翻的最後 189 筆（P10B 收尾時的清單）：177 筆譯為 `draft`，12 筆改標 `grammar-sensitive`
留給 P11。依使用者 2026-10-23 的指示連續翻譯，不產生審閱表；譯文在遊戲內檢視時再調整。

| 項目 | 值 |
| --- | --- |
| 日期 | 2026-10-23 |
| 範圍 | block 3 書本與卷軸 70、block 8 告示與墓碑 98、block 9 陷阱與事件訊息 14、block 1 遊戲訊息 7 |
| Catalog | 5,101 筆，SHA `8823ebf3…152d`（兩次 build 相同） |
| 覆蓋率 | 70.51% → 72.95% |
| 證據 | [evidence/p10c-1/](evidence/p10c-1/)（只有 key 與計數） |

## 翻譯原則

- **玩家要輸入的字保留英文**（使用者 2026-10-23 決定）：
  - 祈禱語（RA、LU、UN、HUNN、SAHF、FAL、ONO、LAHN、IMU、SOL、ORA、ANRA、Summ Ra、Mu Ahm、Om Cah；盜賊指南裡的 AAM、MUL、ROMM）。
  - 符文（Uus Por、Ylem Por、An Corp Mani、3:196 的符文清單；Syn、Flas、Tor、Mes 不是真的符文，照原文）。
  - 伊洛莫的暗語：3:171 的書名「Folanae Fanlo」保留英文，玩家才找得到古爾斯坦說的那本書。
- 書名、人名、地名依 Authority（例如《欺詐之書》、上議廳、大冥河深淵墓園、施法材料）。
- 破英文照原文的笨拙程度譯：蜥蜴人代寫的紙條用倒裝語序；洞穴巨人寫給塞薩爾的錯字賀卡用錯別字。
- 墓碑「Thost the Fi t」字跡磨損，譯為「鬥 士索斯特」，保留缺字。

## 改列 P11

block 8 的 384–395（a_wall、a_door、a_princess chained to a wall 等）是 look 描述，帶 `a_`／`an_` 冠詞標記，
屬物件命名，改標 `grammar-sensitive`。

## 驗證

| 檢查 | 結果 |
| --- | --- |
| `l10n.py validate` | PASS，0 problems，0 drift |
| `l10n.py authority`（這 4 個 block／整份 master） | 0 errors、0 warnings |
| `l10n.py build` ×2 | 5,101 筆，SHA 相同 |
| `verify_localized_replay.py` | PASS，16 個 session，35 個翻譯命中與 master 相同，狀態與存檔與 DOS golden 相同 |
| 排版 QA | 326 筆 0 失敗，全部在訊息捲軸（三行）。3:2（珊卓拉的信）與 3:106（給拉克希的信）四行，會翻一頁；書本在訊息捲軸顯示，英文原文同樣會翻頁 |

**工具：** [layout_qa.py](../../tools/i18n/layout_qa.py) 原本只區分對話框與選項；現在對話以外的字串（block 0xE00 以下）
都放進訊息捲軸試排。

## 剩下的工作

可直接翻譯的靜態文字已全部完成。其餘依 P10.10 分類：模板句 154、組句片段 977、物件名稱 413 在 P11；
點陣圖 UI 261 在 P12；玩家輸入比對字串 90 不翻（P11.4 的中文輸入對照處理）。
