# Phase 6 — Native HD Cursor Overlay

**PASS — Native Acceptance Gate P6，2026-10-21。**
本階段把原版游標 bitmap 畫到 Native RGBA layer，置於 HD 文字上方。
預設仍使用原相容游標；以 `--hd-cursor` 選用新路徑。驗收沿用
[Native-only 規則](VALIDATION.md)，未執行 DOS／Borland 或更新 goldens。

## 使用方式

在 `uw1/` 建置後，以自己的 UW1 data／獨立 home 啟動：

```sh
make port
build/port/uw1port --data "$UW1_DATA" --home "$UW1PORT_HOME" --hd-cursor
```

Windows 使用 `uw1port.exe`。游標本身不依賴 FreeType 或外部新素材。
可同時加入 `--test-hd-text --font tools/fonts/NotoSansCJKtc-Regular.otf` 觀察疊放順序。
`--cursor-log FILE` 會記錄 JSONL snapshot，供測試診斷；一般遊玩不需開啟。
`--screenshot` 仍是原低解析 scanout，`--window-shot` 才包含 HD 文字與新游標。

PoC 階段曾明確禁止 `--hd-cursor` 搭配啟用中的 `mouse-look` enhancement（輸出診斷並 exit 1）。
P12（2026-10-24）解除：在設定畫面開啟滑鼠視角會讓遊戲無法啟動。滑鼠視角把遊戲的游標固定在 3D 視窗中央當準星，
走的是 HD 游標接手的同一條繪製路徑，所以準星就是 HD 游標；實測開啟、轉動視角、關閉都正常。
原相容模式的 mouse-look 沒有修改，並通過既有 enhancements regression。

## 實作與邊界

[MOUSE.C](../../src/ui/MOUSE.C) 僅在 Native 分支增加 raster routing 與 hide notification。
保留原來 `checkMouse`、`set_mouse_data`、force／unforce stack、show／hide nesting、
save-under、constraint、drag threshold、keyboard warp、mouse-look 及 input dispatch 邏輯。
直接 screen 與 3D framebuffer 的兩條 raster path 都在 HD 模式略過原圖形繪製，避免把游標
永久烘進 presentation image；原 save／restore 呼叫仍執行。未選用時仍呼叫原函式。

game thread 透過原 `grs_which1`、`seg009_1`、`grs_unpack` 解碼 bitmap；在 decoder 暫存被
覆寫前將 indexed pixels 複製進 [HdCursor](../../../exhume/runtime/port/presentation/hd_cursor.h)。
Snapshot 包含 visible、picture、logical x/y、hotspot、width/height、mode、path 與 generation。
logical Y 為 top-down `199 - mouse_y`；hotspot 沿用原版 width/2−1、height/2。
Header 明確保存 struct packing，避免 game 的 pack(1) 改變 Native snapshot ABI。

單一 game producer／presentation reader 以 C11 acquire/release atomic lock 交換 owned copy，
不跨執行緒保留 EMS、Bitmap、decoder 或其他 game pointers。
最大 255×255，與原 Bitmap 的 byte dimensions 相符；index 0 透明。
現有所有 cursor/object 圖均由此 EMS path 取得。若未來遇到 VRAM slot，會隱藏 overlay 並回到原 raster；
不宣稱未知 VRAM cursor 能蓋過 HD 文字。本輪公開 sessions 未遇到這個 fallback。

[presentation.c](../../src/port/presentation/presentation.c) 每次 scanout 讀取 snapshot 與當前 palette；
由 presentation thread 將 6-bit palette 轉為 RGBA，再依 output pixel centre 最近鄰取樣。
X/Y 分別套用 generic float viewport，clip 至 logical game area，支援 letterbox 與非整數縮放。
Composite 順序是 game → HD text／geometry → cursor → settings。Settings 開啟時不畫 game cursor，
由既有 settings 的 OS pointer 處理，避免雙游標。沒有重畫、AI upscale 或更改原版素材。

## 本輪驗證

