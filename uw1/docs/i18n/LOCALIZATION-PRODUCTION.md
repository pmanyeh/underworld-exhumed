# Phase 9 — Localization Production Foundation

**Native Acceptance Gate P9：PASS（依計劃 v2.3），2026-10-23。停在 P9，等待人工審閱；未開始 P10。**

本階段建立大量翻譯前的 production pipeline：正式 master dataset、全量 inventory 重新核對、
validators、deterministic catalog build、coverage matrix、runtime hit/miss/fallback trace、
translator/QA export 與可重跑的 round-trip。**不是大量翻譯 Phase**：譯文仍是 PoC 的 52 + 1 筆草稿，
沒有新增翻譯、沒有改 gameplay、沒有重寫 BABL，DOS／Borland 依 [Native-only 規則](VALIDATION.md) 為 NOT APPLICABLE。

## 回答 P9 的問題

| 問題 | 目前答案（玩家 GOG 資料，PAK `e8c9848a…`） |
| --- | --- |
| 有多少可翻字串？ | 10,984 entries；扣除 3,852 個空字串與 41 個 index > 511 的 entries，**7,091 個可翻** |
| 覆蓋率？ | draft 52／7,091（0.73%），reviewed 0、approved 0；hard-coded literal 1 筆 draft |
| 可直接翻？ | **5,737**（`direct`） |
| 需 dynamic/template？ | **154**（含 BABL `@`；runtime 目前以英文 BABL 展開顯示） |
| grammar / object naming？ | object-name 413（block 4 的 404＋block 7 的 9）、grammar 片段 548（block 5 形容詞、block 10 貼圖描述、空白開頭／結尾的拼接片段、lad／lass）；另 43 個 block 24 除錯報表字串 |
| hard-coded / bitmap UI？ | bitmap UI 196（角色建立 74、過場 122）；hard-coded literal 候選 2,565（PLAYER_VISIBLE 60＋UNKNOWN 2,505） |
| 實際遊玩會 hit 哪些？ | 16 個 public replay sessions 共 126 個 key、768 次 lookup：hit 12、template 1、miss 753、dynamic 2 |
| 怎麼 build？ | `l10n.py build`：驗證通過才 pack，同輸入同 bytes，manifest 記 SHA-256 |
| 怎麼找 missing/fallback/drift？ | `--i18n-trace` → `l10n.py coverage --trace` 與 `export --runtime --untranslated` |
| 譯者如何更新而不碰 C？ | 編輯 `master.tsv` 或 export／import 工作表；所有規則見 [譯者說明](../../i18n/zh-TW/README.md) |

## 變更與檔案

| 項目 | 檔案 |
| --- | --- |
| Pipeline（inventory／validate／normalize／build／coverage／export／import） | [l10n.py](../../tools/i18n/l10n.py)（standard library） |
| Master dataset／literal namespace | [master.tsv](../../i18n/zh-TW/master.tsv)、[literals.tsv](../../i18n/zh-TW/literals.tsv) |
| 不含文字的 inventory fingerprint | [inventory-baseline.json](../../i18n/inventory-baseline.json) |
| Runtime QA trace、全遊戲 catalog 上限 | [localization.c](../../src/port/i18n/localization.c)／[.h](../../src/port/i18n/localization.h)、[GAMESTRN.C](../../src/ui/GAMESTRN.C) 的 made-block 一行、[main.c](../../src/port/sys/main.c) 的 `--i18n-trace` |
| 既有工具改讀 master | [pack_translations.py](../../tools/i18n/pack_translations.py)、[validate_poc.py](../../tools/i18n/validate_poc.py)、[verify_localized_replay.py](../../tools/i18n/verify_localized_replay.py)、[validate_vertical_slice.py](../../tools/i18n/validate_vertical_slice.py) |
| Tests／CI | 新 [test_l10n.py](../../tools/i18n/test_l10n.py)（17）；[test_runtime.py](../../tools/i18n/test_runtime.py) 16 → 19；layout／scroll tests 改讀 master；`uw1-port.yml` 三平台新增 asset-free localization step |
| 文件 | [i18n README](../../i18n/README.md) workflow、[譯者說明](../../i18n/zh-TW/README.md)、本報告 |

