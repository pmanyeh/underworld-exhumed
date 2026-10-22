# Native localization / layout / HD text / cursor / scroll (Phases 1–9)

**Phase 9 起，翻譯來源是 [zh-TW/master.tsv](zh-TW/master.tsv)（加上 hard-coded 的
[zh-TW/literals.tsv](zh-TW/literals.tsv)）**；PoC 的 `poc.tsv` 已無損遷移後移除。
譯者規則見 [zh-TW/README.md](zh-TW/README.md)，驗收見
[Phase 9 report](../docs/i18n/LOCALIZATION-PRODUCTION.md)。

## Localization production workflow (Phase 9)

工具為 standard-library 的 [l10n.py](../tools/i18n/l10n.py)。在 `uw1/`，`$UW1_DATA` 是玩家自己的合法資料：

```sh
D="--pak $UW1_DATA/DATA/STRINGS.PAK --cnv $UW1_DATA/DATA/CNV.ARK"
python3 tools/i18n/l10n.py inventory $D           # 重新盤點並與 committed fingerprint 比較
python3 tools/i18n/l10n.py export $D --untranslated --category conversation   # 譯者工作表（含英文，僅留本機）
python3 tools/i18n/l10n.py import $D build/i18n/p9/workbook.tsv               # 合併回 master.tsv
python3 tools/i18n/l10n.py validate $D            # UTF-8／key／source drift／控制碼／模板
python3 tools/i18n/l10n.py build $D               # deterministic build/i18n/zh-TW.uwloc
build/port/uw1port --data "$UW1_DATA" --home "$UW1PORT_HOME" --locale zh-TW \
  --i18n-file build/i18n/zh-TW.uwloc --hd-scroll --font tools/fonts/NotoSansCJKtc-Regular.otf \
  --i18n-trace build/i18n/p9/play.jsonl
python3 tools/i18n/l10n.py coverage $D --trace build/i18n/p9/play.jsonl   # coverage.json／coverage.md
python3 tools/i18n/l10n.py export $D --trace build/i18n/p9/play.jsonl --runtime --untranslated
```