| 項目 | 結果 |
| --- | --- |
| Windows Native | 89 game C + 100 port C compile/link；既有 mt32roms unused-variable warning |
| port-check | 89 game sources，0 errors / 0 warnings |
| Core pixel checks | 1×、3× aspect、fractional letterbox、downscale/offscreen、synthetic HiDPI 共 5 組逐像素 reference |
| Snapshot concurrency | 50,000 次 publication，檢查 metadata／pixels 一致、owned copy、generation、invalid dimensions |
| Memory / alpha | guard bytes、pitch padding、透明 pixels 保留背景、opaque cursor 覆蓋既有 RGBA／HD layer；ASan + UBSan PASS |
| 真正 MOUSE.C fixture | 39 assertions；nesting、save-under、regions、3 層 force／第 4 層忽略／underflow、object cursor、constraint／drag、兩條 raster path |
| MOUSE.C memory | fixture 與 cursor core 分別用真實 game／Native compile flags，ASan + UBSan PASS |
| 實際 SDL compositor | 8 scenarios；320×200、960×720、resize、aspect on/off、fractional letterbox、fullscreen／return |
| Click alignment | 將可見 hotspot 經 renderer→window 轉換，送入真實 SDL event queue，再 assert logical click = (80,60) |
| Layer / hide | 上述 8 scenarios 均 assert cursor > 真實 FreeType HD text，settings > cursor，hide 後不留殘影 |
| Actual UW1 | 960×720、640×400、2880×1800 與 960×720 compatibility screenshots；目視確認原游標在中文面板上方 |
| Compatibility replay | 英文與 zh-TW lookup 各 16 sessions／1,786 checkpoints，所有原 sections 與 saves 均與 committed goldens 相同 |
| HD replay | 16 public sessions／1,786 checkpoints；全部非 presentation sections、checkpoint headers、saves 與 committed goldens 相同 |
| HD cursor coverage | 實際 object IDs、map pen／eraser、region cursors、forced cursors；screen 與 framebuffer 兩條 path |
| Existing text tests | layout 29、runtime 16、inventory 22、FreeType 4,411,580 assertions PASS |
| Native string probe | 10,984 strings + 2 missing keys、block 0、dynamic strings、lookup／ring lifetime PASS |
| Native regressions | enhancements 120/120、settings 72/72、movement 15/15、blackbox 22/22、map notes 4/4 |
| Graphics fuzz | 34 routines／5,680 cases，0 differ；僅 Native host 與既有 machine-code oracle，未執行 DOS |

Core assertions 總數隨 concurrent reader 的排程變動，不能解讀為數百萬組不同情境。
Windows ASan 不含 LeakSanitizer；第三方 SDL／FreeType DLL 未以 sanitizer 重編。

[verify_cursor_replay.py](../../tools/i18n/verify_cursor_replay.py) 明確只排除 `VGA`、`FBUF` 與 `GFX`
三個 presentation sections：移除原 raster 後，video/framebuffer 與 graphics scratch 預期會不同。
其餘 canonical sections（含 Player／Level／RNG／clock counts／palette／CRTC）、headers、存檔 SHA-256
都必須完全相同。另有 3 個 Native-only setup sessions 提供 staging，未假稱它們有 DOS goldens。
相容模式則沒有這三項排除。這個 verifier 不建置 DOS，不改 recording、goldens 或 assertions masks。

不需原版 assets 的測試：

```sh
python3 tools/i18n/test_hd_cursor.py
python3 tools/i18n/test_hd_cursor.py --sanitize
python3 tools/i18n/test_mouse_cursor.py
python3 tools/i18n/test_mouse_cursor.py --sanitize
python3 tools/i18n/fetch_font.py
python3 tools/i18n/test_hd_cursor.py --window --sanitize
```

