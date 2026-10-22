# Phase 8 — Bragit Native HD Vertical Slice / PoC Report

**Native Acceptance Gate P8：PASS。技術驗收結論：PoC = GO。2026-10-22。**

Windows Native executable 已以玩家的合法原版資料，從新角色開始實際走到 Bragit，
經滑鼠啟動對話、完成七次選項與多輪繁中／英文 fallback，正常告別、查看背包、存檔、讀檔，再繼續移動與使用背包。
Linux、macOS Intel、Windows Native build 與獨立 renderer tests 通過；Web compile/link 通過。
完整遊玩流程在 Windows 實測，沒有宣稱 Linux/macOS 也已執行原版資料的完整流程。

依使用者更新的 [Native-only 規則](VALIDATION.md)，DOS／Borland 驗證為 **NOT APPLICABLE**。
沒有 DOS 執行、reference rebuild、golden 再生成，也沒有安裝 Borland 工具鏈。
本輪在 P8 結束；Phase 9、全遊戲翻譯、UW2、HD UI、HD 3D、IME 均未開始。
人工的 architecture／visual quality／performance／cross-platform viability／upstream maintainability 審閱，留待此技術驗收後進行。

## 實際玩家流程

所有步驟均使用 Native 程式原有的 keyboard／pointer event hooks。
沒有 `EXHUME_TEST_CHEATS=1`，沒有 `goto`、`give`、直接 `TalkTo` 或外部寫入遊戲狀態；
五份 recording 都沒有 cheat stream。只有 `skip-intro` enhancement，方便進入主選單。

| 流程 | 實際結果 |
| --- | --- |
| Launch / enter | 合法 GOG UW.EXE 與原 DATA；建立新角色 `a`，啟用外部 zh-TW catalog／FreeType／HD scroll／HD cursor |
| Walk to Bragit | 沿 public `talk` route 正常移動，取得並開啟背包、使用 Red Key 解鎖、開門；抵達 level 1 的 `(18.5, 6.5)` |
| Arrival save | 遊戲自行寫出 slot I，描述 `p8arrival`；沒有修改 PLAYER.DAT 或 LEV.ARK |
| Talk | 讀回 arrival save，以 F2 與視窗內的實際物件點選開始 Bragit 對話 |
| Multi-turn choices | 七次滑鼠選擇 `1, 1, 2, 2, 1, 1, 2`；對話涵蓋戰場、哥布林、逃獄與人類聚落 |
| MORE / fallback | 三次 MORE；長中文、多行、標點與明示換行；含 `@SS1` 的模板由原 BABL 展開為 `lad`，以 HD English fallback 顯示 |
| Exit / continue | 中文「祝你旅途順利。」後正常返回 3D；look mode 顯示 `You see a pack.`；slot II 存檔 `p8done` |
| Restore / save | 從主選單讀 slot II，再存 slot III `p8loaded`；移動半格，再存 slot IV `p8moved`；還原 slot III |
| Continue after restore | 位置恢復，能以 use mode 開啟背包並轉身；最後仍正常運行，無 halt 或 crash |
| Real resize | 在另一份無作弊的 Bragit 對話中開 F11 settings，將 3× 改為 4×，關閉 settings 後擷取 HD 文字 |
| English compatibility | 相同 arrival save、相同七個分支，以原英文 renderer／原 cursor renderer、數字鍵完成對話及存檔 |

本輪採可重跑的分段流程；arrival save 連接正常步行與對話，slot II 再連接讀檔／繼續遊玩。
每段皆為實際 Native game，並非用 presentation fixture 代替遊玩。
NPC 會走動；coarse click grid 的第一次 resize 嘗試未命中，不能算驗收。
最終 resize 使用較密的 ordinary talk clicks，並把 MORE acknowledgements 與 settings 操作分開；
validator 要求真的出現 Bragit 譯文、選項事件與正確尺寸，不以程式 exit 0 當成 PASS。

## Localization / display contract

Bragit whoami 67 對應 resolved string block **3651**；選定路徑命中 27 個 catalog keys，
每個 hit 均逐字比對 [外部 UTF-8 TSV](../../i18n/zh-TW/poc.tsv)。
NPC 原名 key `7:83` 仍可被查到；名稱標題的 legacy bitmap UI 保留英文，沒有擴充為 HD UI。
實際對話文字來自 catalog，沒有把本輪中文台詞寫進 C，也沒有把 UTF-8 寫入 VM、BABL heap 或原 byte buffers。
譯文仍為 PoC draft，未宣稱已完成人工校對或整個遊戲翻譯。

