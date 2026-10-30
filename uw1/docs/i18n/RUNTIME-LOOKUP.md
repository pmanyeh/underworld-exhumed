# Phase 2 — Runtime Localization Lookup

## Result

**Native P2 PASS，依計劃 v2.2，2026-10-21。**

已在真實 Native 遊戲的 `get_string()` 路徑取得正確 UTF-8 translation，
並驗證 locale off、fallback、resolved block、dynamic bypass 與字串生命週期。
採本階段允許的 logging 驗證方式：畫面與 mutable game strings 仍走原本英文路徑。
尚未新增 layout、FreeType、HD overlay 或中文 renderer。

## Baseline / Platform

| 項目 | Phase 開始時的版本 |
| --- | --- |
| Underworld upstream/main | `e5deadd6945b492636aadccc14f52caa908b4186` |
| Underworld feature | `8b7f4d618fb691cc430bb552f10b64941c0baeac`（`zh-tw-native-p1`） |
| Exhume upstream origin/master | `c54e00e9176c14d0913b5806318a30f85a8f4636` |
| Exhume feature/current pin | `a34316463c230bafa3d4f0aa3b894a574e5fc738`，detached，無新 feature branch |
| OS / compiler | Windows、MSYS2 CLANG64、Clang 23.1.3 |
| SDL3 / sound | SDL3 3.4.18、libmt32emu 2.8.3；沿用 Phase 0 |
| Python / FreeType | Python 3.12.10；FreeType 本階段未使用 |

開始前 working tree clean，fetch origin / upstream / Exhume origin 後均沒有新提交。
Exhume 的 7 個既有 tooling/profile backlog commits 維持未合併，不變動 submodule pin。

## Changes / Ownership

- [localization.c](../../../exhume/runtime/port/i18n/localization.c)、[localization.h](../../../exhume/runtime/port/i18n/localization.h)：
  UW1-owned catalog、read-only UTF-8 lookup、source drift fallback、diagnostic observer。
- [GAMESTRN.C](../../src/ui/GAMESTRN.C)：僅新增 `#ifndef __TURBOC__` 的 include 與 Native observation hook。
  DOS branch 保留原本語句；`read_string()` decoder、八個 buffers、made blocks 均未重寫。
- [main.c](../../src/port/sys/main.c)：三個明示 CLI options 與 game-thread initialization。
- [pack_translations.py](../../tools/i18n/pack_translations.py)：從已審查 UTF-8 TSV 生成 deterministic catalog。
- [test_runtime.py](../../tools/i18n/test_runtime.py)：真實 C library 的 lookup / loader / trace tests。
- [native_probe.c](../../tools/i18n/native_probe.c)、[verify_native.py](../../tools/i18n/verify_native.py)：
  擴充實際 `get_string()`、dynamic pointer、ring 與 UTF-8 lifetime 的整合驗證。
- [verify_localized_replay.py](../../tools/i18n/verify_localized_replay.py)：沿用 upstream verifier，加入 locale options，核對每個 trace hit。
- [verify_dos_branch.py](../../tools/i18n/verify_dos_branch.py)：保護 DOS branch tokens；不是 Borland exact-match。
- [README](../../i18n/README.md)：資源契約、生命週期、操作與可移植命令。

沒有修改 Exhume、SDL backend、generated file、replay inputs、goldens、存檔格式或 gameplay。
沒有添加 SDL type 至 game-facing API。`.uwloc`、EXE、DLL、完整 trace/state dumps 只放 ignored build 目錄。
本輪未增加／更動 Phase 1 的 53 筆翻譯草稿。

## Lookup / Legacy Boundary

API：

```c
int port_i18n_init(const char *locale, const char *catalog_path);
int port_i18n_enabled(void);
const char *port_i18n_lookup(unsigned block, unsigned index, const char *fallback);
void port_i18n_shutdown(void);
```

初始化以第二個參數明示 catalog 路徑；相較計劃的建議介面，不依賴隱藏環境變數或工作目錄搜尋。
預設 `en` 不讀檔；`zh-TW` 先驗證整份 catalog，再啟用。載入失敗會保留停用狀態，
native wrapper 輸出一次診斷後繼續英文遊戲。任何 miss、source drift、無效 key 或 NULL fallback
皆回傳原始 fallback pointer，不進行複製／修改。

`get_string()` 先走原版 made-block 查找，命中立即返回；未命中才解析 block 0，
讀取原版字串，再呼叫 observer。observer 使用正式 lookup 取得 UTF-8，必要時寫入 JSONL；
legacy caller 得到原本可修改的英文 ring pointer。高 resolved block 不重新塞入 16-bit packed ID。

