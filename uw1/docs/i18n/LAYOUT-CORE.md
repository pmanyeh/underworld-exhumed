# Phase 3 — UTF-8 / Chinese Layout Core

## Result

**PASS — Native Acceptance Gate P3，2026-10-21。**

已完成獨立於 renderer 的 UTF-8 scalar API、logical text units、整數寬度換行與基本中文標點禁則。
沒有將文字寫入低解析 framebuffer；目前未接入遊戲顯示，FreeType / HD overlay 屬後續階段。
依 [Native-only 規則](VALIDATION.md) 驗收，不要求或執行 DOS 建置／驗證。

## Baseline / Platform

| 項目 | Phase 開始時的版本 |
| --- | --- |
| Underworld upstream/main | `e5deadd6945b492636aadccc14f52caa908b4186` |
| Underworld feature | `d471bcd1c28a330b9efaab0fcc676f2272936e6b` |
| Exhume upstream origin/master | `c54e00e9176c14d0913b5806318a30f85a8f4636` |
| Exhume feature/current pin | `a34316463c230bafa3d4f0aa3b894a574e5fc738`，detached，無新 feature branch |
| OS / compiler | Windows、MSYS2 CLANG64、Clang 23.1.3 |
| SDL3 / Python | 3.4.18 / 3.12.10；沿用既有環境 |
| FreeType | 本階段沒有使用或加入 dependency |

開始前 working tree clean，fetch Underworld origin / upstream 與 Exhume origin 後沒有新提交。
Exhume 既有的 7 個 tooling/profile backlog commits 維持未合併。

## Changes / Files

- [utf8.c](../../../exhume/runtime/port/i18n/utf8.c)、[utf8.h](../../../exhume/runtime/port/i18n/utf8.h)：strict scalar iteration / validation / count / boundary。
- [layout.c](../../../exhume/runtime/port/i18n/layout.c)、[layout.h](../../../exhume/runtime/port/i18n/layout.h)：units、metrics callback、line ranges 與禁則。
- [localization.c](../../../exhume/runtime/port/i18n/localization.c)：catalog 共用新的 validator，保留拒絕 embedded NUL 的規則。
- [test_layout.py](../../tools/i18n/test_layout.py)：獨立編譯真實 C core，使用 Python 作 UTF-8 對照與排版 invariants。
- [test_runtime.py](../../tools/i18n/test_runtime.py)：既有 lookup 測試連結共用 UTF-8 module。
- [layout_stress.c](../../tools/i18n/layout_stress.c)、[verify_layout_sanitizers.py](../../tools/i18n/verify_layout_sanitizers.py)：ASan / UBSan 的 exact-length heap input 壓力測試。
- [i18n README](../../i18n/README.md)、本報告與精簡測試 evidence。

所有 runtime changes 均在 `uw1/src/port/i18n`。沒有修改 game/DOS sources、generated files、
Exhume、SDL backend、replay inputs、goldens、存檔格式或 gameplay。

## UTF-8 Contract