延續 [P7 contract](CHINESE-SCROLL.md)：兩個有界 scroll snapshots、game/presentation 分別擁有 font、
logical 26.6 度量、codepoint wrapping、基本禁則、控制碼與原 input reader。
Presentation 只讀 owned snapshot，字型在 Native output resolution rasterize，沒有經過低解析 framebuffer 放大。
Raw 320×200 的 NPC 與 main paper 均逐一 assert 為單一顏色，window capture 才出現 HD 字與 cursor。
實際 960×720 → 1280×960 resize 後，第一段回覆仍為相同的 246 個 prepared scalar cells；字形清晰、logical 換行與選項位置保持一致。

含 `@` 模板與缺譯／dynamic literals 保留英文 fallback；`[MORE]` 與其餘 bitmap UI 仍是英文。
這符合本輪的 dynamic/fallback gate，不代表已完成中文模板替換、中文輸入或 IME。
完整 Unicode shaping、grapheme、kerning、UAX #14 及實體跨螢幕 DPI 拖移不在本次已驗證範圍。

## Gameplay / replay / save evidence

[acceptance.json](evidence/p8/acceptance.json) 由 [validate_vertical_slice.py](../../tools/i18n/validate_vertical_slice.py)
讀取實際 trace、PNG、recording digests 與 saved files 產生。

| 錄製 | 原 checkpoints | 每份 replay checkpoints | 結果 |
| --- | ---: | ---: | --- |
| 新角色／完整步行／取得物品／解鎖／arrival save | 249 | 250 | PASS |
| Bragit 七次滑鼠選項／告別／look pack／存檔 | 306 | 307 | PASS |
| 讀檔／再存檔／移動／再還原／use pack | 122 | 123 | PASS |
| 原英文對話與存檔 | 300 | 301 | PASS |
| 實際 Bragit 對話與 settings resize | 478 | 479 | PASS |
| 合計 | 1,455 | 1,460 | 各兩次一般 replay + 一次完整遊戲 sanitizer replay |

[verify_native_recording.py](../../tools/i18n/verify_native_recording.py) 要求原錄製每一個 checkpoint 的 header 與
全部 canonical sections 相同，saved files 相同；三次 replay 連 terminal 也一致，合計 4,380 個 replay checkpoints。
GUI timed exit 不寫 terminal；每份 replay 只允許額外的一個 kind-4 terminal，event count 必須等於 recording stop。
沿用 upstream canonical masks／segment normalization，沒有新增 section exclusions，沒有修改 goldens。
這裡的「canonical 相同」不代表未正規化的原始 memory dump 每一 byte 相同。

存讀檔另驗證：

- Slot II → restore → slot III 的 PLAYER.DAT，除了既有四-byte game clock，**其餘 player／inventory bytes 完全相同**。
- Slot II、III、IV 的 BGLOBALS.DAT 完全相同；移動後位置由 `(4736,1664,768)` 變為 `(4608,1664,768)`，即半格。
- 再還原 slot III 後，Native motion log 的位置回到 `(4736,1664,768)`；其後可開背包、轉身至 facing 57344。
- 繁中與獨立英文七輪流程的 **整份 BGLOBALS.DAT 相同**；player 除 facing／clock 外相同，avatar record 後的 serialized item slots／objects 相同。

兩種語言的獨立遊玩可能在不同朝向找到走動中的 NPC，paging／clock 時序也不同；
上述跨語言語義檢查明確排除 facing／clock／avatar motion record。
它與「同一份 recording 的所有 canonical sections 必須一致」是不同檢查，後者沒有增加任何排除。

## 本輪找到並修正的 Native memory access

完整遊戲以 `--debug --sanitize address,null,bounds` 建置，fail-fast replay。
三次初始失敗 logs 與修正後結果全部保留，沒有停用 failing checks 或加 sanitizer suppression。

| 問題 | Native 修正 | 證據 |
| --- | --- | --- |
| GRIDDB 用第一個 64-byte row pointer 加上整張 automap 的線性 index，觸發 bounds | 取完整 64×64 array 的 byte representation，仍存取相同 automap bytes | [initial bounds log](evidence/p8/initial-bounds-failure.log) |
| pick_3d 將 1-byte PickUp 讀成 2 bytes；DOS padding 不適用於 Native globals，ASan 報 overread | Native 比較只讀 byte counter | [initial PickUp log](evidence/p8/initial-pick-overread-failure.log) |
| store_path 封裝最後一個 byte 時，未使用的尾端 steps 仍讀 stale squares，可能越界索引 3×3 direction table | Native 跳過超過 pathlen 的 direction／slope padding bits，不改已使用步驟 | [initial path log](evidence/p8/initial-path-tail-failure.log) |