也可直接編輯 `master.tsv` 後執行 `normalize`／`validate`。`build` 先完整驗證，任何
source drift、重複 key、非法 UTF-8 或格式錯誤都拒絕產生 catalog；同一輸入永遠得到相同 bytes 與 SHA-256。
`--i18n-trace` 在任何 locale 都記錄每次 lookup：`hit`、`template`（已譯但仍以英文 BABL 展開顯示）、
`miss`、`drift`、`invalid`、`dynamic`；coverage／export 會彙總同一 key 的重複次數。
`export` 與 `inventory.tsv` 含原版英文，只放在 ignored `build/`；committed 的只有翻譯資料
（master 只存 `source_hash`，不存英文）、不含文字的 [inventory fingerprint](inventory-baseline.json) 與計數報告。
Coverage 的完成率只計 **production scope**（排除 empty、unaddressable 與 block 24 debug report），
另列 all-inventory 數字；分類規則見 [zh-TW/README.md](zh-TW/README.md) 與
[P9 review fixup](../docs/i18n/LOCALIZATION-PRODUCTION.md#p9-review-fixup--pre-p10-gate)。

目前已完成文字盤點、翻譯資料、Native UTF-8 lookup 與獨立的文字排版核心。
預設保留原英文 renderer；`--hd-scroll` 可選用繁中 scroll／conversation 顯示。
Phase 4 已加入選用的通用 RGBA overlay；Phase 5 加入 FreeType 高解析度繁中示範。
Phase 6 加入原版 bitmap 的 Native 游標 overlay；驗收見 [HD cursor](../docs/i18n/HD-CURSOR.md)。
Phase 7 將 UTF-8 換行、控制碼、分頁與對話選項接入實際遊戲；驗收見
[Chinese scroll integration](../docs/i18n/CHINESE-SCROLL.md)。
Phase 8 已完成實際 Bragit 多輪繁中流程、存讀檔與跨平台建置，技術驗收 **PoC = GO**；
完整結果與限制見 [PoC report](../docs/i18n/POC-REPORT.md)。
Phase 9 建立 localization production pipeline，停在 P9 等待人工審閱，尚未開始 P10 大量翻譯。
驗收見 [Phase 1](../docs/i18n/TEXT-INVENTORY.md)、[Phase 2](../docs/i18n/RUNTIME-LOOKUP.md)
與 [Phase 3](../docs/i18n/LAYOUT-CORE.md)、[Phase 4](../docs/i18n/OVERLAY-FOUNDATION.md)。

目前依使用者更新採 [Native-only 驗收](../docs/i18n/VALIDATION.md)：不需要 Borland 或 DOS 驗證。
先前報告的 DOS NOT RUN 是歷史紀錄，不是待解決的阻礙。

## Chinese conversation / scroll

在 `uw1/` 建置 catalog、下載字型、啟動 Native executable：

```sh
python3 tools/i18n/l10n.py build --pak "$UW1_DATA/DATA/STRINGS.PAK"
python3 tools/i18n/fetch_font.py
make port
build/port/uw1port --data "$UW1_DATA" --home "$UW1PORT_HOME" \
  --locale zh-TW --i18n-file build/i18n/zh-TW.uwloc --hd-scroll \
  --font tools/fonts/NotoSansCJKtc-Regular.otf
```

Windows 使用 `uw1port.exe`；font／catalog 也可使用絕對路徑。
HD scroll 會同時啟用 HD cursor，需要停用 mouse-look；不會自動修改已存的 enhancement 設定。
`--scroll-log FILE` 可記錄實際顯示文字、pause、MORE 與選項事件。
缺譯文、dynamic string、含 `@` 的模板，保留英文 fallback；模板先由原 BABL 展開。
這階段沒有把 UTF-8 寫進 VM、game string ring 或原 byte buffers，也沒有實作中文輸入／IME。
不加 `--hd-scroll` 時，`--locale zh-TW` 仍只做 lookup／trace，維持原 renderer。

## FreeType HD text

先安裝 FreeType development package（pkg-config 名稱 `freetype2`）：
Linux `libfreetype6-dev`、macOS `brew install freetype`、MSYS2 CLANG64
`pacman -S mingw-w64-clang-x86_64-freetype`。再在 `uw1/`：

```sh
python3 tools/i18n/fetch_font.py
make port
build/port/uw1port --data "$UW1_DATA" --home "$UW1PORT_HOME" \
  --test-hd-text --font tools/fonts/NotoSansCJKtc-Regular.otf
```

Windows 使用 `uw1port.exe`，MSYS2 可用 `make PY=python port`。
font path 相對目前工作目錄；可傳入絕對路徑。預設不啟用 HD text，也不搜尋 OS fonts。
設定畫面關閉後可見三句指定的中文／中英混合文字；F11 Display 可調整大小與 fullscreen。
`--test-overlay` 可同時啟用 Phase 4 幾何框。

字型為 Noto Sans CJK TC Regular 2.004，原檔不修改、依 SHA-256 驗證；字型與 OFL 1.1
授權下載到 ignored `tools/fonts/`，不提交 16 MB font binary。沒有 FreeType 的 build 可繼續使用
原英文路徑；明示要求 HD text 時會診斷失敗，不會悄悄拿低解析 bitmap 代替。
Web 保留 no-FreeType stub；web target 的 pkg-config 只搜尋 web libraries，避免誤連結 host FreeType。

不需遊戲 assets 的實際 C renderer 測試：

```sh
python3 tools/i18n/test_hd_font.py
python3 tools/i18n/test_hd_font.py --sanitize
python3 tools/i18n/test_hd_font.py --window
```

測試需已下載字型；`--window` 額外需要 SDL3，會短暫開窗並切換 fullscreen。
API、cache、字型授權與本階段驗收見 [Phase 5 report](../docs/i18n/HD-TEXT.md)。

## Generic HD Overlay Foundation

Exhume 的 `PlatHooks.overlay` 在 presentation thread 提供清為透明的 native-output RGBA buffer，
以及與遊戲相同的 viewport／logical dimensions。RGBA bytes 採 straight alpha；settings layer 在它上方。
未指定 callback 時不配置 overlay，hidden 模式不呼叫；resize／fullscreen／output DPI 尺寸變更會重建資源。
契約詳見 Exhume 的 `docs/presentation-overlay.md` 與 `runtime/port/platform/plat.h`。

UW1 預設關閉。使用原有 Native launcher 或 executable 加上 `--test-overlay`：

```sh
build/port/uw1port --data "$UW1_DATA" --home "$UW1PORT_HOME" --test-overlay
```

關閉啟動時的設定畫面後，可見半透明藍色塊、黃色 logical reference box 與一個 output pixel 寬的白色準星。
F11 設定畫面會覆蓋示範；可在 Display 頁切換 aspect／integer scaling／fullscreen。
示範只讀 callback metadata，不讀 game-thread mutable state，也不繪製文字或更換遊戲游標。

通用 backend pixel tests（在 Exhume 根目錄、需 SDL3 compiler 環境，無需遊戲 assets）：

```sh
python3 tools/overlaycheck.py
python3 tools/overlaycheck.py --sanitize --output build/overlaycheck-asan
```

## UTF-8 / Chinese Layout Core

Native C API 位於 [utf8.h](../src/port/i18n/utf8.h) 與 [layout.h](../src/port/i18n/layout.h)，
無 SDL / FreeType dependency。`utf8_next/prev/validate/count/boundary_before` 使用明示長度與 byte offset，
嚴格拒絕 malformed / truncated UTF-8。`layout_text` 將來源轉成 scalar、newline 與原子控制碼 units，
以 caller 的整數 pixel/fixed-point advance callback 換行；輸出保留每個 unit 的原始 byte span。

```c
/* Callback may use an actual font in a later phase; same units as max_width. */
static int sample_width(uint32_t cp, void *context) {
    (void)context;
    return cp < 128 ? 1 : 2; /* Synthetic metrics, not a font. */
}
struct text_layout text = {0};
size_t error_byte;
if (layout_text(utf8_text, byte_length, 32, sample_width, NULL,
                &text, &error_byte) == LAYOUT_OK) {
    /* Consume lines/units; skip advancing TRIMMED spaces, process controls
       separately. LAYOUT_OVERFLOW is explicit; never silently cut bytes. */
    layout_free(&text);
}
```

ASCII 優先在空白換行，長單字依 codepoint fallback；中文採字間換行與基本行首／行尾禁則。
禁則無法與窄欄寬並存時，保留完整字元並標記 `LAYOUT_OVERFLOW`。
`\p`、`\P`、`\m`、`\0..\6` 是零寬、兩個 bytes 的 control units，核心不執行它們。
ASCII edge spaces/tabs 保留為 `LAYOUT_TRIMMED` units；CR/LF/CRLF 保留明示換行，末尾 newline 產生空行。
完整 API/error/lifetime 與政策界線見 Phase 3 報告；目前不處理完整 grapheme shaping 或 kerning。

在 Native compiler 環境、不需遊戲 assets、SDL 或 FreeType：

```sh
python3 tools/i18n/test_layout.py -v
python3 tools/i18n/test_runtime.py -v
python3 -m unittest discover -s tools/i18n -p test_i18n.py -v
python3 tools/i18n/verify_layout_sanitizers.py
```

Sanitizer driver 需要 Clang 的 ASan / UBSan runtime；一般 core tests 只需 C11 compiler。
本機已通過兩者。`localization.c` 現在共用相同的 UTF-8 validator，另保留 catalog 的 NUL 拒絕規則。

## Run the Phase 2 lookup

在 `uw1/`，以原有 Native compiler 環境建置，再編譯譯表與執行遊戲：

```sh
make port
python3 tools/i18n/pack_translations.py --pak "$UW1_DATA/DATA/STRINGS.PAK" build/i18n/zh-TW.uwloc
build/port/uw1port --data "$UW1_DATA" --home "$UW1PORT_HOME" \
  --locale zh-TW --i18n-file build/i18n/zh-TW.uwloc --i18n-log build/i18n/hits.jsonl
```

Windows executable 名稱為 `uw1port.exe`。`--locale en` 為預設，不讀取譯表。
缺少／損毀的 catalog 會發出診斷並使用英文。未命中、source drift、unresolved block 0 與
dynamic keys 皆回傳 caller 原本的 fallback pointer；不複製或修改它。
`--i18n-log` 每個實際命中的 key 記錄一次 UTF-8 JSONL，包含 resolved block、index 與完整譯文。

`get_string` 的 Native hook 在查找 made blocks 之後解析 block 0、解出原文並呼叫 observer；
observer 取得真正的 UTF-8 translation，但回傳原始可修改英文 pointer。
這是計劃允許的 logging 驗證路徑：直接替換會把 UTF-8 送進會修改字串的文法函式、
固定容量 buffer 與低解析 renderer，因此本階段不在 legacy path 直接回傳譯文。
新 API 的 caller 可直接使用 `port_i18n_lookup` 回傳的 UTF-8；未來 presentation integration
需在適當邊界使用它，不能直接把這個 const pointer 當成原版 mutable buffer。

兩個參數的 `port_i18n_init(locale, catalog_path)` 明確指定資源位置，避免依賴工作目錄或
程序內的隱藏環境變數。catalog 在 game thread 啟動時載入，init/shutdown/trace 僅由該 thread 操作，
lookup 本身只讀取資料。平台 loop 可能在 game thread 仍存活時返回，所以主執行緒不釋放 catalog；
正常 game wrapper 返回才 shutdown，parked exits 由程序結束回收。沒有新增 live reload 或跨 thread 讀寫。

譯文獨立儲存並保持 immutable，直到 shutdown；再多次讀取原版 string ring 也不會覆寫它。
呼叫 shutdown 前須確保所有讀者已停止。原版八個 512-byte slots、動態 pointer、control/template
展開與物品命名行為維持原樣。

### Catalog contract

可編輯來源仍為 UTF-8 TSV。`.uwloc` 是可重建的 build artifact，不能手動編輯或提交。
格式使用固定 little-endian，不序列化任何 C struct / pointer：

```text
8 bytes magic: UW1LOC1 + NUL
uint32 entry count
per entry, sorted by (block,index):
  uint16 block, uint16 index, uint32 source_bytes, uint32 target_bytes
  CP437 source bytes + NUL
  UTF-8 target bytes + NUL
```

每筆原文最多 511 bytes、譯文最多 16,384 bytes；總數最多 32,768 筆、整檔最多 16 MiB
（Phase 9 由 4,096 筆／4 MiB 放寬，足以容納全遊戲 10,984 個 entries）。
PoC 資料 pack 為 52 static entries / 8,990 bytes；literal 保留到相應 UI 階段，不進 catalog。
只有 `draft`／`reviewed`／`approved` 且有譯文的列會 pack；`l10n.py build --min-status reviewed` 可做較嚴格的 build。
loader 先檢查整檔，拒絕截斷、重複／未排序 key、額外尾端資料、非法 key、NUL 與損毀 UTF-8，
通過後才公開整份 catalog。UTF-8 format check 是 loader 的局部輸入驗證，不是 Phase 3 layout API。

### Verify Phase 2

```sh
python3 tools/i18n/test_runtime.py -v
python3 tools/i18n/verify_native.py "$UW1_EXE" "$UW1_DATA/DATA/STRINGS.PAK" \
  --catalog build/i18n/zh-TW.uwloc --report build/i18n/native-comparison.json
python3 tools/i18n/verify_localized_replay.py --config exhume.toml \
  --catalog build/i18n/zh-TW.uwloc --master i18n/zh-TW/master.tsv --report build/i18n/replay-zh-TW.json
```

第一項使用真實 C module 建立 shared library，不需 SDL 或原版遊戲資料。
最後一項使用 upstream 原本的 replay verifier 加上 locale CLI options；沒有更動 inputs 或 goldens。
Windows 本機回歸使用 Phase 0 的 `build/poc/native-v22.toml`，處理沒有 symlink 權限的測試資料路徑。
此 PoC wrapper 不嘗試 DOS rebuild，直接以已提交 goldens 驗證 Native 結果。

## Reproduce with your own game data

Python 3.10+ standard library 即可執行盤點與驗證。在 `uw1/` 執行：

```sh
python3 tools/i18n/extract_strings.py "$UW1_DATA/DATA/STRINGS.PAK" build/i18n/strings.tsv \
  --cnv "$UW1_DATA/DATA/CNV.ARK" --metadata build/i18n/metadata.json
python3 tools/i18n/scan_literals.py build/i18n/literals.tsv
python3 tools/i18n/validate_poc.py "$UW1_DATA/DATA/STRINGS.PAK" "$UW1_DATA/DATA/CNV.ARK"
python3 -m unittest discover -s tools/i18n -p test_i18n.py -v
```

Windows 用原有 MSYS2 CLANG64 / Python 環境；本機路徑與 libraries 設定沿用
[Phase 0](../docs/i18n/PREFLIGHT.md)。原版資料位於玩家指定的位置，不會由工具下載。
完整盤點輸出放在已忽略的 `build/i18n/`，不提交原版資料或完整英文文字庫。

Native C 交叉驗證需先成功 `make port`，沿用相同 compiler、DLL/library 環境：

```sh
python3 tools/i18n/verify_native.py "$UW1_EXE" "$UW1_DATA/DATA/STRINGS.PAK" \
  --report build/i18n/native-comparison.json
```

這會連結原有 game/runtime objects 與小型 test driver，直接呼叫未修改的
`GAMESTRN.C` decoder。程式不初始化遊戲畫面、不修改遊戲來源；對每個 PAK entry
比較 C 與 Python 輸出的 bytes，另驗證 block 0 與兩個 dynamic block。
這個交叉驗證 driver 針對本 PoC 的 UW1 GOG baseline，並要求 Bragit 的文字存在。

## TSV contract

UTF-8、無 BOM、LF 換行。先使用標準 TSV/CSV reader（delimiter 為 tab）讀欄位，
再逐欄呼叫 `extract_strings.unescape()`。引號遵循 CSV quoting，不能以單純 `split('\t')`
取代 reader。欄位內的 `\\`、`\n`、`\r`、`\t`、`\0` 分別代表 literal backslash、
newline、CR、tab、NUL。其他反斜線 escape 拒絕接受；不得用 `eval` 或通用 Python escape decoder。

例如遊戲的反斜線加 `m` 在 TSV 中寫成 `\\m`；真正換行寫成 `\n`。
兩者不可混淆。原版 bytes 以 CP437 可逆映射至 Unicode；這次 10,984 筆實際輸出皆為 ASCII。
解碼結果遵循遊戲 C string 語意：第一個 NUL 結束、最多 511 bytes，不是原始壓縮 bitstream 的備份。

`strings.tsv` 欄位為 `block, index, category, source_en`，block/index 使用十進位。
穩定排序為 block、index；保留空字串及所有磁碟 entry，包括超出 raw ID 的 41 個尾端 entries。
後者屬於盤點，不能直接當作 runtime `get_string` key。

兩個翻譯檔使用相同的 TSV 規則（`l10n.py` 另外拒絕 BOM、CR 與非 canonical 排版）：

| 檔案 | 欄位 | identity |
| --- | --- | --- |
| `zh-TW/master.tsv` | `block, index, category, source_hash, target_zh_TW, status, context, notes, flags` | resolved `(block,index)`，例如 `3651`／`3` |
| `zh-TW/literals.tsv` | `key, source, target_zh_TW, status, context, notes, flags` | `literal:<repo path>:<call>:<text SHA256 前16碼>:<occurrence>` |

Literal identity 不依賴行號；path、call、內容或同值出現次序改動會使 key 改變，需重新審查。
master 的 `source_hash` 是 `SHA-256(遊戲解出的 CP437 原文 bytes)`，只用於 drift check，
**不提交英文原文**；catalog 需要的英文 bytes 在 build 時從玩家的 STRINGS.PAK 取得並逐筆核對 hash。
literals 的 `source` 是本 repository C 程式碼中的字串，不屬於遊戲資料，仍保留原文。
欄位語意、status 與 flags 見 [zh-TW/README.md](zh-TW/README.md)。
P9 fixup 前的 master 曾含 `source` 英文欄；新 schema 由目前的 master 開始，歷史提交與 evidence 不改寫。
PoC 的 `poc.tsv`（`kind, block, index, source_key, role, source_en, zh_tw, note`）已在 Phase 9 遷移：
`role` 成為 `context` 的 `PoC role: …`，`note` 成為 `notes`，狀態一律 `draft`。

## Runtime identity and lifetime

以 [GAMESTRN.C](../src/ui/GAMESTRN.C)、[BABL.C](../src/conv/BABL.C) 與
[CONVERSE.C](../src/conv/CONVERSE.C) 為準：

1. `raw id` 先轉 uint16，`block = id >> 9`、`index = id & 0x1ff`。
2. **先找 made/dynamic blocks**。`0x7C` 是對話執行期組成的字串，`0x7D` 是玩家姓名。
   它們繞過 static lookup；不得把使用者輸入／執行期結果當作英文原文翻譯。
3. 若 raw block 為 0，以 `CutsceneOrConversationStringBlock` 解析真正 block；
   對話從 CNV.ARK header 取得，過場由 `CUTS.C` 設定為 `0xC00 + n`。
4. 其餘直接以 `(block,index)` 讀 PAK。對話的 resolved block 常為 `0xE00+...`，
   但必須讀 header，不能假設所有 slot 都符合公式。高 block 不可再塞回 16-bit packed ID。
5. `read_string` 使用八個 512-byte 循環 slots，最多輸出 511 bytes。
   翻譯 lookup 的未來生命週期管理不能改壞原本 buffer contract。

Bragit 的 CNV slot / whoami 為 67，header 指向 `0xE43`（3651），有 1,695 code words；
PAK 共 50 entries，0 為空，1–49 為 template fragment、玩家選項與 NPC 台詞。
姓名在 block 7、index 83。PoC 全數涵蓋這 49 個非空 entries 與姓名；
另收 block 1 的 2、5 及 `SCROLLIO.C` 的 `[MORE]`，共 53 筆。

靜態 template 可以翻譯，但 `@SS1` 等 BABL markers 必須完整保留；它們在顯示前由
`convert_string` 展開。`@GI` 是整數、`@GS` 是字串、`S` 為 local、`P` 為間接 local、
`C` 為常數，`@@` 表示 literal `@`；索引本身可遞迴引用整數變數。
驗證器保留 marker 次數與完整索引、控制碼順序及明示換行數；未知語法直接報錯。
目前沒有實作 dynamic template translation strategy。

原版物品名稱的 `article_singular&plural` 是 `fix_name_string` 的語法，不能當作普通整句替換。
這階段只盤點，不翻譯物品名稱；文法與中文物品命名留待相應階段處理。

## Hard-coded literal triage

掃描 `uw1/src`、固定 pin 的 `exhume/runtime/port` 及本工具的 C driver。
先去除 comments，再處理字元常數、C escapes、相鄰字串與 multiline literal；
保留原始 C spelling、path、行號、最近 call context、classification 與 reason。
全數輸出分為 `PLAYER_VISIBLE / DEBUG_ONLY / PORT_UI / TEST_ONLY / UNKNOWN`。

這是保守的 lexical inventory，沒有執行 preprocessor 或完整 interprocedural data-flow。
`UNKNOWN` 包含 include/path、format、命名陣列與尚待人工追蹤的 UI 文案；
不能將 UNKNOWN 當成 DEBUG_ONLY，也不宣稱已列出每個實際 runtime display event。
未在遊戲目錄外掃描原版 binary 的 literals。

已識別範例包含 `scroll_print` / `string_to_screen` 的直接文案、
`INTERACT.C` 的 `You see ` 片段、角色建立的 Str/Dex/Int/Vit labels、
`SCROLLIO.C` 的 `[MORE]`，以及 native settings 的命名 label arrays、`plat_message` dialogs。
片段須連同後續拼接與文法審查；尚未實作任何 literal runtime replacement。
