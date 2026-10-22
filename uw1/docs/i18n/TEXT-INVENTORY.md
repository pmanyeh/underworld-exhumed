# Phase 1 — Text Inventory / Translation Data

目前依 [使用者更新的 Native-only 規則](VALIDATION.md)，不要求 Borland / DOS 驗收。
以下 DOS NOT RUN 為本階段當時的歷史紀錄，不再列為驗證缺口或待解決事項。

## Result

**PASS — Acceptance Gate P1，依計劃 v2.2，2026-10-21。**

完成可重現的 STRINGS.PAK / CNV.ARK 盤點、hard-coded literal 分類與 Bragit 繁中 PoC 資料。
遊戲仍顯示英文；本階段尚未新增 runtime lookup、FreeType 或 HD overlay。

## Baseline / Platform

| 項目 | Phase 開始時的 SHA / 版本 |
| --- | --- |
| Underworld upstream/main | `e5deadd6945b492636aadccc14f52caa908b4186` |
| Underworld feature | `a2c1e960a67e179c94ea6011c593b8dbe1c2a360` |
| Exhume upstream origin/master | `c54e00e9176c14d0913b5806318a30f85a8f4636` |
| Exhume feature/current pin | `a34316463c230bafa3d4f0aa3b894a574e5fc738`；detached，尚未建立 Exhume feature branch |
| OS | Windows，PowerShell / MSYS2 CLANG64 |
| Compiler / SDL3 | Clang 23.1.3、SDL3 3.4.18；libmt32emu 2.8.3 |
| Python | 3.12.10，盤點工具僅使用 standard library |
| FreeType | 本階段未使用，未作為驗收依賴 |

開始前 working tree clean，已 fetch Underworld origin / upstream 及 Exhume origin，沒有新提交。
Exhume pin 後的 7 個既有 tooling/profile commits 保持未合併，詳見 [Phase 0](PREFLIGHT.md)。

## Changes / Files

- [extract_strings.py](../../tools/i18n/extract_strings.py)：解碼 Huffman、block/index，輸出 UTF-8 TSV 與可選 CNV header metadata。
- [scan_literals.py](../../tools/i18n/scan_literals.py)：C literal inventory 與五類保守分類。
- [validate_poc.py](../../tools/i18n/validate_poc.py)：檢查 source drift、key、PoC 範圍、模板與控制碼。
- [native_probe.c](../../tools/i18n/native_probe.c)、[verify_native.py](../../tools/i18n/verify_native.py)：連結未修改的 native decoder，逐筆比較原始 bytes。
- [test_i18n.py](../../tools/i18n/test_i18n.py)：不需原版 assets 的 malformed-input / format / token tests。
- `poc.tsv`：53 筆翻譯草稿（Phase 9 已無損遷移至 [master.tsv](../../i18n/zh-TW/master.tsv) 與 [literals.tsv](../../i18n/zh-TW/literals.tsv)）。
- [資料契約與操作說明](../../i18n/README.md)、本報告與精簡驗收 evidence。

未修改任何 `uw1/src`、generated file、gameplay、renderer、Exhume source 或 submodule pin。
完整英文盤點、EXE、PAK、ARK、objects、DLL 與暫存檔均只留本機，不提交 Git。

## Inventory / Identity Findings

PAK 共 **122 blocks、10,984 entries**；保留空字串。13 個 category 包含全部十個一般 block、
cutscene、conversation，以及用途未作 runtime 證明的 block 24（`unknown_static`，內容含 dungeon report）。
有 **41 entries 的 index >511**（block 2 的 9 筆、block 5 的 32 筆）；僅保留盤點，
不能直接套入 `get_string` 的九位元 index。譯表驗證器會拒絕它們。

所有已解碼輸出均為 ASCII，採 CP437 可逆映射後輸出 UTF-8。
PAK 解碼保留遊戲的 MSB-first bit order、兩種 terminator、uint16 seek sum 及 511-byte C string 上限。
損毀的 tree、cycle、越界／截斷及超出遊戲 signed header 範圍的資料會清楚報錯。