三者都保留 `__TURBOC__` 原路徑；沒有為了方便 PoC 改變對話分支、穿牆或跳過物件互動。
修正後五份完整遊玩 recording 全部 sanitizer replay PASS，原 public compatibility goldens 也 PASS。
既有 `inv_look`／`checkTrap` 的 FARNULLTRAP emulated-null reads 仍會記錄；
它們是既有受控的 emulated memory 行為，不是此處隱藏的 host ASan／UBSan failure。

Full-game replay 是 headless；實際 Windows gameplay window 另外實測。
真實 FreeType renderer 與八個 SDL scroll window scenarios 另以 ASan/UBSan 通過。
Windows 未使用 LeakSanitizer，SDL／FreeType DLL 未以 sanitizer 重編；不宣稱對全部遊戲路徑做過記憶體安全證明。

## Regression / build / performance

| Gate | 結果 |
| --- | --- |
| Local Windows build | MSYS2 CLANG64／Clang 23.1.3；89 game + 103 port C compile/link |
| Windows port-check | 89/89，0 errors／0 warnings |
| Compatibility | English 與 zh-TW lookup 各 16 sessions／1,786 checkpoints；原 sections、screens、saved files 與 committed goldens 相同 |
| Native matrix | graphics fuzz 34 routines／5,680 cases，0 differences；enhancements 120/120；settings 72/72；movement 15/15；blackbox 22/22；map notes 4/4 |
| Text / font | inventory 22、runtime 16、layout 29；10,984-string Native probe + 2 missing；FreeType 4,411,580 assertions；no-FreeType stub PASS |
| Cursor / scroll | actual MOUSE.C 39 assertions；50,000 cursor snapshot publications；5,000 scroll publications；core／renderer sanitizer PASS；real FreeType scroll renderer 75,258 assertions |
| SDL scroll window | 八種實際 window／resize／aspect／fractional／fullscreen／return scenarios PASS |
| Native CI | Linux Ubuntu 24.04、macOS 15 Intel、Windows compile/link、font／cursor／MOUSE／scroll tests 均 success |
| Web CI | Emscripten compile/link success；不宣稱 Web HD scroll runtime 已驗收 |

