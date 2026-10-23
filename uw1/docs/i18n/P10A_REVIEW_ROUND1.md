# P10A 審閱 第 1 輪（P10.5.1）

| 項目 | 值 |
| --- | --- |
| 日期 | 2026-10-23 |
| 審閱對象 | master 的全部 231 筆 draft（P10A Batch 1 的 179 筆、PoC 的 52 筆，含 Bragit） |
| 審閱方式 | Claude 逐筆對照英文預審並預填 ok／fix，**使用者確認接受**預審結果與 6 筆修改建議 |
| 審閱工作表 | `build/i18n/review/P10A_REVIEW_Claude預填.xlsx`（含英文，只在本機，不提交） |
| Catalog | 231 筆，SHA `582390b5…7f00` |

## 結果

| | 筆數 | 處理 |
| --- | ---: | --- |
| ok | 225 | `status` 改為 `reviewed`；notes 加「reviewed: 使用者確認（Claude 預審）2026-10-23」 |
| fix | 6 | 依建議修改，維持 `draft`；notes 註記，下一輪確認 |
| question | 0 | — |

### reviewed（225 筆）

- `1`：3–6、9–21、24–27、30、32–33、62、69、93–97、99–102、119–129、131–136、142、146、148–165、168、171、177–186、191–209、211–215、224、228–243、246–255、258–259、270–274、276–280（147 筆）
- `7`：0–1、83（3 筆）
- `3651`：1–15、17–29、31–41、43–49（46 筆）
- `3672`：2–30（29 筆）

### fix（6 筆，維持 draft）

| key | 修改 |
| --- | --- |
| `3651:16` | 第三段「好地方」→「最佳地點」（原文 best） |
| `3651:30`、`3651:42` | 「生命十字」→「聖符」（Authority 的 ankh） |
| `3672:1` | 去掉「放逐下來的放逐者」的重複。**與審閱建議不同：** 建議的寫法拿掉了 Authority 的「放逐者」（outcast），lint 報 warning；改為保留該詞的寫法 |
| `1:210` | 「這個法術環級的法術」→「這個環級的法術」 |
| `1:2` | 「這把鑰匙不合」→「這把鑰匙對不上這個鎖」 |

## 驗證

- `l10n.py validate`：PASS，424 列，0 problems，0 drift。
- `l10n.py authority`（整份 master）：0 errors，0 warnings，23 info。
- `l10n.py build`：231 筆，SHA `582390b574407f3b1e73797987174b304694bef872e16a7383fb6367144d7f00`。
- `tools/i18n` 單元測試：107 個，OK。
- `verify_localized_replay.py`：PASS，16 個 session，35 個不同的翻譯命中與 master 相同，狀態與存檔與 DOS golden 相同。

## Findings

- **R1（lint 漏網）：** Authority 的 `Ankh` 是大寫，`authority` 子命令區分大小寫，所以原文小寫的 `an ankh` 沒有被比對到，「生命十字」因此沒被抓出。
  一般名詞（object、world-lore 等）應不分大小寫比對；這是工具修改，P10B 前另做。
- 依 P10.5.1，`reviewed` 只來自人工審閱紀錄：本輪的 reviewed 由使用者確認接受，紀錄寫在每列 notes。`approved` 仍只在 P13 使用。