CNV.ARK 有 97 個非空 headers。**Bragit slot 67 的 header 實際指向 block 3651 (`0xE43`)**，
有 1,695 code words。該 block 的 index 0 為空，1–49 全部納入翻譯；
包含性別片段、初次與再次見面、玩家選項和離別等支線，無需先重建 conversation VM。
block 7/index 83 為姓名，另收兩則一般 scroll message 與一個 `[MORE]` literal sample，共 53 筆。

static identity 為 **resolved `(block,index)`**，不能直接以原始 packed ID 作翻譯 key。
raw block 0 依當前 conversation/cutscene context 解析，dynamic/made blocks 的查找優先於 static lookup。
`0x7C` 的組成字串與 `0x7D` 的玩家姓名均繞過靜態翻譯；不將執行期輸入當原文。
154 筆來源含 `@`，此數字只表示 marker 存在，不代表全部都是同一種 template。
Bragit 的 `@SS1` 與 `\m` 等控制碼已原樣保留。八個 512-byte 循環 buffers 的生命週期維持不變。
完整規則見 [README](../../i18n/README.md)。

literal scan 共 **2,507 筆**：PLAYER_VISIBLE 60、DEBUG_ONLY 21、PORT_UI 52、TEST_ONLY 10、UNKNOWN 2,364。
掃描範圍含 UW1 C/headers、固定 pin 的 runtime，以及本階段 C test driver。
UNKNOWN 是人工追蹤佇列，包含 include/path、formats、間接引用與尚待判斷的顯示字串；
本階段不宣稱完成全遊戲的 literal localization。PoC 選定的 `[MORE]` 已追到實際顯示呼叫。

## Commands / Tests

可移植的完整命令見 [README](../../i18n/README.md)。本機使用 Phase 0 的 no-space junction、
CLANG64 與 Python 環境；回歸沿用 `build/poc/native-v22.toml` 的原始資料與 Windows host 設定。

| Native check | 本輪結果 |
| --- | --- |
| setup-port | 沿用已驗收環境；沒有新 Native dependencies |
| `make GAME=uw1 ... port` | PASS，重新連結英文 Native executable |
| native launch / public regression | PASS，16 sessions、1,786 checkpoints；含 save comparison |
| Bragit `talk` replay | PASS，627 checkpoints 與既有 DOS goldens 相同 |
| native C decoder comparison | PASS，10,984 entries + 2 missing-key cases |
| block 0 C probe | PASS，`get_string(3)` 在 context 3651 等於 `read_string(3651,3)` |
| dynamic C probe | PASS，`make_string/get_string` 對 0x7C、0x7D 保留 pointer identity |
| extraction / literal scan repeat | PASS，兩次 TSV byte-for-byte 相同 |
| `validate_poc.py` | PASS，53 筆 source key、coverage、control/template checks |
| asset-free `unittest` | PASS，22 tests |
| `make repocheck` / `git diff --check` | PASS，syntax、文件 links、禁止資料／binary 檢查通過 |

本輪執行 `replay.py --config build/poc/native-v22.toml verify all`。
工具先嘗試重建 DOS reference hash，因 Borland 缺失而失敗，之後按原工具行為使用已提交 goldens；
最終 16 組 Native 比對全部通過。沒有將 DOS 重建稱為 PASS，也沒有重建／改寫 goldens。
原先誤用 `verifyall` 只顯示 usage，未當作驗證；已改以正確 `verify all` 完成上述回歸。
這階段沒有 C/runtime 變更，其餘 Phase 0 的 fuzz、settings、movement、blackbox、map-note 結果見前期報告，
不將那些前期執行次數重算為本輪新增測試。

### DOS Exact-Match

Toolchain available: **NO**。