`poc.tsv` 已移除；歷史報告中的連結改指向 master。Exhume 未修改，submodule pin 不變。

## P9.1 Stable identity

正式 identity 維持 resolved `(block, index)`。block 0 必須存成實際的對話／過場 block，`0x7C`／`0x7D`
dynamic blocks、index > 511 與 block > `0x7FFF` 一律拒絕。`source` 只作 drift check、context 與譯者參考，
不是 key。Literal 使用獨立的 `literal:` namespace（P1 定義不變）。

## P9.2 Master dataset

`master.tsv` 欄位依計劃：`block, index, category, source, target_zh_TW, status, context, notes, flags`。
沿用 P1 的 TSV escape 契約，另外拒絕 BOM、CR、非 canonical 排序／flags。

- Status：`untranslated`、`draft`、`reviewed`、`approved`、`blocked`。只有 `draft／reviewed／approved`
  且有譯文的列會 pack，`blocked` 即使有譯文也不 pack；`--min-status` 可做 reviewed／approved-only build。
- Flags：`dynamic-template`、`object-name`、`control-code` 由原文自動推導，`normalize` 自動補上、
  `validate` 要求存在；`grammar-sensitive`、`hardcoded-related`、`showcase`、`needs-context` 為人工。
- **只收有翻譯工作的列**；沒有工作的列由 `normalize` 移除。完整 7,091 個條目的英文工作表由
  `export` 從玩家資料即時產生，只留在 ignored `build/`。如此 repository 不會散布完整英文文字庫，
  但隨翻譯進度，`source` 欄會逐步包含已翻條目的英文——此政策列入人工審閱。
- Parse → write 為 byte-for-byte 相同（test 驗證）；`normalize` 為 idempotent。

## P9.3 Inventory reconciliation

`l10n.py inventory` 重新解碼 STRINGS.PAK 與 CNV.ARK，輸出完整 `inventory.tsv`（本機）與不含文字的
[summary](evidence/p9/inventory.json)，並與 committed fingerprint 逐 block 比對 SHA-256。結果 **SAME**。

| 項目 | 結果 |
| --- | --- |
| Blocks／entries／categories | 122／10,984／13（與 P1 相同，並非沿用假設，是重新解碼） |
| 空字串 | 3,893（含 41 個 unaddressable） |
| index > 511 | 41（block 2 的 9 筆、block 5 的 32 筆），**全部為空字串** |
| Dynamic blocks on disk | 無；`0x7C`／`0x7D` 只在 runtime 由 `make_string` 產生 |
| Block 0 on disk | 無；runtime 依 `CutsceneOrConversationStringBlock` 解析 |
| Conversation | 97 個 CNV headers ↔ 97 個 conversation blocks，全部互相對應，無孤立 block |
| Cutscene blocks | 14（`0xC00`–`0xC03`、`0xC18`–`0xC21`） |
| Category 變更 | block 24 由 P1 的 `unknown_static` 改名 `dungeon_report`（內容為除錯用 dungeon report） |
| Source drift vs master | 0 |

`--write-baseline` 才會接受新的資料作為 baseline；預設在 DRIFT 時回傳失敗並列出變動的 blocks。

## P9.4 Coverage matrix

機器可讀 [coverage.json](evidence/p9/coverage.json)、人可讀 [coverage.md](evidence/p9/coverage.md)，
Runtime 欄來自 16 個 public zh-TW replay sessions 的 QA traces。