以 [RFC 3629](https://www.rfc-editor.org/rfc/rfc3629.html) 的 1–4 byte scalar encoding 為依據，
拒絕 overlong、surrogate、超出 U+10FFFF、非法 continuation 與截斷序列。

| API | 行為 |
| --- | --- |
| `utf8_next` | 以明示 byte length 解碼並前進一個 scalar；成功 1、結尾 0、錯誤 -1 |
| `utf8_prev` | 驗證並退回緊鄰的一個 scalar；不重新驗證整個 prefix |
| `utf8_validate` | 回報整段是否合法及第一個損毀序列的 lead-byte offset |
| `utf8_count` | 計算 scalar 數，錯誤不改寫 caller count |
| `utf8_boundary_before` | 取得不超過指定 offset 的最後 valid-prefix boundary；不跳過損毀 bytes |

next / prev 在 end/error 時不改寫 cursor 或 scalar。Primitive 允許 U+0000；
layout 與 catalog 因 C-string 使用情境，另外拒絕 embedded NUL。
沒有以 byte-by-byte substring 作換行或修復非法資料，也沒有偷偷插入 replacement characters。

## Layout Model / Policy

每個 unit 含 source byte offset/length、scalar、kind、advance 與 flags；
每一個來源 byte 都歸屬於一個完整 glyph、newline 或 control unit。
每行以 unit range 表示，保留 source order，另有 int64 width、hard-break / overflow flags。
輸出 owns units/lines，使用完呼叫 `layout_free`；不保存 source pointer，之後若需取 byte span，caller 須保留原文。
API 是 reentrant，無 global mutable layout state；測試亦驗證 callback 可遞迴呼叫獨立 layout。

寬度由 caller 的 nonnegative integer callback 提供，每個 glyph 只量測一次，
pixel 或 fixed-point 皆可，但 max_width 必須使用相同單位。控制碼與 newline 不呼叫 metric。
較寬 glyph / 禁則群組可能超出欄寬；以 int64 累加並標記 overflow，絕不切斷 UTF-8 bytes。
負 advance、非法 UTF-8、NUL、非空 output 或無效參數會明確回報錯誤；輸入上限 1 MiB。

換行政策：

1. ASCII 優先使用空白後的 break；過長單字在合法 scalar 邊界分割。
2. CJK 採字間換行。行首禁則包含 `，。！？；：、）》」』】`；行尾禁則包含 `（《「『【`，
   另收常見 ASCII brackets / punctuation 與 `〈〉〔〕｛｝`。
3. 判斷禁則時跨過零寬控制碼及空白，所以加色碼不能繞過標點規則。
4. 若寬度與禁則無法同時滿足，保留完整標點群組並標記 `LAYOUT_OVERFLOW`。不刪字、不無限迴圈。
5. ASCII space/tab 保留原始 units，視覺行首／行尾標為 `LAYOUT_TRIMMED`，不計 advance；全形空白保留縮排寬度。
6. LF、CR、CRLF 均是明示 hard break，CRLF 是一個 unit；空字串產生一行，末尾 newline 另保留空行。
   明示 hard break 的權限高於 soft-wrap 禁則；不修正作者本來就寫在邊緣的標點。
7. `\p`、`\P`、`\m`、`\0..\6` 是 atomic zero-width controls，通常跟隨右側 glyph 的行；
   核心不執行 pause / MORE / color。不認得的 backslash 序列保留為一般 glyphs。

這是 PoC 的明示、基本中文政策，不宣稱完整 Unicode Line Breaking Algorithm。
目前按 codepoint 處理，沒有完整 grapheme clusters、emoji ZWJ sequences、shaping 或 kerning；
callback 提供 additive advances，tab 亦由 callback 決定固定寬度，不是動態 tab stops。
尚未使用真實字型，沒有中文渲染畫面驗收。

## Tests / Commands

命令見 [README](../../i18n/README.md)，core tests 不需 SDL、FreeType 或原版遊戲 assets。

| 本輪驗證 | 結果 |
| --- | --- |
| `test_layout.py` | PASS，29 tests，真實 C core 以 `-Wall -Wextra -Werror` standalone shared library 編譯 |
| UTF-8 對照 | 全部 1,112,064 個合法 scalars；4,096 個 seed 3629 抽樣 forward/backward roundtrip |
| 非法 UTF-8 對照 | 10,000 組 seed 3629 random bytes 與 Python strict decoder 一致，另測 overlong / surrogate / truncation 等固定案例 |
| Layout coverage | ASCII、中文、中英混合、指定標點與括號、3/4-byte、empty、CRLF、controls、長單字、零寬／超寬 metrics |
| PoC 實際譯文 | 53 筆 × 4 種欄寬，212 次 source-byte coverage、line width 與禁則檢查 |
| 長文 / 隨機排版 | 90,000 scalars 確認 callback 各一次；100 組 seed 303 mixed input invariants |
| ASan + UBSan | PASS，100,000 組 exact-length random-byte heap inputs + 10,000 組 mixed-layout inputs，seed 303 |
| 既有 inventory / runtime tests | PASS，22 + 16 tests；含完整 52-key lookup、UTF-8 invalid catalogs 與 lifetime |
| Native build | PASS，89 game sources + 95 port sources；既有 mt32roms unused-variable warning，新增 core 無 warning |
| Native port-check | PASS，89 game C compile，0 errors / 0 warnings |
| English replay | PASS，16 sessions、1,786 checkpoints，包含 save comparisons |
| zh-TW lookup replay | PASS，16 sessions、1,786 checkpoints，12 個不同 key 的譯文與 TSV 相同 |
| 實際 decoder / hook probe | PASS，10,984 entries + 2 missing-key cases，含 block 0、dynamic 與八個 ring slots |
| Repository checks | PASS，syntax、links、禁止 binary/data 與 whitespace checks |

本輪新增及既有 unit tests 合計 **67 tests**。ASan / UBSan 檢查對象是 standalone text core，
不將此結果擴大為整個遊戲已通過 sanitizer。Windows 的 LeakSanitizer 不可用，本輪未宣稱 leak detection。
完整遊戲的 settings / movement / blackbox 等較廣矩陣為 Phase 2 歷史結果，本輪沒有重算其執行次數。
此次主要變更為獨立核心與共用 validator；Native build、雙 locale 公共 replay 及相應專項測試已完成。

### Native-only / Cross-platform

```text
DOS_VALIDATION:
NOT APPLICABLE — Native-only PoC
```

本輪沒有執行 DOS rebuild、DOS branch guard 或尋找 Borland。
Native replays 沿用已提交 golden snapshots，沒有生成／修改參考檔。
DOS source 路徑沒有變更；新增 runtime C 位於 project 原本排除於 DOS build 的 `src/port`。

Windows 本輪實測上述結果。macOS / Linux / Web 本輪未實機編譯／執行，不能以 portable C 設計取代實測。
核心與 headers 僅使用 C standard library，standalone shared-library test 支援 POSIX PIC，
沒有 SDL type、font API 或 Windows-only product shortcut；Web runtime API 沒有變更。

## Evidence / Samples

- [Manifest](evidence/p3/manifest.json)：baseline、code/executable hashes 與測試統計。
- [Core tests](evidence/p3/layout-tests.log)、[runtime tests](evidence/p3/runtime-tests.log)、[inventory tests](evidence/p3/inventory-tests.log)。
- [Sanitizer log](evidence/p3/sanitizers.log)、[logical layout examples](evidence/p3/layout-examples.json)。
- [Native build](evidence/p3/native-build.log)、[port-check](evidence/p3/port-check.log)、[decoder / hook](evidence/p3/native-comparison.json)。
- [English replay](evidence/p3/replay-en.log)、[zh-TW replay](evidence/p3/replay-zh-TW.log)、[hit summary](evidence/p3/replay-zh-TW.json)。
- [Repository checks](evidence/p3/repocheck.log)。

Samples 使用 synthetic metrics（ASCII=1、其餘=2；另有 W=9/i=2），不是 FreeType font measurements。
例如 `hello world` 在 width 5 分成 `hello` / `world`；`繁體中文測試` 在 width 4 分成三行；
`甲，乙` 在 width 2 保留 `甲，` 並標記 width 4 overflow，下一行才是 `乙`。
所有樣本保留 byte offsets 以便核對，沒有新增 renderer 截圖。

## Git / Sync Gate

Current branch: `feature/zh-tw-native`。Phase 工作與報告 checkpoint：
`92b5906088ab533277c85faa89ab667ff0e5c850`。

**NO CHANGE** — checkpoint 後確認 working tree clean，再 fetch Underworld origin / upstream、Exhume origin。
main / origin/main / upstream/main 均為 `e5deadd6945b492636aadccc14f52caa908b4186`，
main 對 upstream ahead/behind 為 0/0；feature 對 origin 為 1/0，僅本階段工作尚未推送。
Exhume pin 為 `a343164...`，origin/master 為 `c54e00e...`，既有 backlog 保持 7 commits。
沒有新雲端提交、merge required 或 submodule reconciliation；沒有 mid-phase merge、rebase 或 force push。
Fetch 沒有引入來源變更，無 merge 後回歸需求；本輪 Native 結果保持有效。
[Sync metadata](evidence/p3/upstream-sync.json) 保留完整 SHA 與 clean-state gate。

本節以獨立文件 commit 收尾，之後建立 annotated `zh-tw-native-p3` 並推送 feature 與 tag。
最終 checkpoint 由 tag 指向，避免文件自我引用 commit SHA；提交後再次確認 working tree clean。

## Recommended Next Step

下一次執行 Phase 4 — Generic HD Overlay Foundation：依計劃建立平台中立的 overlay plumbing，
先驗證 framebuffer 上方的 native-resolution RGBA layer；尚不接入中文／FreeType。

**STOPPED HERE. Phase 4 was not started.**