這個邊界是刻意的：原版字串會被 `fix_name_string()` 修改，而且有多處固定容量的拼接／對話 buffers。
直接把 immutable UTF-8 pointer 回傳至所有舊 caller，會引入寫入常駐譯文與多位元組長度風險。
本階段用計劃允許的 logging path 證明真正的 runtime lookup；後續 presentation integration
再消費 UTF-8，不能把本階段測試當成已完成中文顯示或所有 legacy caller 的 UTF-8 改造。

Catalog 為 52 個 static entries、8,990 bytes；literal `[MORE]` 留在獨立 namespace，未加入 runtime pack。
其原文 bytes 用 CP437 作 source identity，target 保留 UTF-8；完整格式與 bounds 見 README。
loader 拒絕截斷、錯誤 magic、duplicate / unsorted keys、dynamic / unresolved keys、過大長度、
尾端資料、NUL 與非法 UTF-8。Packer 同時驗證 BABL markers、控制碼順序與明示換行。

## Thread / Lifetime

init、shutdown 與 trace 完全由單一 game thread 擁有；lookup 不寫 table，
沒有 timer、audio 或 presentation callback 存取這些 mutable state，也沒有 live reload。
譯文直到 shutdown 都保持有效，獨立於原版八個 512-byte slots；miss 仍遵守 caller 原有生命週期。

目前 upstream 的 `plat_run()` 在關窗時可能返回而不 join game thread，
因此沒有在 main thread 的 `plat_run()` 後 free catalog。正常 game wrapper 返回才 shutdown；
`plat_game_exit` / parked exit 路徑保留資源直到程序結束，由 OS 回收。Trace 每筆立即 flush。
這避免新增 shutdown 與 game lookup 同時讀寫／釋放的競爭；沒有為此修改平台 backend。

## Commands / Native Tests

可重現的完整命令見 [README](../../i18n/README.md)。本機 compiler / DLL 路徑沿用
[Phase 0](PREFLIGHT.md)，replay 使用 `build/poc/native-v22.toml` 與原本未修改的 assertions。

| 檢查 | 本輪結果 |
| --- | --- |
| setup-port | 沿用已驗收 Native 環境，無新 dependency |
| Native build / port-check | PASS；game syntax check 0 errors / 0 warnings |
| Native launch | PASS；由以下真實 native replays 執行遊戲 |
| `test_i18n.py` | PASS，22 個既有 asset-free inventory tests |
| `test_runtime.py` | PASS，16 tests，含譯表全部 52 keys 的逐 bytes 比對 |
| 實際 C decoder / hook probe | PASS；10,984 entries + 2 missing-key cases，另測 block 0 / ring / dynamic |
| locale off replay | PASS，16 sessions、1,786 checkpoints，包含 saves |
| locale zh-TW replay | PASS，同樣 16 sessions、1,786 checkpoints；12 個不同 key 的實際譯文皆等於 TSV |
| Bragit talk | 兩種 locale 均 PASS，627 checkpoints；zh-TW trace 包含完整 key / UTF-8 text |
| fuzz / enhancements | PASS，5,680 cases / 120 checks |
| settings | Native aggregate PASS，72 個去重 assertions；原始 subprocess temp-name error 與補測分別保留 |
| movement / blackbox / map notes | PASS，15 / 22 / 4 checks |
| catalog 缺失的實際 Native launch | PASS，newgame 41 checkpoints 與 DOS golden 相同，診斷後使用英文 |
| CLI | PASS，help 列出新選項；未知 locale 在啟動前 exit 2 |
| `make repocheck` / `git diff --check` | PASS，提交前驗證，保留結果於 evidence |

公共 `talk` recording 選擇數次第一選項，最後停在長篇建議的 MORE prompt；
本輪 trace 確認起始對話、選項、探索回答、後續建議與 template fragment，
沒有宣稱該 recording 涵蓋 Bragit 的每一支線。全部 52 static keys 另以真實 C API 測試覆蓋。

本機 Native matrix 沿用前期的兩個測試環境處理：fuzz host 以 `-pthread` 連結相同 objects；
Windows settings 暫存資料夾將 label 的冒號換成底線，保持所有 test inputs / labels / assertions 不變。
原始 settings pool 會另啟 upstream subprocess，未繼承 wrapper，因此原始 exit 仍為 1：
68 個 assertions 通過後，`rt-4:3_aspect` 發生 WinError 267。只補跑 `roundtrip:1/3`，
相同 assertions 8/8 通過，其中 4 個已在原始執行通過；合計 72 個 distinct assertions。
沒有抹掉原始失敗 log，亦不將整條未修正的 upstream test command 稱為 PASS。
本輪同時更新 fuzz host 的 suffixless hardlink，使測試確實使用重新連結的目前 objects。
這些處理僅在本機測試 helper，沒有改動 Exhume 或 Native product implementation。

