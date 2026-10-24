# Phase 7 — Chinese Wrapping / Scroll Integration

**PASS — Native Acceptance Gate P7，2026-10-22。**
繁中／中英混排已接進實際 UW1 message scroll、NPC scroll 與 conversation choices。
使用者指定的 [Native-only 規則](VALIDATION.md) 持續適用；沒有 Borland／DOS build、DOS 執行或 golden 更新。
本輪是 Phase 7 integration；尚未執行 Phase 8 正式 vertical slice，也不宣告 PoC GO。

## 啟動

依 [README](../../i18n/README.md#chinese-conversation--scroll) 產生 catalog、下載字型與建置。
在 `uw1/` 執行，使用自己的合法 data 與獨立 home：

```sh
build/port/uw1port --data "$UW1_DATA" --home "$UW1PORT_HOME" \
  --locale zh-TW --i18n-file build/i18n/zh-TW.uwloc --hd-scroll \
  --font tools/fonts/NotoSansCJKtc-Regular.otf
```

Windows 用 `uw1port.exe`。Font／catalog 相對目前目錄，也可用絕對路徑。
`--hd-scroll` 要求 `--locale zh-TW`、可用的 FreeType 與 scalable Unicode font，並自動選用 P6 HD cursor。
可與 mouse-look 同時使用（P12 起；PoC 階段曾明確拒絕，見 [HD-CURSOR.md](HD-CURSOR.md)）。
Font 失敗不會悄悄退回低解析中文；catalog 缺少／失效則延用原有 English lookup fallback。
`--scroll-log FILE` 記錄 UTF-8 text、pause、MORE 與選項事件。

`--locale en`／未加 HD scroll 保留原 renderer。`--locale zh-TW` 單獨使用仍是 P2 的 lookup／trace 路徑。
`--screenshot` 為原低解析 scanout；`--window-shot` 才含 HD 文字與游標。
不需加入 P5 的 `--test-hd-text`；該選項只會額外顯示三句示範文字。

## 資料與 thread contract

[localization.c](../../src/port/i18n/localization.c) 增加有界的 display provenance：
保存 decoded ring pointer、原英文的 owned copy 與 catalog target，lookup 不改變 `get_string` 回傳值。
同英文內容的另一個 pointer 不會自動獲得翻譯。來源被修改、metadata eviction、缺 key、source drift，均回到原文。
Made/dynamic block 的 `get_string` 會清除該 pointer 的 provenance，避免動態字串誤用舊的靜態翻譯。
Catalog／provenance 仍由 game thread 擁有，沒有 runtime reload。

[CONVERSE.C](../../src/conv/CONVERSE.C) 在顯示邊界取翻譯；選項的 target 在取得原 key 時保存，
VM、比較／文法函式、BABL heap 與 `babl_display` 仍保留原資料。UTF-8 不進 `conv_buffer`、160-byte menu line
或 49-byte scroll copy。顯示前綴、編號、控制碼與全文放入另外配置的 owned display buffer。

含 `@` 的模板整句使用原 BABL 已展開的英文；本輪沒有重寫 interpreter／template substitution。
例如實際 Bragit 分支中的 `@SS1` 已由原邏輯變為 `lad`，再以 HD English 顯示；不會把 `@SS1` 留在畫面。
其他缺譯、dynamic string、硬編碼 UI literal 也使用 fallback。Legacy CP437 high bytes 先轉成 UTF-8。
`[MORE]` 沿用英文 label；原 TSV 的 literal namespace 仍保留，沒有加入新的 literal runtime replacement。

[SCROLLIO.C](../../src/ui/SCROLLIO.C) 的 Native opt-in 分支略過 byte wrapping 與 legacy glyph raster。
實際紙面清為原 paper colour，文字只由 HD layer 顯示。Clear／重新初始化會清掉 display state，
離開 conversation 清掉選項與 NPC 內容；map／menu 不顯示 scroll overlay。
ASCII typed-answer／yes-no editor 暫時保留原路徑並隱藏 main HD scroll，結束後清理該 scroll；未實作中文輸入或 IME。

[scroll_text](../../src/port/presentation/scroll_text.h) 是 game-thread producer：
每個 scroll 有最多 1,024 個 owned scalar cells，包含 logical position、palette colour、choice identity。
輸入上限為 16 KiB，strict UTF-8；先複製文字再執行可能等待輸入的控制碼，不借用 caller 的字串跨 wait。
Snapshot 以 acquire/release atomic lock 複製，明確保存 Native struct packing；presentation thread 不讀 game structs、
string ring、BABL buffer 或 mutable text pointers。

Game 與 presentation 各有一個獨立的 FreeType font instance。
Game font 以 64px reference advance 產生 logical 26.6 度量：5 logical em、5/4 水平預留、8 logical pixels 行高。
Latin 使用比例寬度，CJK 使用該 face 的 advance；不是以 UTF-8 byte 數決定長度。
Presentation font 按當前 viewport 的 Y scale 重新 rasterize，每個 scalar 畫在已決定的 logical 位置，
clip 到 paper bounds。Resize／aspect／fullscreen 不重排文字，也不改選項 hit rows。
既有 glyph cache 與 native grayscale alpha composition 繼續使用。

## Wrapping / controls / input

復用 P3 [layout](../../src/port/i18n/layout.h)：只在 codepoint boundary 斷行、英文優先 space、長單字 fallback、
中文字間換行、保留 CR/LF/CRLF 與明示換行。補上 `…` 的行首禁則，與計劃列出的標點一致。
含 `scroll_esc=0` 的資料將 backslash 當 literal，避免 save descriptions 中的文字被執行為控制碼。
沿用 P3 的基本 kinsoku／overflow policy，並非完整 UAX #14、grapheme、kerning 或 shaping。

- `\p`／`\P`：沿用 `scroll_wait`，分別等待 600／200 ticks 或輸入變化。
- `\m`：顯示 `[MORE]`、等待原 input reader，移除 prompt 後繼續。
- `\0..\6`：沿用原 palette indices `2E,26,F1,60,B4,C4,D4`。
- 長文自動分頁也走原 `scroll_wait`。段落中途的 `\m` 保留目前寫入行，不會跳到紙面底部。
- Pending newline 延至後續文字才前進，避免末尾 newline 額外產生空白頁。

每個可見選項行保存原 choice identity；`conv_play_menu` 的 HD click path 依實際可見行選擇，
不沿用 byte-wrap 產生的舊 line table。Keyboard number、option IDs、VM return、allocation/free 邏輯保留。
疊放順序為 game → HD text → HD cursor → settings。

## 本輪驗證

| 項目 | 結果 |
| --- | --- |
| Windows Native | 89 game C + 103 port C compile/link；既有 mt32roms unused-variable warning |
| Windows port-check | 89 C，0 errors / 0 warnings |
| Flow core | 短／長／無空白中文、中英混排、標點、長英文單字、partial-line append、explicit newline、多頁、empty／invalid UTF-8 |
| Controls | 所有 colour escapes、600/200-tick pause、explicit／automatic MORE、literal backslash、MORE 後目前行位置 |
| Lifetime / state | provenance identity、source mutation、dynamic invalidation、CP437 fallback、5,000 次 concurrent snapshot publications |
| Memory | flow、real FreeType renderer、真實 SDL window 都以 ASan + UBSan 通過；Windows 不含 LeakSanitizer |
| FreeType headless | proportional logical metrics、native masks、warm-cache reuse、pitch／guard bytes、logical clipping；本機 75,258 assertions |
| SDL window | 8 scenarios：1×、3×、resize、aspect on/off、fractional letterbox、fullscreen／return；固定 layout、choice hit、settings 上層 |
| Actual UW1 | 6 Native fixtures：Bragit 開場、點選、短中文、長文 MORE、全部 controls、空文字；目視檢查 screenshots |
| 原英文底字 | 逐一 assert raw 320×200 main paper 為均一顏色；Bragit NPC paper 也相同，HD text 只存在 window capture |
| Choice／dynamic fallback | 真實 mouse held-click 選到 option 1；後續中文台詞、MORE、展開英文 fallback 與下一組中文選項正確 |
| Native HD replay | 6 新錄製 fixtures；原錄製的 81 個 checkpoints 全部 canonical sections／headers 相同；每份 replay 兩次，含 terminal 均一致 |
| Compatibility replay | English 與 zh-TW lookup 各 16 sessions／1,786 checkpoints，原 sections／screens／saved files 與 committed goldens 相同 |
| Existing text | layout 29、runtime 16、inventory 22、FreeType 原測試 4,411,580 assertions、no-FreeType stub PASS |
| Cursor | P6 pixel/concurrency sanitizer 與 8 個實際 SDL cursor scenarios PASS |
| Native string probe | 10,984 strings + 2 missing keys、block 0、dynamic、ring lifetime PASS |
| Native matrix | graphics fuzz 34 routines／5,680 cases、enhancements 120/120、settings 72/72、movement 15/15、blackbox 22/22、map notes 4/4 |
| CLI diagnostics | 非 zh-TW locale、缺失 font、mouse-look 衝突三種情形均以 exit 1 明確報錯 |

Concurrent reader 的 assertion 數量隨排程改變，不代表數萬組不同的語言 cases。
原圖／字型／第三方 library 未重新繪製；SDL／FreeType DLL 本身沒有以 sanitizer 重編。

新 fixtures 使用隔離的 home、原 `items` Native verified save stage，與受 `EXHUME_TEST_CHEATS=1` 保護的
`talk WHOAMI`／`scroll UTF8_FILE` test commands；實際走 `TalkTo`、BABL、SCROLLIO 與輸入處理。
它們用於 P7 integration，沒有宣稱完成「走到 Bragit、完整多輪／離開／繼續遊玩、modern save/load」的 P8 操作流程。
正常遊玩不需開啟這些 test commands。

Actual mouse fixture 使用分開的 press/release，避免同一次 event-queue pump 的瞬間 click 被 game polling 略過。
Window→logical mapping 則由 8 個真實 SDL window scenarios 單獨驗證。

GUI timed exit 只關閉 recording stream；replay 會額外寫出一個 kind-4 terminal checkpoint。
Verifier 要求原錄製的每個 checkpoint 完全對應，僅允許這一個 terminal，且 event count 等於 recording stop。
接著兩次 replay 的全部 87 個 canonical checkpoints（含 6 個 terminal）及 saved files 必須一致。
使用既有 canonical masks／segment normalization，沒有為 HD scroll 增加 section 排除或修改原 goldens。
這裡的 saved-file 比對不等於已完成 P8 的 modern save/load cycle。

## 可重跑測試

不需遊戲 assets；最後一項會開窗並切換 fullscreen：

```sh
python3 tools/i18n/fetch_font.py
python3 tools/i18n/test_scroll_text.py
python3 tools/i18n/test_scroll_text.py --sanitize
python3 tools/i18n/test_scroll_text.py --render
python3 tools/i18n/test_scroll_text.py --render --sanitize
python3 tools/i18n/test_scroll_text.py --window --sanitize
```

Native game fixtures／replay helper 的實際執行版本保存在 evidence，需自己的原版 data 與已驗證的 save stage。
不散布原版 assets、exe、font binary、saved games 或本機生成的 recordings。

## CI / Evidence / Baseline

實作 commit `bb7391a83d12373c8318f2ade16ed47f0859fa4b`；MORE 目前行修正與測試
`4e0f622da7f4ee3aa159cfd8e5b15909fcacc41c`。兩者的四個 CI jobs 均 success。
最終程式碼 [run 39713029689](https://github.com/pmanyeh/underworld-exhumed/actions/runs/39713029689)：
Linux／macOS Intel／Windows compile/link、font／cursor／MOUSE／scroll flow 與 real FreeType scroll renderer tests 通過；
Web compile/link 通過，保留無 FreeType fallback，不宣稱 Web HD scroll runtime 已驗證。

Linux/macOS game-only port-check 各 46 個既有 warnings、0 errors；Windows 0/0。
新 standalone scroll tests 使用 `-Wall -Wextra -Werror`。
Linux/macOS 沒有原版 assets，未執行完整遊戲或真實 OS window；實體跨螢幕 DPI 拖移也未實測。
HiDPI coverage 是 synthetic output/window geometry；Windows 實際 resize／fullscreen 已實測。

[manifest](evidence/p7/manifest.json) 保存 source／exe／image hashes、test scope 與 baseline；
[CI metadata](evidence/p7/ci.json)、logs、game／replay reports 與 helpers 也保留。
畫面：[Bragit 開場](evidence/p7/bragit-menu.png)、[點選後與 fallback](evidence/p7/bragit-click.png)、
[長文 MORE](evidence/p7/scroll-pages.png)、[fractional SDL fixture](evidence/p7/resize.png)。

| Phase 開始 baseline | SHA |
| --- | --- |
| Underworld upstream/main | `e5deadd6945b492636aadccc14f52caa908b4186` |
| Underworld feature / P6 tag | `ef28ab992e089fa496e325c22bd8a4ca36af6e4a` |
| Exhume upstream/master | `c54e00e9176c14d0913b5806318a30f85a8f4636` |
| Exhume feature / submodule pin | `89278495bb0b8b224b5bb58e53da2fd367c0d8c0` |

本階段沒有修改 Exhume。

## Phase boundary sync

驗收 evidence checkpoint：`b13b7c5173a0b197b24704da29d14967150fc59e`。
主 repository 與 Exhume working trees 都 clean 後，執行兩邊 `fetch --all --prune`。

| 檢查 | 結果 |
| --- | --- |
| Underworld main / origin/main / upstream/main | 全部 `e5deadd6945b492636aadccc14f52caa908b4186`，main vs upstream `0/0`，沒有更新 |
| Underworld feature vs origin（結案 push 前） | `1/0`，唯一未 push 的提交為上述 evidence checkpoint |
| Exhume main / origin/main | 仍為 `c54e00e9176c14d0913b5806318a30f85a8f4636` |
| Exhume upstream/master | 新增至 `aa2353a2442c9cb9ef2267aabd9398abb8aac42d`；main vs upstream `0/4` |
| Exhume feature / origin feature / submodule pin | 全部 `89278495bb0b8b224b5bb58e53da2fd367c0d8c0`，feature vs origin `0/0` |
| Exhume feature vs upstream/master | `1/11`：原 7 個 backlog，加上本次 4 個新提交 |

Underworld **UPSTREAM_SYNC = NO CHANGE**。
Exhume **UPSTREAM_SYNC = DEFERRED（非 UW1 Native 更新）**。
新提交為 `f25ecbf`（ARMCC5 objplace）、`f599f98`（fnscore）、`419a4f2`（ARMCC5 docs）與 merge `aa2353a`。
分類為 A（文件）／I（其他 compiler profile tools/tests），13 個檔案均位於 README、`profiles/armcc5/` 與相關 skill 文件；
沒有變更 `runtime/`、共用 `tools/`、Native build、renderer、mouse、replay 或 localization。
因此本輪只記錄差異，延續原有 tooling/profile/documentation backlog，沒有 merge/rebase、更新 fork main 或改動 pin。
既有 P7 程式與驗收結果仍對應同一組 baseline；不宣稱已驗證或整合最新 Exhume upstream。

詳見 [sync.json](evidence/p7/sync.json)。`zh-tw-native-p7` annotated tag 指向本輪結案提交，
與 `feature/zh-tw-native` 一同 push。尚未開始 Phase 8。