最終程式碼 `ec959aa8585cf8cf8488e1debc0371b065d6da3a` 的
[CI run 39718841930](https://github.com/pmanyeh/underworld-exhumed/actions/runs/39718841930) 四個 jobs 全部成功；
[metadata](evidence/p8/ci.json) 與選錄 logs 保留。
Linux/macOS port-check 的 46 個既有 warnings 與原 sound-library unused-variable warning 保留，沒有視為新失敗。
Concurrent assertion counts 隨 reader 排程變動，不能當成不同語言案例的數量。

完整 Bragit window run：2,140,887 cache hits／310 rasterizations／310 entries，84,726 bytes glyph masks。
Resize run：645,958 hits／196 rasterizations，56,404 bytes；font／catalog 不在每 frame 重新載入或解析。
Presentation 使用 immutable copied snapshots；FreeType／locale 邏輯留在 UW1，Exhume 只提供通用 overlay plumbing。
這些是本機驗證範圍的 cache／運行證據，尚非跨硬體效能 benchmark。

## 畫面與可重跑方式

[抵達並存檔](evidence/p8/walk.png) · [繁中對話／fallback／選項](evidence/p8/dialogue.png) ·
[實際 4× resize](evidence/p8/resize.png) · [讀檔後開背包](evidence/p8/restore.png) ·
[原英文模式](evidence/p8/english.png)。PNG 只做 lossless recompression，decoded pixels 已驗證完全相同。
[manifest](evidence/p8/manifest.json) 記錄 source／exe／font／catalog／image／save／recording SHA-256；
只散布 evidence、翻譯資源及 source，不散布原版 data、exe、saved games、recordings、font 或 DLL binaries。

遊玩啟動方式見 [README](../../i18n/README.md#chinese-conversation--scroll)。
測試需自己的合法 data、已建置 executable、font/catalog 與新的 output directories；在 `uw1/`：

```sh
python3 tools/i18n/record_vertical_slice.py --case walk --data "$UW1_DATA" --output build/p8-new/walk
python3 tools/i18n/record_vertical_slice.py --case dialogue --data "$UW1_DATA" --stage build/p8-new/walk --output build/p8-new/dialogue
python3 tools/i18n/record_vertical_slice.py --case restore --data "$UW1_DATA" --stage build/p8-new/dialogue --output build/p8-new/restore
python3 tools/i18n/record_vertical_slice.py --case dialogue --english --data "$UW1_DATA" --stage build/p8-new/walk --output build/p8-new/english
python3 tools/i18n/record_vertical_slice.py --case resize --data "$UW1_DATA" --stage build/p8-new/walk --output build/p8-new/resize
```

以 `verify_native_recording.py --help` 的參數逐份驗證；`--sanitize` 另需相同 source 的
`portbuild.py --debug --sanitize address,null,bounds` build。Stage 只需上一段必要的 SAVE1..4，output 必須是新目錄。
最後 `validate_vertical_slice.py` 接收五個 case outputs、五份 replay reports 與 `--report`。
本機最初的 walk／dialogue helper 實際版本、generated input scripts、commands 與 regression wrappers 保存在 evidence；
它們含本機路徑，重跑時應使用上述通用 runner 或自行調整路徑。

## Definition of Done / review boundary

| 項目 | 驗收 |
| --- | --- |
| Native launch、合法 data、fork/upstream 分離 | PASS |
| Generic Exhume overlay、UW1 owns localization/font/cursor policy | PASS；沿用 P4/P5 architecture，P8 未修改 Exhume |
| 外部 UTF-8、block/index、FreeType、Native masks、logical/native 分離 | PASS |
| UTF-8 safe wrap、基本禁則、controls、MORE | PASS |
| 原 mouse logic、HD cursor、hotspot／click／z-order | PASS |
| 實際 Bragit 多輪流程、conversation globals、save/load、continued play | PASS |
| English compatibility、Windows/Linux/macOS Native builds | PASS，完整遊玩在 Windows |
| DOS exact-match | NOT APPLICABLE — 使用者指定 Native-only |
| Report / STOP | 本報告完成；停在 P8，待人工審閱，不自動開始 P9 |

## Baseline / phase boundary sync

| Phase 開始項目 | SHA |
| --- | --- |
| Underworld upstream/main | `e5deadd6945b492636aadccc14f52caa908b4186` |
| Underworld feature / P7 tag | `5175c2e9d3326382220a8233a651407d73a3e7e9` |
| Exhume upstream/master（P7 boundary 已觀察） | `aa2353a2442c9cb9ef2267aabd9398abb8aac42d` |
| Exhume accepted runtime base | `c54e00e9176c14d0913b5806318a30f85a8f4636` |
| Exhume feature / submodule pin | `89278495bb0b8b224b5bb58e53da2fd367c0d8c0` |

P7 已記錄的 ARMCC5／tooling／documentation backlog 沿用，不宣稱已整合最新 Exhume upstream。
驗收 evidence checkpoint：`cc8f0e2f5078c201377fa85689e947923ee70793`。
該提交後，主 repository 與 Exhume working trees 都 clean，才執行兩邊 `fetch --all --prune`。

| 結案檢查 | 結果 |
| --- | --- |
| Underworld main / origin/main / upstream/main | 全部仍為 `e5deadd6945b492636aadccc14f52caa908b4186`，`0/0` |
| Underworld feature vs origin（結案 push 前） | `1/0`，唯一未 push 的提交為上述 evidence checkpoint |
| Exhume upstream/master | 仍為 `aa2353a2442c9cb9ef2267aabd9398abb8aac42d`，相較 P7 boundary 沒有新提交 |
| Exhume main / origin/main | 仍為 `c54e00e9176c14d0913b5806318a30f85a8f4636`，vs upstream `0/4` |
| Exhume feature / origin feature / pin | 全部仍為 `89278495bb0b8b224b5bb58e53da2fd367c0d8c0`，vs origin `0/0`，vs upstream `1/11` |

**UPSTREAM_SYNC = NO NEW CHANGES；既有 Exhume backlog 仍 DEFERRED。**
沒有 merge/rebase、更新 fork main 或改動 submodule pin；P8 source、tests 與 evidence baseline 未變。
詳見 [sync.json](evidence/p8/sync.json)。`zh-tw-native-p8-poc` annotated tag 指向本輪結案提交，
與 `feature/zh-tw-native` 一同 push。**P8 到此停止，未開始 P9。**