| Category | Total | Empty | Unaddr. | Direct | Special | Untransl. | Draft | Rev. | Appr. | Runtime seen | hit | fallback |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| books | 512 | 442 | 0 | 70 | 0 | 70 | 0 | 0 | 0 | 0 | 0 | 0 |
| chargen | 521 | 437 | 9 | 0 | 75 | 75 | 0 | 0 | 0 | 36 | 0 | 36 |
| conversation | 5,189 | 99 | 0 | 4,911 | 179 | 5,041 | 49 | 0 | 0 | 26 | 9 | 17 |
| conversation_common | 512 | 403 | 0 | 99 | 10 | 108 | 1 | 0 | 0 | 5 | 1 | 4 |
| cutscene | 122 | 0 | 0 | 0 | 122 | 122 | 0 | 0 | 0 | 42 | 0 | 42 |
| dungeon_report | 512 | 411 | 0 | 0 | 101 | 101 | 0 | 0 | 0 | 0 | 0 | 0 |
| game | 512 | 102 | 0 | 332 | 78 | 408 | 2 | 0 | 0 | 13 | 1 | 12 |
| object_look | 544 | 409 | 32 | 0 | 103 | 103 | 0 | 0 | 0 | 1 | 0 | 1 |
| object_names | 512 | 108 | 0 | 0 | 404 | 404 | 0 | 0 | 0 | 2 | 0 | 2 |
| spells | 512 | 299 | 0 | 213 | 0 | 213 | 0 | 0 | 0 | 0 | 0 | 0 |
| textures | 512 | 248 | 0 | 0 | 264 | 264 | 0 | 0 | 0 | 0 | 0 | 0 |
| trap_text | 512 | 498 | 0 | 14 | 0 | 14 | 0 | 0 | 0 | 0 | 0 | 0 |
| writings | 512 | 396 | 0 | 98 | 18 | 116 | 0 | 0 | 0 | 0 | 0 | 0 |
| **total** | **10,984** | 3,852 | 41 | 5,737 | 1,354 | 7,039 | 52 | 0 | 0 | 125 | 11 | 114 |

特殊清單：untranslated 7,039、template 154、grammar-sensitive 961、bitmap UI 196、
runtime untranslated 113、runtime fallback 114、runtime dynamic/invalid 1（`0x7D` 玩家姓名）、
source drift 0、invalid target 0、hard-coded literal candidates 2,565。

Handling 是**第一版靜態分類**，依序判定：`unaddressable`、`empty`、`template`（`@`）、
`object-name`（block 4、block 7 ≥ 256 的冠詞語法）、`grammar`（block 5、block 10、空白開頭／結尾、
人工 `grammar-sensitive`）、`bitmap-ui`（block 2 與 cutscene，`CHARGEN.C`／`STATS.C`／`CUTS.C` 以
`string_to_screen` 繪製）、`hardcoded`（人工 flag）、`debug-report`（block 24）、其餘 `direct`。
人工 flags 只能把條目推向特殊處理，不能把特殊條目降為 direct。

## P9.5 Deterministic catalog build

```text
master.tsv ──validate (含 PAK source drift)──▶ pack ──▶ build/i18n/zh-TW.uwloc + manifest
```

- 排序固定為 `(block,index)`；duplicate key、非法 UTF-8／surrogate／NUL、malformed record、source drift
  任何一項都拒絕 build（回傳非 0，不寫 catalog）。
- 從 master build 的 catalog 與 P8 **完全相同**：`fd133b9a159b10e3…dff98`，52 entries、8,990 bytes
  （[catalog.json](evidence/p9/catalog.json)）。重複 build 結果相同。
- Catalog 上限由 4,096 筆／4 MiB 放寬為 32,768 筆／16 MiB，足以容納全遊戲；格式 `UW1LOC1` 不變，
  runtime loader 與 packer 共用相同上限，並以 32,768 筆的 catalog 實測載入與查找。

## P9.6 Validators

| 類別 | 檢查 |
| --- | --- |
| Encoding | strict UTF-8（含行號）、無 BOM、LF only、無 NUL、無 surrogate、無 U+FEFF、除 LF 外無 control characters、譯文 ≤ 16,384 bytes |
| Key | duplicate、非 canonical decimal、block 0／`0x7C`／`0x7D`／> `0x7FFF`、index > 511、key 不存在於當前 PAK、category 不符 |
| Source | 與當前解碼原文逐字比較；drift 列入 report 並阻擋 build，不會默默接受；原文須可 CP437 表示且 ≤ 511 bytes |
| Controls | `\p`、`\P`、`\m`、`\0`–`\6` 種類與順序相同；明示換行數相同；結尾的 `\m`／`\p` 必須仍在結尾；譯文不可自行寫出 `[MORE]` |
| Templates | 完整遞迴 BABL 語法（`@SS1`、`@GI2C3`、`@PS-1`、`@@` 等）次數與寫法相同；未知語法 fail closed |
| Status／flags | 狀態與譯文一致、未知 flag、derived flags 缺漏、沒有工作的列 |
| Literals | key 仍存在於 literal scan、仍為 PLAYER_VISIBLE、source 相同 |