```text
DOS_EXACT_MATCH_CHECK:
NOT RUN — Borland reference toolchain not installed
```

依 v2.2 保持可追蹤的 reference 驗證缺口，不阻塞 Native Phase 1。

## Cross-platform

Windows 本輪實測工具、Native build、decoder 與 replay 通過。
macOS / Linux 本輪未實機執行新增 Python/driver；工具使用 standard library，
driver 沿用 upstream compiler/library 設定，沒有 Windows 專用功能分支作為必要條件。
Web runtime/API/source 未變更，本輪未另作 Web build。Phase 0 的相同遊戲來源 CI 證據仍保留，
不能據此宣稱新增工具已完成三平台實測。

## Screenshots / Evidence

- [Manifest](evidence/p1/manifest.json)：input hashes、統計、重現性及 Native executable hash。
- [26 個跨類別 samples](evidence/p1/samples.tsv)：13 類各兩筆，保留短摘錄、完整 bytes digest 與 Native match。
  實際比較涵蓋全部 10,984 entries，並非只比對這 26 筆。
- [Literal 分類樣本](evidence/p1/literal-samples.tsv)、[C decoder report](evidence/p1/native-comparison.json)。
- [Native build log](evidence/p1/native-build.log)、[replay log](evidence/p1/replay.log)。
- [Unit-test log](evidence/p1/unit-tests.log)、[data validation log](evidence/p1/data-validation.log)、[repocheck](evidence/p1/repocheck.log)。
- 前期 [Bragit choices](evidence/talk-ck0550.png) 與 [後續回應](evidence/talk-ck0626.png)
  可對照英文內容；本輪沿用相同 session 並通過全部 checkpoints。沒有新增中文畫面或人工操作驗收。

## Regressions / Remaining Risks

受測 Native state / save 沒有 regression。翻譯尚未接入 runtime，中文 layout、glyph、overlay 與
繁中多輪互動均留待後續階段驗收。譯名、語氣與句長是 PoC 草稿，待畫面出現後校對。
Literal 的 UNKNOWN 與物品名稱文法仍需各自階段追蹤，不可批次套用一般文字替換。
原版資料來源為玩家本機 GOG baseline；其他版本的 text/key drift 由 validator 拒絕接受，不能假定通用。

## Git / Branch State / Upstream Sync Gate

Current branch: `feature/zh-tw-native`。Phase 工作與報告 checkpoint：
`f659e05544310fbf1fca3a03c9b4051a6402f948`。main / origin/main / upstream/main
均保持 `e5deadd6945b492636aadccc14f52caa908b4186`。

**NO CHANGE** — checkpoint 後確認 working tree clean，再 fetch Underworld origin / upstream、Exhume origin。
兩個 upstream SHA 均與 Phase 開始相同，main 對 upstream ahead/behind 為 0/0。
當時 feature 對 origin 為 1/0，唯一新增 commit 即本階段工作，沒有雲端競爭提交。
Exhume pin 仍為 `a343164...`、origin/master 仍為 `c54e00e...`，既有 backlog 維持 7 commits。
沒有 merge required、沒有 submodule reconciliation，也沒有 mid-phase merge、rebase 或 force push。
因 fetch 未引入來源變更，無 merge 後回歸需求；本階段實測結果保持有效。
[Boundary sync metadata](evidence/p1/upstream-sync.json) 記錄完整 SHA。

Boundary 記錄以獨立文件 commit 收尾，之後建立 annotated `zh-tw-native-p1` 並推送 feature 與 tag。
最終 checkpoint 由 tag 指向，避免在文件內自我引用 commit SHA；提交後再次確認 working tree clean。

## Recommended Next Step

下一次開始 Phase 2 — Runtime Localization Lookup：以 resolved key 查找 UTF-8 translation，
驗證 locale off、missing fallback、block 0、dynamic bypass 與 buffer lifetime；仍先不修改 renderer。

**STOPPED HERE. Phase 2 was not started.**
