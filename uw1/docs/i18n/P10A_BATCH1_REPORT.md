# P10A Batch 1 — Core / System / Runtime High-Priority Direct Strings

**結論：`P10A_BATCH1_PASS_WITH_FINDINGS`**（2026-10-23）。157 筆 direct 字串譯為 `draft`
（人工審閱 F3 後，另補 22 筆，共 179 筆，見 [F3 處理](#f3-處理人工決定後)），
全部經 import／validate／authority lint／deterministic build／Native QA；沒有 Authority 違規、沒有 source drift。
另有一項需要人工決定的 regression 設計問題（[F1](#i-findings--blockers)）與數項交給 P11／P12 的發現。
**本批完成後停止**：沒有開始 Batch 2、P10B／P10C、P11、P12 或 M0–M2，沒有任何譯文標為 `approved`。

Evidence：[evidence/p10a-b1](evidence/p10a-b1/)。本報告與 evidence 只以 `block:index` 指稱字串，不收錄英文原文；
含英文的工作表、inventory 與 scroll log 只留在 ignored `build/i18n/p10a-b1/`。

## A. Baseline

| 項目 | 值 |
| --- | --- |
| Branch | `feature/zh-tw-native`（與 `origin/feature/zh-tw-native` 同步） |
| Commit | `97424d432ed5270f1e951a01d227d7365ff11495` |
| Tag | `zh-tw-native-authority-ready`（`git describe --exact-match` 成立） |
| Working tree | clean，沒有本任務以外的修改 |

## B. Entry Gate

| Gate | 結果 |
| --- | --- |
| Authority validator | PASS — 980 terms、969 project-selected、11 approved、**review-needed 0**（[authority-validator.json](evidence/p10a-b1/authority-validator.json)） |
| Inventory | 10,984 entries／122 blocks／13 categories，fingerprint **SAME**（[gate-inventory.json](evidence/p10a-b1/gate-inventory.json)） |
| Master validate（含 PAK） | PASS — 52 rows、0 problems、**0 drift** |
| Baseline catalog | 52 entries、`fd133b9a…dff98` = P8／P9 catalog（[gate-catalog.json](evidence/p10a-b1/gate-catalog.json)） |
| `test_l10n.py` | 31／31 PASS |
| Native baseline build | PASS — 0 errors；1 個既有的 Exhume `mt32roms.c` unused-variable warning（P9 也有）（[log](evidence/p10a-b1/gate-native-build.log)） |

環境註記：工作區已改名為含空白的 `D:\git\Ultima Underworld`，pkg-config 的 `-I` 路徑因此被斷開，
第一次 build 找不到 FreeType／mt32emu header。這不是程式或資料 drift：既有的 junction
`D:\git\uw1-native-poc` 指向同一工作區（也是 P9 helper 使用的路徑），經由它 build 即通過。

## C. Batch Selection

**Direct 總數 5,444**（未翻 5,398）。本批選 **157 筆**，分三個語意完整的 bundle：

| Bundle | 筆數 | 內容 |
| --- | ---: | --- |
| Core system messages（block 1） | 125 | 由 `game_sprint` 整句送到 scroll 的系統／遊玩訊息：鑰匙與開鎖、撿拾／放下、睡眠與紮營、神龕祈禱語、釣魚、燈油與火把、門釘、烹飪、飲食、施法失敗、存讀檔、Look／Talk／Use 的「沒有對象」回應、世界事件 |
| Conversation system lines（block 7） | 2 | `7:0`、`7:1`：無法對話、沒有回應 |
| NPC bundle：Brawnclan（slot 88，block 3672） | 30 | 完整對話，含問候、三個身分選項、主選單與交易（barter）選項；runtime 已觀察到 16 筆 |

- **Runtime-seen**：157 筆中 23 筆出現在既有 QA traces（block 1 的 7 筆＋Brawnclan 16 筆）。
- **控制碼**：4 筆（`1:163`–`1:165` 的色碼、`1:168` 存檔提示）。
- **為何屬於 P10A**：都是 `direct`、player-facing、整句顯示；涵蓋 Tier 1 的 core system、runtime-seen、
  高頻與語境明確的 NPC 對話。Bragit 已在 PoC 完成，Brawnclan 是 traces 中唯一未翻的 NPC。
- **Authority coverage**：本批共使用 53 個 Authority 術語（52 project-selected、1 approved：mantra→祈禱語）。

### Block 1 consumer audit（選擇前）

`direct` 是第一版靜態分類，只以首尾空白判斷拼接片段。實際逐一追蹤 block 1 的程式用法
（`game_sprint` 字面與計算索引、`game_strings_3`、`print_path_to`、`str_cat`）後，block 1 的 326 筆
未翻 direct 分成（[block1-consumer-audit.json](evidence/p10a-b1/block1-consumer-audit.json)）：

| 結果 | 筆數 | 處理 |
| --- | ---: | --- |
| 整句、單獨送到 scroll | 125 | 本批翻譯 |
| 由多次 `scroll_print` 組成一句（方向、距離、時段、飢餓／疲勞、中毒程度、修理難度、美德聖物、序數、版本字首、存讀檔的進度點、句尾） | 172 | **不翻**；加 `grammar-sensitive`，notes 記程式位置 → P11 |
| Native C 中找不到任何讀取者 | 29 | 先不動；人工決定後 7 筆移到 debug scope、22 筆翻譯（見 F3） |

HD scroll 以每次 `scroll_print` 的指標各自查譯文，所以只翻組句中的片段會產生中英夾雜的句子，
屬於 P11 grammar 機制。spells 的 `6:384`–`6:404`（21 筆，作用中法術名）同理：`RUNES.C` 先印法術名，
再印 `1:137`–`1:139` 的 grammar 片段，也一併標記。共 **193 筆 direct → grammar**；
人工 flag 只會把條目推向特殊處理，這是 P9 文件化的機制，沒有把任何特殊條目改成 direct。

## D. Translation

| 項目 | 數量 |
| --- | ---: |
| Translated（本批） | 157＋22（F3 處理）＝ **179** |
| draft | 179 |
| reviewed | 0（repo 沒有可驗證的 review workflow，故不提升） |
| approved | 0 |
| blocked | 0 |
| needs-context | 0 |
| P11 finding（`grammar-sensitive`，未翻） | 193 |
| 移到 debug scope（無讀取者、已被取代或開發用） | 7 |

做法：從玩家資料 export 新的 workbook（只留本機）→ 填入譯文／context／notes → `l10n.py import`
（逐列比對英文、寫入 `source_hash`）。對話不加外層「」，與 P8 Bragit 譯文及遊戲原本的顯示一致；
刪節號用「……」、句內用全形標點；色碼、換行與 `1:168` 的前導空白原樣保留。
Brawnclan 的山岳族口音以較直率的口吻表現，交易選項依玩家／NPC 身分區分語氣。

Authority 使用重點：深淵、貪金王、不列顛尼亞、山岳族（the Folk）、放逐者、冒險者、腐蟲、破帷者、
守護之符、祈禱語（approved）、法術環級、以物易物／交易、鐵柵門、長竹竿、提燈、月門、月之石等。
`1:272` 的 princess 是 `TERR_PRINCESS` 牆面貼圖，依物件術語「鎖在磚牆上的少女」譯為少女。

**Authority decisions required（本批）：無。** 沒有自行發明譯名，也沒有 canonical 不適用的情形。
既有 PoC 列的不一致另列 F6。

## E. Validation

| 項目 | 結果 |
| --- | --- |
| `l10n.py validate`（含 PAK） | PASS — 424 rows、0 problems、0 drift（[validate.json](evidence/p10a-b1/validate.json)） |
| `l10n.py validate`（無資料，CI 模式） | PASS |
| Authority lint（本批 blocks 1／7／3672） | **0 errors、0 warnings**、16 info（[authority-lint-batch.json](evidence/p10a-b1/authority-lint-batch.json)） |
| Authority lint（整份 master） | 0 errors、5 warnings、30 info；5 個 warning 全是批次前就存在的 PoC 列（[before](evidence/p10a-b1/authority-lint-before.json)／[after](evidence/p10a-b1/authority-lint-all.json)） |
| Source hash | import 時逐列由目前資料計算；validate 重算 424 列皆相符 |
| Stale workbook | 竄改本批 workbook 一列英文後 import：整份拒絕（`stale export`），master SHA-256 不變 |
| Catalog determinism | 兩次 build 相同：231 entries、25,967 bytes、`fb2f31fc5039174fc42812e2004f662b2ccc8fd994223807e5089bf73081ff3d`（[catalog.json](evidence/p10a-b1/catalog.json)）。F3 處理前的 157 筆版本為 209 entries、`94c8d925…33bc`，本報告的 Native QA 截圖與 scroll log 以該版本產生；新增的 22 筆在這些場景都不會出現 |
| UTF-8／控制碼 | validator 全數通過；runtime 確認 `\6` 綠色、`\4`／`\0` 色碼正常 |
| English source 不入庫 | 掃描 master 與 evidence：沒有任何完整英文遊戲句；僅 context 標籤含 Authority 已收錄的法術名 |

16 個 info 都是一般字詞而非詞表詞義（例如 No luck 不是 UI 的 No、at rest 不是休息指令、Your sleep 是名詞），
已逐筆檢視，不需修改。

新增 **`l10n.py authority`**（P10 流程中的 authority lint 原本沒有工具）：讀取 master 旁的
`authority/GLOSSARY.tsv`，不建立任何新詞表；路徑由 master 所在的 locale 推得，沒有寫死 zh-TW。
規則見 [README](../../i18n/README.md)；5 個 asset-free tests 加入 `test_l10n.py`（CI 已執行此檔）。

## F. Native QA

**Public replays（HD scroll、zh-TW、scroll log）**：overpay（Brawnclan 全流程＋交易）、talk、savecursor、
tooheavy、items。本批在 runtime 可達的 **23 個 key 全數 `hit`**，每一筆的完整譯文都出現在實際顯示的文字中；
**沒有任何本批 key 退回英文**；沒有多出的 MORE 分頁（[hd-qa-scroll.json](evidence/p10a-b1/hd-qa-scroll.json)）。
這些 HD run 只取 scroll log；它們與 DOS goldens 的畫面比對本來就不適用（HD scroll 會重畫紙面）。

**實機視窗（real-time input script，與 P8 dialogue 相同的操作）**：

| 畫面 | 結果 |
| --- | --- |
| Talk 空處（`1:156`）[截圖](evidence/p10a-b1/hd-talk-1-156.png) | 三行「你無法與那個對話。」，無截斷、標點正確 |
| 存檔成功（`1:165`）[截圖](evidence/p10a-b1/hd-save-1-165.png) | 「儲存遊戲成功。」以 `\6` 綠色顯示 |
| 存檔說明輸入中（`1:168`）[本批](evidence/p10a-b1/hd-save-prompt-batch1.png)／[P8 catalog](evidence/p10a-b1/hd-save-prompt-p8-catalog.png) | 輸入時提示文字不顯示——**P8 catalog 下相同**，是既有的 HD 行為（見 F4）；輸入框起點隨較短的中文提示左移 |
| Bragit 對話 | 與 P8 相同；`@SS1` 模板句仍為英文（既知 P11） |

| 檢查 | 結果 |
| --- | --- |
| Untranslated direct（本批） | 0 |
| Wrong context／speaker | 發現 1：`3672:20` 實際是玩家選項，原 context 誤標 NPC；譯文兩者皆適用，已更正 context（catalog 不變）。`3672:14` 未在 runtime 出現，context 註明未驗證 |
| Choice meaning | 7 組選項（身分、主選單、交易、索取）的語意與後續 NPC 回應一致 |
| Line wrap／clipping | 無；CJK 逐字換行，最長的 `3672:7` 在對話框內不需分頁 |
| Punctuation／control tokens | 正確 |
| English fallback（同畫面的其他字串） | 交易估價句 `7:3`–`7:16`（grammar，P11）、Bragit 模板句（P11）——都不是本批 |
| Source drift／terminology | 0／0 |

## G. Coverage（before → after）

同一組 19 個 public zh-TW sessions 的 traces（before 為批次前存檔，after 以新 catalog 重跑）。
[before](evidence/p10a-b1/coverage-before.md)／[after](evidence/p10a-b1/coverage-after.md)／[compare](evidence/p10a-b1/coverage-compare.json)。

| 指標 | Before | After |
| --- | ---: | ---: |
| Production scope | 6,990 | **6,983**（7 筆移到 debug） |
| Debug scope | 101 | 108 |
| Direct total | 5,444 | **5,244**（193 筆改為 grammar、7 筆移出） |
| Direct translated（draft 以上） | 46 | **225** |
| Direct untranslated | 5,398 | **5,019**（= 5,398 − 179 譯 − 193 改分類 − 7 移出） |
| Grammar handling | 692 | 885 |
| draft／reviewed／approved | 52／0／0 | 231／0／0 |
| blocked | 0 | 0 |
| needs-context | 0 | 0 |
| Production completion | 52（0.74%，分母 6,990） | **231（3.31%，分母 6,983）** |
| Runtime hit／fallback（19 sessions） | 11／114 | **34／91** |
| Authority violations（error＋warning，整份） | 5（PoC） | 5（PoC，無新增） |
| Authority violations（本批） | — | 0 |
| Source-drift violations | 0 | 0 |

English fallback 不計為 translated；coverage 只計 `draft`／`reviewed`／`approved` 且有譯文的列。

## Regression

| 項目 | 結果 |
| --- | --- |
| CI-equivalent localization step（無遊戲資料） | PASS — validate、Authority、l10n 36（1 筆需資料者略過）、inventory 22、runtime 19（[log](evidence/p10a-b1/ci-equivalent.log)） |
| `test_l10n.py`（含玩家資料） | 36／36 PASS，含 PoC catalog = P8 SHA |
| English compatibility | 16 public sessions 與 DOS goldens 全部相同（[log](evidence/p10a-b1/replay-en.log)） |
| zh-TW lookup replay | PASS，每個 hit 符合 master（[replay-zh-TW.json](evidence/p10a-b1/replay-zh-TW.json)） |
| P8 recordings：english、resize | PASS（各兩次） |
| P8 recordings：walk、dialogue、restore（HD） | 以本批 catalog **不相同**——原因與隔離證明見 **F1** |
| 同上，以 P8 catalog | PASS（walk 250、dialogue 307、restore 123 checkpoints，各兩次，saves 相同） |
| 同上，以本批 catalog 去掉這些錄影顯示的 5 筆 | PASS（三者全部相同；F3 處理後的 231 筆 catalog 去掉同 5 筆再跑一次，仍全部相同） |
| Text／font／cursor／scroll（含 ASan／UBSan） | inventory、l10n、layout、runtime、font、no-FreeType、cursor、mouse、flow、render、window：PASS（[text-tests.json](evidence/p10a-b1/text-tests.json)） |
| Native probe（10,984 strings＋runtime hook） | 第一次執行失敗（`text-tests.json` 記為 1，原因見 F7）；修正 `verify_native.py` 後重跑，本批與 P8 catalog 皆 PASS（[native-probe.json](evidence/p10a-b1/native-probe.json)） |
| Native build | PASS；本批未改任何 C 程式 |
| Native matrix（fuzz／enhancements／settings／movement／blackbox／map notes） | 未重跑：本批沒有改 C 或遊戲邏輯，這些檢查與翻譯資料無關 |
| `git diff --check`、repocheck | PASS |

DOS／Borland 依計畫不是 P10 blocker，未執行。

## H. Files Changed

| 檔案 | 用途 |
| --- | --- |
| `uw1/i18n/zh-TW/master.tsv` | +179 draft 譯文；+193 列 `grammar-sensitive` 稽核標記（無譯文）；52 → 424 rows，僅 `source_hash`，不含英文 |
| `uw1/tools/i18n/l10n.py` | 新增 `authority` 子指令（authority lint，locale-neutral）；`UNREAD_KEYS` 把 7 筆無讀取者的 block 1 字串歸入 debug scope（F3） |
| `uw1/tools/i18n/test_l10n.py` | authority lint 的 5 個 asset-free tests；scope 測試涵蓋 `UNREAD_KEYS` 與 hash 比對 |
| `uw1/i18n/zh-TW/README.md` | debug scope 的定義加入這 7 筆 |
| `uw1/tools/i18n/verify_native.py` | runtime hook 的預期 key 集合允許已翻譯的 `1:3`（F7） |
| `uw1/i18n/README.md` | workflow 加入 `authority`；連到本報告 |
| `uw1/docs/i18n/P10A_BATCH1_REPORT.md` | 本報告 |
| `uw1/docs/i18n/evidence/p10a-b1/` | 計數／key-only 的 JSON、logs、4 張視窗截圖、本機 helper 副本（`*.py.txt`） |

沒有修改 runtime C、Exhume、submodule pin、goldens、gameplay、save format、Authority 或 style guide。

## I. Findings / Blockers

**F1 — P8 HD recordings 與 catalog 內容耦合（需要人工決定 regression 策略）。**
HD 模式下 `SCROLLIO.C` 把 HD 排版後的游標寫回遊戲記憶體（`scroll->cur_x`、`cur_y`、`start_line`）。
walk／dialogue／restore 錄製時，`1:156`、`1:162`、`1:165`、`1:168`、`1:253` 還是英文；翻成中文後寬度不同，
這些 presentation 欄位便與錄影不同，而 verifier 比對全部 canonical sections。
隔離證明：同一 executable 用 P8 catalog 全部 PASS；用本批 catalog 只去掉這 5 筆，三份錄影也全部 PASS；
沒有多出的 MORE 分頁，輸入時序不受影響。所以這不是翻譯或程式錯誤，但**之後每一批只要翻到這些錄影會顯示的字串
就會重現**。可選方案：(a) 把 P8 HD 錄影定位為「HD code path regression」，固定用 P8 catalog 重播（本批採此做法）；
(b) 從 canonical 比對排除 scroll 游標欄位；(c) 每批重新錄製。另需在 P13 確認這些欄位是否進入存檔（save compatibility）。

**F2 — 193 筆 direct 實為組句片段（→ P11）。** 見 C 節。另外 `7:3`–`7:16` 的交易估價句已是 grammar，
在 Brawnclan 交易流程中仍以英文出現。

**F3 — block 1 有 29 筆在 Native C 找不到讀取者**（`1:0`、`1:1`、`1:7`、`1:8`、`1:13`、`1:32`、`1:33`、`1:97`、
`1:98`、`1:130`、`1:146`、`1:194`–`1:209`、`1:215`、`1:281`）。其中有開發者字串與 UW1 改用 hard-coded 文字的提示。

#### F3 處理（人工決定後）

依人工審閱採用的分類（[block1-consumer-audit.json](evidence/p10a-b1/block1-consumer-audit.json)）：

| 處理 | Keys | 理由 |
| --- | --- | --- |
| 移到 **debug scope**（不計入完成率，仍留在 inventory） | `1:0`、`1:1`、`1:281` | 開發者字串、試玩版專用 |
| 同上 | `1:7`、`1:8`、`1:130` | UW1 改用 `USEITEMS.C` `UseThing` 寫死的「Use ‹物品› on what?」 |
| 同上 | `1:98` | 實際使用的是 `1:244`（`INTERACT.C` 的 yes/no 問句，grammar） |
| **翻譯為 draft**，留在 production | `1:13`、`1:32`、`1:33`、`1:97`、`1:146`、`1:194`–`1:209`、`1:215`（22 筆） | 程式找不到讀取者，但可能由關卡資料觸發；翻譯沒有壞處 |

移出規則沿用 P9 對 block 24 的定義（遊戲程式從不讀取 → debug），以 `l10n.py` 的 `UNREAD_KEYS` 明列，
並記錄稽核時的 `source_hash`：資料不同時自動回到 production，不會誤排除其他版本的字串。
人工 flag 機制沒有被用來做這件事，也沒有新增 scope 種類。22 筆譯文使用 Authority 的地下世界、真理之鑰、陷阱、
按鈕、開關、拉桿、魔杖；`1:199` 原文把 lever 拼成 level，依語意譯為拉桿。
這 22 筆在現有的遊玩記錄中都不會出現，所以沒有 runtime 截圖；validate、authority lint（0／0）、
deterministic build、zh-TW lookup replay、native probe 與 P8 隔離重播都已用新 catalog 重跑並通過。

**F4 — 存檔說明輸入時提示文字不顯示（→ P12）。** 輸入框暫停 HD scroll；P8 catalog 下相同，是既有行為。

**F5 — `--min-status reviewed` 在沒有任何 reviewed 列時 build 失敗**（catalog 至少要 1 筆）。既有行為，僅記錄。

**F6 — PoC Bragit 列與 Authority 不一致（Authority decision／P10B）。** 5 個 warning：Cabirus×2（凱布勒斯）、
Mountainmen（山岳族）、guard（守衛）、Gray Goblins（灰色惡鬼族）；另 Goblins 用異名「哥布林」（info）、
pole 用「長竿」（info）。這些列被 `test_poc_catalog_equals_p8` 固定為 P8 catalog，修改會破壞該基準，所以本批不改。

**F7 — `verify_native.py` 的預期 hook keys 原本寫死 PoC catalog。** probe 以 `1:3` 轉動 string ring；
`1:3` 翻譯後也會 hit。改為必須包含原三筆、且只能另含 `1:3`。

**F8 — 工作區路徑含空白會使 Native build 失敗**（pkg-config 的 include flags）。目前經由 junction 建置；不影響 repo。

## J. Recommendation

**`P10A_BATCH1_PASS_WITH_FINDINGS`**

流程（Authority → workbook → import → validate → authority lint → build → Native QA → trace → coverage）
已完整走過且可重複，本批沒有 Authority、drift 或 runtime 退回英文的問題。F3 已依人工決定處理。
請在擴大 P10 前先決定 F1 的 regression 策略，並審閱 179 筆 `draft`。

建立本機 commit 與 checkpoint tag `zh-tw-native-p10a-batch1`（未 push；`zh-tw-native-p10` 保留給整個 P10 驗收）。

**STOPPED HERE。P10A Batch 2、P10B、P10C、P11、P12 與 M0–M2 都沒有開始。**