全部規則都有 asset-free unit test；P9 只做 inventory／validation／classification，**未改 BABL**。
`l10n.py validate` 在沒有遊戲資料時只做 row-level 檢查（CI 使用），有 `--pak` 時加上 drift 與 key 存在檢查。

## P9.7 Runtime missing / fallback trace

`--i18n-trace FILE` 在 game thread 為每次 resolved lookup 寫一行 JSONL（任何 locale；英文模式全部記為 miss）：

| event | 意義 |
| --- | --- |
| `hit` | catalog 命中且 HD 路徑顯示譯文 |
| `template` | 已翻譯但含 `@`，目前仍以英文 BABL 展開顯示（P11） |
| `miss` | 未翻譯 |
| `drift` | catalog 有此 key，但 runtime 原文不同，回到英文 |
| `invalid` | 未解析的 block 0、超出範圍的 key |
| `dynamic` | made block（`0x7C` 對話組字、`0x7D` 玩家姓名），繞過 static lookup |

每行 flush，避免 parked exit 遺失。16 個 sessions 總計只有 768 行（約 55 KB），所以不在 runtime 做 aggregate；
`coverage`／`export` 依 key 彙總次數。既有 `--i18n-log`（每 key 一次的 hit log，P8 工具使用）格式不變。
lookup 結果、fallback pointer、display provenance 與 string ring lifetime 均不變。
各 session 計數見 [trace-summary.json](evidence/p9/trace-summary.json)。

## P9.8 Context / QA export

`l10n.py export` 產生譯者工作表：`key, block, index, category, handling, flags, status, source, target_zh_TW,
context, notes, auto_context, runtime_hits, runtime_events, previous_source, next_source`。
`auto_context` 由 CNV header 與 block 7 的 NPC 名稱推導（例如 `conversation slot 67 (Bragit)`），
前後條目提供鄰近語境；可依 category／block／handling／未翻譯／runtime 篩選。
本機全量工作表為 7,091 列，runtime 篩選為 125 列。沒有 screenshot 連結或 CAT 功能，避免 P9 膨脹。

## P9.9 Round-trip workflow

```text
inventory → export → 編輯 → import → validate → build → Native 遊玩 (--i18n-trace) → coverage → 再編輯
```

以玩家資料與暫存 master 實測（[roundtrip.json](evidence/p9/roundtrip.json)）：匯出 block 1 未翻譯列 → 譯一筆 → import
→ validate PASS → 兩次 build 相同 SHA；遺失換行的譯文與 stale export（原文不同）都被 import 拒絕且不改 master；
把該列改回 untranslated 再 import，master 與原檔 byte-for-byte 相同，catalog 回到 P8 SHA。

## P9.10 P8 compatibility / regression

| Gate | 結果 |
| --- | --- |
| PoC migration | 53／53 列的 source、target、note、role、key 全部保留（[migration.json](evidence/p9/migration.json)）；PoC 子集 catalog 與 P8 SHA 相同 |
| Native build／port-check | Windows CLANG64 compile/link PASS；port-check 0 errors／0 warnings |
| English compatibility | 16 public sessions，與 committed goldens 全部相同（[log](evidence/p9/replay-en.log)） |
| zh-TW lookup replay | 16 sessions 與 goldens 相同；12 個 distinct hits 逐字符合 master；同時產生 QA traces |
| P8 player recordings | walk 249、dialogue 306、restore 122、English 300、resize 478 checkpoints，各兩次 replay，全部 canonical sections／saves 相同 |
| Text／font／cursor／scroll | inventory 22、l10n 17、layout 29、runtime 19、FreeType（ASan/UBSan）、no-FreeType、HD cursor、MOUSE.C、scroll flow／render／window、10,984-string native probe：全部 PASS |
| Native matrix | PASS（見下方） |
| Repository check | PASS |