Window test 需要 SDL3、FreeType、下載的字型與桌面；會短暫切換 fullscreen。
三個 Native CI jobs 都執行 standalone cursor core 與真正 MOUSE.C fixture；CI 不含遊戲 assets。
Windows 真實視窗已測試；Linux/macOS 的實際 OS window／fullscreen 與實體多螢幕 DPI 拖移尚未實測。
HiDPI coverage 為 synthetic output/window geometry，未宣稱跨實體 DPI 螢幕驗證。

本機初次 fuzz relink 失敗，是舊的 ignored Native config 缺少 P5 新增的 FreeType link entry；
更新該 local config 後，原 fuzz assertions 全數通過，保留初次失敗與 rerun logs。
另一次建置碰到 regression process 正在使用 exe，Windows 拒絕覆寫；結束該輪後原命令重跑成功。

## Evidence / Git

實作與測試 commit：`72da1625d223141b46ee68f4f4c8687ba81d4b78`，已 push。
[CI run 39629282507](https://github.com/pmanyeh/underworld-exhumed/actions/runs/39629282507)
四個 jobs 全部 success：Linux／macOS Intel／Windows 的 Native compile/link、FreeType test、
cursor pixel／concurrency test 與 MOUSE.C fixture；Web 成功 compile/link。
Linux/macOS game-only port-check 各有 46 個既有 warnings、0 errors，Windows 為 0/0。
新 cursor core test 使用 `-Wall -Wextra -Werror`；MOUSE fixture 保留 game compile flags。
不能將 CI success 解讀為所有原 game C 無 warning，也不代表 Linux/macOS 實際遊戲已啟動。

完整 [manifest](evidence/p6/manifest.json) 記錄 baseline、source／exe hashes 與測試範圍。
選錄的 CI logs 與 [job metadata](evidence/p6/ci.json)、replay reports、原失敗及重跑 logs 均保留。
實際視窗：[windowed](evidence/p6/hd-window.png)、[fullscreen](evidence/p6/hd-fullscreen.png)、
[compatibility](evidence/p6/compat-window.png)；另有不需 game assets 的
[fractional resize fixture](evidence/p6/resize.png)。

本輪起始 baseline：

| 項目 | SHA |
| --- | --- |
| Underworld upstream/main | `e5deadd6945b492636aadccc14f52caa908b4186` |
| Underworld feature / P5 tag | `7132c6f731c3aea01c00d900f1d691efe0bb4e5b` |
| Exhume upstream/master | `c54e00e9176c14d0913b5806318a30f85a8f4636` |
| Exhume feature / submodule pin | `89278495bb0b8b224b5bb58e53da2fd367c0d8c0` |

本階段未修改 Exhume；cursor policy、state bridge 與 tests 都在 UW1。
尚未開始 Phase 7，conversation／scroll 仍使用原英文顯示路徑。

## Phase boundary sync

驗收 evidence checkpoint：`673d9ac5583da682672fa59b6dd7f56a56f00a21`。
該提交後，主 repository 與 Exhume working trees 都 clean，才執行兩邊 `fetch --all --prune`。

| 檢查 | 結果 |
| --- | --- |
| Underworld main / origin/main / upstream/main | 全部 `e5deadd6945b492636aadccc14f52caa908b4186`，main vs upstream `0/0` |
| Underworld feature vs origin（結案 push 前） | `1/0`，未 push 的唯一提交為上述 evidence checkpoint |
| Exhume main / origin/main / upstream/master | 全部 `c54e00e9176c14d0913b5806318a30f85a8f4636`，`0/0` |
| Exhume feature / origin feature / submodule pin | 全部 `89278495bb0b8b224b5bb58e53da2fd367c0d8c0`，`0/0` |
| Exhume feature vs upstream/master | `1/7`，沿用 P4 的 overlay commit 與既有 tooling/profile/documentation backlog |

**UPSTREAM_SYNC = NO CHANGE。** 沒有新上游變更，也沒有 merge/rebase 或改動 submodule pin。
詳見 [sync.json](evidence/p6/sync.json)。`zh-tw-native-p6` annotated tag 指向本輪結案提交，
與 `feature/zh-tw-native` 一同 push。Phase 6 到此結束。