### DOS Exact-Match（歷史紀錄；目前不適用）

使用者已於本階段完成後取消 DOS 驗收要求；目前採 [Native-only 規則](VALIDATION.md)，
Borland 缺失不再列為驗證缺口、blocker 或後續工作。以下保留當時的執行事實。

Toolchain available: **NO**。

```text
DOS_EXACT_MATCH_CHECK:
NOT RUN — Borland reference toolchain not installed
```

`verify_dos_branch.py` 與 Phase 1 baseline 的 DOS branch token stream 相同。
此 structural guard 排除 headers、使用 Clang preprocessor，不代表 Turbo C compile/link 的 byte match。
Replay verifier 自行嘗試 DOS reference hash 時遇到相同缺口，之後依 upstream 行為使用已提交 goldens；
沒有修改／重新生成它們，也沒有將 DOS 重建稱為 PASS。依 v2.2，此缺口不阻塞 Native P2。

## Cross-platform / Risks

Windows 本輪實測 Native module、CLI、lookup 及 regression。
macOS / Linux 本輪未實機執行；C module 無平台分支／SDL dependency，catalog 明確指定 endian，
Python 使用 standard library，shared-library tests 支援 POSIX PIC。這些設計不能代替三平台實測。
Web 的公共 API／backend 未修改，本輪未額外作 Web compile。
既有 Windows symlink 權限與 private MT-32 ROM 缺失的測試限制沿用 Phase 0；
未另作實體滑鼠人工操作或 MT-32 音訊品質驗收。其餘受測 Native state / save 沒有 regression。

中文 layout、字型、渲染、實際繁中互動仍待後續階段。譯文為 PoC 草稿；
fallback 保留原版 bytes，後續 presentation 亦須正確處理其 encoding。
目前不提供 runtime locale switch；未來若需要，須建立明確的 reader 停止／同步協定。

## Evidence

- [Manifest](evidence/p2/manifest.json)：baseline、code / catalog hashes、驗收統計。
- [Runtime tests](evidence/p2/runtime-tests.log)、[inventory tests](evidence/p2/inventory-tests.log)。
- [Native comparison](evidence/p2/native-comparison.json)、[DOS branch guard](evidence/p2/dos-branch.log)。
- [Native build](evidence/p2/native-build.log)、[port-check](evidence/p2/port-check.log)。
- [English replay](evidence/p2/replay-en.log)、[zh-TW replay](evidence/p2/replay-zh-TW.log)、
  [translation hit summary](evidence/p2/replay-zh-TW.json)。
- [Bragit UTF-8 trace](evidence/p2/bragit-hits.jsonl)，僅 PoC 譯文，不包含完整原版文字庫。
- [Native matrix](evidence/p2/native-matrix.json)、[repocheck](evidence/p2/repocheck.log)。
- [Settings 原始 log](evidence/p2/settings.log)、[相同 slice 補測](evidence/p2/settings-roundtrip.log)、
  [缺 catalog 回歸](evidence/p2/missing-catalog.json)、[CLI checks](evidence/p2/cli.json)。

沒有新增中文 screenshot；真實遊戲仍維持英文，畫面基準由 replay screen digests 驗證。

## Git / Upstream Sync Gate

Current branch: `feature/zh-tw-native`。Phase 工作與報告 checkpoint：
`eb47037cf2083cef1714116eb0dd61f1f1f714ed`。

**NO CHANGE** — checkpoint 後確認 working tree clean，再 fetch Underworld origin / upstream、Exhume origin。
main / origin/main / upstream/main 均為 `e5deadd6945b492636aadccc14f52caa908b4186`，
main 對 upstream ahead/behind 為 0/0；feature 對 origin 為 1/0，僅本階段工作尚未推送。
Exhume pin 仍為 `a343164...`，origin/master 仍為 `c54e00e...`，既有 backlog 為 7 commits。
沒有新雲端提交、merge required 或 submodule reconciliation；沒有 mid-phase merge、rebase 或 force push。
Fetch 沒有引入來源變更，無 merge 後回歸需求；本輪 Native 結果保持有效。
[Sync metadata](evidence/p2/upstream-sync.json) 記錄完整 SHA 與 clean-state gate。

本節與 boundary 記錄以獨立文件 commit 收尾，之後建立 annotated `zh-tw-native-p2`，推送 feature 與 tag。
最終 checkpoint 由 tag 指向；文件不自我引用 commit SHA。提交後再次確認 working tree clean。

## Recommended Next Step

下一次執行 Phase 3 — UTF-8 / Chinese Layout Core，建立 renderer-independent 的
UTF-8 iteration、width-based wrapping 與基本標點禁則；仍不提前加入 FreeType / HD overlay。

**STOPPED HERE. Phase 3 was not started.**