P8 recordings 本輪以一般 build replay，沒有重做 sanitizer replay；P9 的 C 變更只有 trace 寫檔與 catalog 上限，
已以 `test_runtime.py` 實際 C module 測試涵蓋。Phase 工作提交 `f28292c` 的
[CI run 39821060821](https://github.com/pmanyeh/underworld-exhumed/actions/runs/39821060821)：Linux、macOS、
Windows 的 Native build／port-check、新增的 asset-free localization step（master validate、l10n 17、
inventory 22、runtime 19）與既有 font／cursor／scroll tests，以及 Web compile/link，四個 jobs 全部 success。
同一次 push 的 repocheck 失敗，原因只是本報告尚未提交（README 連結）；由本報告提交修正。
完整遊戲流程只在 Windows 實測。

Native matrix（[native-matrix.json](evidence/p9/native-matrix.json)）與 P8 數量相同、全部 PASS：
graphics fuzz 34 routines／5,680 cases、0 differences；enhancements 120/120；settings 72/72；
movement 15/15；blackbox 22/22；map notes 4/4。

## Acceptance Gate P9

| 項目 | 結果 |
| --- | --- |
| Full inventory refresh | PASS — 10,984／122／13，fingerprint SAME |
| Deterministic catalog build | PASS — 重複 build 相同；等於 P8 catalog |
| Master dataset parse/write | PASS — byte-for-byte round trip、normalize idempotent |
| Duplicate／source-drift／UTF-8／control validators | PASS — 全部有正反向 tests；實際資料 0 problems |
| Coverage matrix | PASS — JSON + Markdown，含 runtime 欄 |
| Runtime hit/miss/fallback report | PASS — 六種 events，16 sessions traces 彙總 |
| PoC translations migrate without loss | PASS — 53／53，catalog SHA 不變 |
| P8 regression | PASS |
| English compatibility | PASS |
| No new gameplay behavior | PASS — 只有 opt-in trace 與 catalog 上限；replays／saves 相同 |
| Docs | PASS — workflow、譯者說明、本報告 |
| DOS exact-match | NOT APPLICABLE — Native-only |

## 人工審閱項目（P10 前）

1. **Dataset schema**：欄位、status、flags 是否足夠；`blocked` 有譯文時不 pack 的規則。
2. **English source 的散布政策**：master 隨翻譯進度會逐步含已翻條目的英文原文（PoC 已如此）。
   替代方案是只存 source SHA-256，譯者完全依賴本機 export。需決定後才大量翻譯。
3. **Coverage classification**：spells 目前算 direct，但部分可能出現在 bitmap UI；block 1 的部分
   字串實際由 `string_to_screen` 顯示（例如 main menu）；block 24 是否列入正式翻譯範圍。
4. **Workflow ergonomics**：TSV 工作表是否適合譯者使用的工具（試算表或 CAT）；import 要求原樣欄位標頭。
5. **Special-string inventory**：154 個 template、413 個 object-name、548 個 grammar 片段、
   196 個 bitmap UI、43 個 debug report、2,565 個 hard-coded literal 候選的處理分期（P11／P12）。

## Baseline / Upstream sync gate

| 項目 | Phase 開始 | 結案 fetch 後 |
| --- | --- | --- |
| Underworld main / origin/main / upstream/main | `e5deadd6945b…` | 相同，main vs upstream `0/0` |
| Underworld feature | `ae8a69122f49…`（P8 結案） | Phase 工作 `f28292c8ca4a…` 已 push，vs origin `0/0` |
| Exhume upstream/master | `aa2353a2442c…` | 相同，沒有新提交 |
| Exhume main / origin/main | `c54e00e9176c…` | 相同，vs upstream `0/4` |
| Exhume feature / submodule pin | `89278495bb0b…` | 相同，vs upstream `1/11` |

**UPSTREAM_SYNC = NO NEW CHANGES；既有 Exhume backlog 仍 DEFERRED。** 沒有 merge／rebase、
沒有更新 fork main、沒有改 submodule pin；Exhume 本階段未修改。詳見 [sync.json](evidence/p9/sync.json)。
工作區既有、與本階段無關的 `.gitignore` 修改及未追蹤的 `record_showcase.py`（P8.5 showcase 工作）
保持原狀，未納入提交。結案提交加上 annotated `zh-tw-native-p9` tag，與 feature 一同 push（不 force）。

本機重跑用的 helper（含本機路徑）保存為 evidence 中的 `p9-*.py.txt`；英文工作表、完整 inventory、
traces、recordings 與 catalog 只留在 ignored `build/`。

**STOPPED HERE. Phase 10 was not started.**
