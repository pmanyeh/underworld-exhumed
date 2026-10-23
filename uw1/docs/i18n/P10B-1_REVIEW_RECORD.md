# P10B-1 審閱紀錄（P10.5.1）

| 項目 | 值 |
| --- | --- |
| 日期 | 2026-10-23 |
| 審閱對象 | P10B-1 的 254 筆 draft（block 3604、3603、3591、3593），加上 [審閱第 1 輪](P10A_REVIEW_ROUND1.md) 修改、待確認的 6 筆 |
| 審閱方式 | **使用者逐筆審閱**，在審閱表的 review_result 欄填寫結果 |
| 審閱工作表 | `build/i18n/review/P10B-1_REVIEW.xlsx`（含英文，只在本機，不提交） |
| Catalog | 485 筆，SHA `3e57452c…cba2`（與 P10B-1 相同；狀態不進 catalog） |

## 結果

| | 筆數 | 處理 |
| --- | ---: | --- |
| ok | 260 | `status` 改為 `reviewed`；notes 加「reviewed: 使用者審閱 2026-10-23（P10B-1_REVIEW）」 |
| fix | 0 | — |
| question | 0 | — |

套用前逐筆核對審閱表的 source_hash 與譯文和 master 相同、狀態為 `draft`。

| block | reviewed |
| --- | ---: |
| `3604` 古利克 | 83 |
| `3603` 哈格巴德 | 61 |
| `3591` 凱查瓦爾 | 56 |
| `3593` 維尼克斯 | 54 |
| `3651`（第 1 輪修改：16、30、42） | 3 |
| `3672`（第 1 輪修改：1） | 1 |
| `1`（第 1 輪修改：2、210） | 2 |

master 已翻譯的 485 筆至此全部是 `reviewed`；`approved` 仍只在 P13 使用。

## 驗證

- `l10n.py validate`：PASS，680 列，0 problems，0 drift。
- `l10n.py authority`（整份 master）：0 errors，0 warnings，56 info。
- `l10n.py build` ×2：485 筆，SHA 相同，與 P10B-1 提交時相同，所以 P10B-1 的重播結果照樣適用。

## 待決

- **B1** 仍待決：Crux Ansata「安卡十字」與騎士團「安塔南十字騎士團」的譯名是否統一，見 [P10B-1 報告](P10B-1_REPORT.md) D 節。
