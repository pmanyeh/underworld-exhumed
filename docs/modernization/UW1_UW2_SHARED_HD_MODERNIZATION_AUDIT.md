# M0 — UW1/UW2 Shared HD Modernization Audit

**性質：READ-ONLY／DESIGN ONLY。** 本稽核沒有修改任何 renderer、遊戲程式、Exhume 或 build 設定，
只閱讀原始碼、既有文件與既有工具輸出。依 v2.5 計畫 16.3，M0 與 P10 是不同的工作，獨立提交、獨立報告。

| 項目 | 值 |
| --- | --- |
| 日期 | 2026-10-23（§13、§14 同日追加） |
| Underworld Exhumed | `feature/zh-tw-native` `ae2fb9ac9538250d1bbc1bea61befcb6989be4f3` |
| Exhume（submodule pin） | `89278495bb0b8b224b5bb58e53da2fd367c0d8c0` |
| 主要來源 | `uw1/src`、`uw2/src`、`exhume/runtime/port`、`uw1/docs/subsystems/{3d,gfx}.md`、`uw2/docs/subsystems/gfx.md`、`exhume/docs/{port,presentation-overlay}.md` |

## 結論摘要

1. **畫面是固定 320×200 的 mode X。** 遊戲畫進模擬的平面式 VGA 記憶體，`vga_scanout` 每次輸出 320 寬的畫面，
   SDL 只做縮放、aspect、letterbox。HD 文字與游標已經用 Exhume 的 RGBA overlay 疊在上面（輸出解析度、不放大）。
2. **3D renderer 與繪圖函式庫是 `asm2c` 自動產生的 C**（UW1 31 個檔、約 91,000 行；UW2 25 個檔、約 56,000 行），
   以模擬的 x86 暫存器逐指令執行，檔頭明言不得手改；原版組合語言又受 DOS byte-match 保護。
   **因此不可能「在原地把 renderer 參數化」。**
3. **最好的切入點是 render database。** 遊戲 C（`VIEW3D.C`、`GRIDDB.C`、`DRAWOBJ.C`、`GAMESORT.C`）每個 frame
   產生一段 bytecode，C 寫入的 opcode 約 20 個且已有文件；`cRender` 再執行它。建議 M3 寫一個新的原生 HD renderer
   直接解讀同一份 database，畫到 overlay；原本的 renderer 照常執行，負責 pick frame 與回歸比對。
4. **滑鼠與 UI 點擊全在邏輯 320×200 座標。** UW1 全部只有 14 處 `input_addmouse` 註冊點擊矩形；HD UI 只要畫在
   同樣的邏輯矩形上，點擊判定不用改。
5. **加寬視野會改變 gameplay。** 視野格（vision grid）固定 17×33，邊界射線固定在 heading ±0x2040；
   而 `process_grid` 依「首次看見的格數」給經驗值。真寬螢幕若擴大可見範圍，經驗值與 AI 可見性都會不同，
   必須是預設關閉的選項，並另行驗證。
   以 port-only「只畫、不寫自動地圖與經驗值」的第二段處理兩側格子可避開這點（§14）。
6. **HD 路徑已有一處寫回遊戲記憶體。** HD scroll 把排版後的游標寫回 `scroll->cur_x`、`cur_y`、`start_line`
   （P10A F1）。M1／M3 的任何 HD surface 都要遵守「只讀遊戲狀態、不回寫」，否則重演 F1。
7. **UW1 與 UW2 的高層 C 幾乎相同，低層 renderer 不同。** `VIEW3D.C` 相似度 0.96、`GAMESORT.C` 0.93；
   但 UW1 用 `WALLMAP`／`PERSPMAP`／`QUADFIT`，UW2 用 `TEXMAP`／`PERTP`／`PROJPOLY`／`SCANLINE`，
   翻譯後的 port 檔相似度僅 0.01–0.25。共用 HD renderer 應建在 database 層，而非 rasterizer 層。

## 1. 畫面管線（現況）

```text
遊戲 C／翻譯後的組合語言
  → 寫入模擬 VGA（mode X：每個位址 4 個 pixel、4 個 plane，兩頁＋閒置空間）
  → vga_scanout（Exhume runtime/port/gfx/vga.c）：320 寬、200 列（列掃兩次時）
  → SDL3：indexed framebuffer 依 aspect／integer scaling／letterbox 放到 destination
  → RGBA overlay（輸出解析度，straight alpha）：HD 文字、HD 游標
  → settings layer → present
```

- 列表 `Ytab` 由最高列往下建，**y 從畫面底部往上算**；clip window（`WIN_LEFT` 等）同一規則。
- 畫在不顯示的那一頁，`grPageFlip` 交換；video memory 多餘的部分由 `VALLOC` 管理，存放 UI 圖片與 sprite 背景。
- overlay contract（`exhume/docs/presentation-overlay.md`）提供 `logical_width/height` 與 `viewport_x/y/w/h`，
  可把任何邏輯點換算到輸出像素；overlay 不影響 input mapping、scanout、截圖與 replay。**這是 M1／M3 的共用顯示通道。**

## 2. 寫死解析度的位置

以 `320／200／160／100／199／319` 與對應十六進位數字掃描（**上限估計**：`100`、`200` 也常是非畫面數字）：

| | UW1 | UW2 |
| --- | ---: | ---: |
| 總數 | 277 | 275 |
| 集中處 | `ui` 65、`gfx` 42、`game` 27、`3d` 23 | `gfx` 78、`ui` 43、`game` 22、`3d` 18 |
| 前幾名檔案 | `CHARGEN.C` 22、`AUTOMAP.C` 20、`MOUSE.C` 19、`CUTS.C` 13、`VIEW3D.C` 10 | `CUTS.C` 49、`CHARGEN.C` 19、`AUTOMAP.C` 17、`MOUSE.C` 16、`VIEW3D.C` 10 |

另外，翻譯後的組合語言把寬度寫進程式碼本身：例如 frame buffer 複製到螢幕的展開迴圈，由 `setup_frame_buf`
依視窗寬度寫入一個 `ret` 來截斷（`uw1/src/port/gfx/grentry.c`）。這類位置無法以掃描數字找全，是不在原地改解析度的另一個理由。

## 3. 3D view、frame buffer 與投影

| | UW1 | UW2 |
| --- | --- | --- |
| `place_3d_view(x, y, w, h)` | `(52, 180, 172, 113)`（`game/UWEDIT.C:335`） | `(16, 182, 208, 128)`（`game/UWEDIT.C:333`） |
| zoom | `0x7ED2`（一般）、`0x6062`、`0x61A8`（demo） | 同一套邏輯 |

- 3D view 畫進**獨立的 frame buffer**（列寬 `xwid + 2`），`cFBtoScreen` 再複製到 mode X 螢幕；`render_FB` 只畫進 FB（淡入淡出、覆蓋畫面）。
- 投影參數在 renderer 的資料段（seg051）：`PROJ_X`、`PROJ_Y`、`CENTRE_X`、`CENTRE_Y`、視角矩陣 `MAT_*`（1.15 fixed point）、
  點表 `PNT_X`；clip code 依 ±z 判定。這些都由翻譯後的程式讀寫。
- 細節等級 0–3 決定牆／地板／天花板是否貼圖；距離陰影 7 以後退為平塗、4 以後用縮小的貼圖。

## 4. Render database（建議的 M3 切入點）

每個 frame：`setup_vars` → `do_2dclip`（視野格）→ `GRIDDB.C` 的 `process_grid` 寫 bytecode → `send_db` 設 clip window、
呼叫 `cRender` 執行 → `cFBtoScreen`。

- C 寫入的 opcode（`uw1/src/include/view3d.h`）：`OP_MOVEC`、`OP_SFCAL`、`OP_ORG`、`OP_SETCOLOR`、`OP_TMAP`、`OP_OBJ`、
  `OP_UWOBJ`、`OP_FETCHMAP`、`OP_DEFDELTA`、`OP_IHCALL`、`OP_UWCRIT`、`OP_DEFRES`、`OP_POLYRES`、`OP_COMPACT_TMAP`、
  `OP_COMPACT_WTMAP`、`OP_SETBMCOL`、`OP_MOUSEQ`、`OP_SET_TMCTXT`、`OP_SETTMOBJ`、`OP_SET_GMAP_CTXT`。
- **風險：** `OP_SFCAL`／`OP_IHCALL` 會呼叫 renderer 資料段裡的 **3D 模型 bytecode**（門、桌子等），opcode 表共 110 個，
  UW1 自有的 1Eh、20h、24h、28h、30h、52h、6Ah 尚未追蹤（`uw1/docs/subsystems/3d.md` 的 open questions）。
  新 renderer 必須涵蓋模型 bytecode，不只是 C 寫的那 20 個 opcode。
- database 不含螢幕座標，只有世界座標、貼圖與模型呼叫，所以**同一份 database 可以畫到任意解析度**。

## 5. Pick frame

`do_3d_grab` 以「代表物件的顏色」重畫一張 frame（物件 1–0xBF、牆 0xC0＋貼圖、地板 0xF0＋貼圖、天花板 0xFA），
`INTERACT.C` 的 `pick_3d` 讀游標下的顏色（`p += y * (xwid + 2) + x + 2`）。

**建議：** pick frame 保持由原 renderer 在邏輯解析度產生。HD renderer 只負責顯示，點選結果與 replay 因此完全不變。
代價是 HD 畫面與 pick 的邊緣可能差一個邏輯像素；QA 時需確認可接受。

## 6. Rasterizer 與貼圖

| | UW1 | UW2 |
| --- | --- | --- |
| 原版模組 | `WALLMAP`、`PERSPMAP`、`QUADFIT`、`POLYFILL`、`LPFDELTA`、`NOCLIP`、`CAMERA` | `TEXMAP`、`TEXMAPV`、`PERTP`、`PERTLP`、`PROJPOLY`、`SCANLINE`、`CLIP` |
| port | 全部 `asm2c` 產生（31 檔、約 91,000 行，含繪圖函式庫） | 全部 `asm2c` 產生（25 檔、約 56,000 行） |

繪圖原語把形狀轉成 span，再經 `SPAN_WRITER` 依筆型（實心、XOR、頁間複製等）寫入；Sutherland-Hodgman clipper 會改寫自己的比較指令。
這些行為在新 renderer 中要以一般 C 重寫，而不是沿用翻譯碼。

## 7. 滑鼠

```text
host pointer（SDL）
  → SDL_RenderCoordinatesFromWindow：轉成遊戲 destination 內的座標（含 HiDPI 密度）
  → port 的 mousedrv.c（取代 MOUSEDRV.ASM 與 int 33h）：未鎖定時，算出讓遊戲游標落在 host 指標位置的 mickeys
  → MOUSE.C：遊戲自己的游標，x 0–319、y 0–199（由下往上）
```

HD 游標（P6）已經從遊戲的游標狀態畫在 overlay 上。只要 HD UI 維持邏輯座標，滑鼠路徑不需要改。

## 8. UI 區域與點擊

點擊區由 `INPUT.C` 的 `input_addmouse(ulx, uly, lrx, lry, arg, mode_mask, handler)` 註冊，座標為邏輯 320×200、y 由下往上；
游標形狀另由 `MOUSE.C` 的 `defineMouseRegion`（最多 20 個）決定。

UW1 主畫面（`INTERACT.C`、`PLAYER.C`、`INVPANEL.C`、`AUTOMAP.C`）：

| 區域 | 矩形（ulx, uly, lrx, lry） | 來源 |
| --- | --- | --- |
| 指令圖示 | 8, 0x54, 0x20, 0xCE | `INTERACT.C:174` |
| 符文架（施法） | 0xB0, 0x2D, 0xDE, 0x3D | `INTERACT.C:175` |
| 作用中法術 | 0x34, 0x2F, 0x66, 0x3F | `INTERACT.C:176` |
| 資訊（羅盤區） | 0x7A, 0x31, 0x98, 0x40 | `INTERACT.C:177` |
| 生命／法力瓶 | 0xF4, 0x2C, 0x135, 0x50 | `INTERACT.C:178` |
| 移動箭頭 ×3 | 0x6B..0xAA, 0x1F..0x2F | `PLAYER.C:183–185` |
| 3D view | 依 `place_3d_view` | `PLAYER.C:298` |
| 物品欄／紙娃娃 | 0xF0, 0x52, 0x13B, 0xBD | `INVPANEL.C:161` |
| 交易（雙方欄位） | 0x52..0xC1, 0x98..0xBE | `PLAYER.C:282–283` |
| 對話選單 | 0x0F, 1, 0x131, 0x1E | `PLAYER.C:284` |
| 自動地圖 | 全畫面 | `AUTOMAP.C:142` |

UW1 共 14 處呼叫；UW2 也是 14 處，分布在 `PLAYER.C`（7）、`GAMESCR.C`（4）、`ICONS.C`、`INVPANEL.C`、`AUTOMAP.C`。

UI 圖片來自 `.GR` 檔，由 `LOADGR.C` 載入 VRAM pool；文字用 `FONT*.SYS` 點陣字，以 1-bit buffer 經 map mask 寫入。
**對 M1 的意義：** HD UI 可以「在同一個邏輯矩形上，用 HD 素材與 HD 文字覆蓋原 UI」，點擊仍走原本的矩形。
若 M2 想重新排版（移動或放大區塊），需要一層「HD 版面 → 原邏輯矩形」的對應；這一層應由 port 提供，不改遊戲 C。

## 9. 目前的 HD 文字路徑（供 P10T 與 M1）

| 項目 | 現況 | 位置 |
| --- | --- | --- |
| scroll box 位置 | 由 `SCROLLIO.C` 以 scroll 的邏輯邊界呼叫 `scroll_text_box`（行高 8） | `uw1/src/ui/SCROLLIO.C`（HD 分支） |
| 字級 | 繪製時固定 em 5（邏輯像素） | `uw1/src/port/presentation/scroll_overlay.c` |
| 字型 | `--font` 指定的單一字型檔 | port 啟動參數 |
| 換行 | `layout_text_controls`，以實際字型量測；CJK 逐字換行 | `scroll_text.c`、`i18n/layout.c` |
| **回寫遊戲記憶體** | `scroll->cur_x`、`cur_y`、`font_color`、`start_line` 取自 HD 排版 | `SCROLLIO.C` HD 分支 |
| 輸入框 | 暫停 HD scroll，使用原 renderer | `SCROLLIO.C` `edit_string`、`wyorn` |

P10T 把字級、行高等改成設定值時，第 6 點的回寫會隨設定改變；必須先完成 v2.5 P10.11 的存檔比對。

## 10. UW1／UW2 程式對應

行相似度（difflib，同名檔）：

| 目錄 | 高相似（≥ 0.8） | 低相似（< 0.4） | 只在 UW1 | 只在 UW2 |
| --- | --- | --- | --- | --- |
| `src/3d` | `VIEW3D.C` 0.96、`GAMESORT.C` 0.93、`EXPAND` 0.87、`SETPNT` 0.84 | `INSTANCE` 0.36、`SPHERE` 0.26、`TMAPOPS` 0.28 | `CAMERA`、`GRDB.C`、`NOCLIP` | `CLIP`、`PERTP`、`PERTLP`、`PROJPOLY`、`SCANLINE`、`TEXMAP`、`TEXMAPV` |
| `src/gfx` | `SPRITE` 0.98、`MODEX` 0.93、`LPFDELTA` 0.89、`GRDISP` 0.86、`VALLOC` 0.84、`SCRSHOT.C` 0.83 | `CUTS.C` 0.31、`SHOWPIC.C` 0.30、`GRMISC` 0.33 | `GRJUMPS`、`MAPDATA`、`QUADFIT`、`VSTATS`、`WALLMAP` | — |
| `src/ui` | `INPUT.C` 0.93、`GAMESTRN.C` 0.87、`MOUSE.C` 0.83 | — | `OPTIONS.C` 等 | `BROWSE.C`、`GAMESCR.C`、`ICONS.C`、`SCROLL.C` 等 |
| `src/port/3d`、`src/port/gfx` | `SETPNT` 0.96、`SCALEBM_CODE` 0.94、`HOLEPROBE` 0.92 | 其餘多在 0.01–0.25（各自翻譯的機器碼） | `WALLMAP`、`PERSPMAP`、`POLYFILL` 等 | `TEXMAP` 等 |

**共用層級建議：**

- **可共用：** overlay 顯示通道、HD 文字與游標、UI 區域對應層、render database 解讀器的骨架（opcode 語意大多相同，編號沿用 UW2）、
  貼圖與調色盤載入、素材覆寫機制。
- **各自實作：** 3D 模型 bytecode 的差異、rasterizer 細節、UI 版面、過場（`CUTS.C` 差異大）。

## 11. 對 M1／M2／M3／M4 的建議

**M1／M2（與 P12 合流）**

- 以 overlay 為唯一 HD 顯示通道；HD UI 元件定義在邏輯矩形上，由 overlay contract 換算到輸出像素。
- 素材覆寫以 `.GR` 檔與索引為 key；沒有覆寫時退回原圖放大，不得出現空白。
- HD 文字使用 P10T 的設定（字型、字級、行距）；每個 surface 一組。
- 嚴守「只讀不寫」：HD UI 不寫任何遊戲記憶體；需要的狀態以 immutable snapshot 交給 presentation thread（已是 overlay contract 的要求）。

**M3（P13 之後）——建議方案 A：原生 HD renderer 解讀 render database**

```text
遊戲 C → render database（每 frame）
   ├─→ 原 renderer（翻譯碼）→ 邏輯 FB：pick frame、回歸 golden、HD 關閉時的畫面
   └─→ 新 HD renderer（一般 C）→ HD buffer → overlay 的 3D view 矩形
```

- 第一個 gate 依計畫：同一份 database，HD renderer 在 1× 與原 renderer 逐 pixel 比對（或可量化的差異），再到 2×。
- 優點：遊戲狀態與 replay 完全不動；pick 不變；可逐步取代；UW1／UW2 共用骨架。
- 待解：3D 模型 bytecode 與 UW1 自有 opcode；sprite／critter 的縮放與透明；距離陰影表與調色盤；每 frame 雙重繪製的效能。

不建議：

- 方案 B：手改 `asm2c` 產物或原版組合語言使之參數化——違反「不得手改」與 DOS byte-match，且分散在約 15 萬行。
- 方案 C：只做像素放大濾鏡（xBR 等）——成本低但不是真 HD；可作為 Q 軌道的 presentation 選項。

**M4（寬螢幕）**

- 3D view 矩形固定在 320×200 版面中；寬螢幕 3D 需要 HD renderer 畫出比原矩形更寬的範圍。
- 三種做法、成本與 gameplay 影響見 [§14](#14-169-寬螢幕評估)；建議依 A → B → C 的順序。

## 12. 開放問題與下一步

1. 追蹤 UW1 自有 opcode（1Eh、20h、24h、28h、30h、52h、6Ah）與 3D 模型 bytecode 的格式——M3 的前置研究。
2. 確認 HD scroll 回寫的欄位是否進入存檔（v2.5 P10.11；屬 P10，不屬 M0）。
3. 為 M1 列出主畫面每個 UI 元件對應的 `.GR` 檔與索引，作為素材覆寫的 key 表。
4. ~~評估 HD renderer 雙重繪製的效能預算~~：已量測，見 [§13](#13-效能量測)。
5. UW2 的 UI 區域與 `GAMESCR.C`／`ICONS.C` 的細部對照，留到 P14／M5。

本稽核沒有修改任何程式（§13 的量測用本機暫時修改，量完已還原，未提交）。**M0 STOPPED HERE；M1／M3 沒有開始。**

## 13. 效能量測

2026-10-23 追加。目的：以實測取代 §11 的估算，作為 M3 的效能預算。

**方法：** 在本機暫時替 `c3dentry.c` 與 `VIEW3D.C` 加入以 `UW1PORT_PERF_LOG` 開啟的計時（`plat_counter`），
重播 `tests/replay` 的公開 session；**重播時不套用 32 fps 節奏**，所以量到的是純 CPU 時間。修改內容與腳本在
[evidence/m0-perf/](evidence/m0-perf/)（`perf-instrumentation.patch.txt`、`m0-perf.py.txt`、`m0-perf-wide.py.txt`），
量完已還原並重建；還原後 `walk`、`items`、`talk` 與 DOS golden 全部一致。

- 機器：Intel Core Ultra 7 255H（16 核）、Intel Arc 140T；Windows 11；`uw1port` 一般 build（翻譯模組 `-O2`）。
- 樣本：10 個 session、19,821 張 3D 畫面。`fallpause`、`load`、`overpay`、`powerkill`、`tooheavy` 需要特定存檔起點，
  `intro` 沒有 3D 畫面，這 6 個沒有樣本。

### 13.1 原 renderer（UW1，172×113）

| 每張畫面 | 平均 | p50 | p95 | p99 | 最大 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 建 render database（`setup_vars`～`process_grid`） | 0.019 ms | 0.014 | 0.045 | 0.064 | 1.08 |
| `cRender`（翻譯後的 renderer 畫進 FB） | 0.87 ms | 0.65 | 1.79 | 2.18 | 18.6 |
| `cFBtoScreen`（FB 複製到 mode X） | 0.13 ms | 0.09 | 0.25 | 0.33 | 3.2 |
| 整個遊戲迴圈（相鄰兩張畫面的間隔） | 1.91 ms | 1.52 | 4.01 | 5.46 | — |
| render database 大小 | 1,418 B | 1,548 | 1,984 | 2,468 | 2,800 |

- pick frame（`do_3d_grab`）27 次，平均 0.30 ms；只畫進 FB 的 `render_FB` 11 次，平均 1.22 ms。
- 最大值 18.6 ms 出現在 `talk` 的單一張畫面，p99 只有 2.3 ms，屬於主機排程的偶發延遲。
- **預算：** 32 fps 時每張畫面有 31.25 ms。原 renderer 的 p99 是預算的 7%；整個遊戲（邏輯＋原 renderer）平均約佔
  單一核心的 6%（1.91 ms × 32）。**保留原 renderer 的成本可以忽略。**
- 原 renderer 是模擬 x86 逐指令執行的翻譯碼，仍達到每秒約 2,200 萬個可見像素（19,436 px ÷ 0.87 ms），
  含重疊繪製約 4,500–6,700 萬。原生 C 的 HD renderer 每像素應快數倍（待 M3 的 1× gate 實測）。

### 13.2 多畫格子的成本（`wide-pitch`）

`wide-pitch` 把視野射線打開到 ±90°，向下看時再多畫一次背後的格子，是「畫更多格子」的現成實例。
以 `items` 的存檔為起點，重播 `tests/replay/enhanced/wide-pitch`（7,119 張畫面）：

| | 平均 | 最大 | database 最大 |
| --- | ---: | ---: | ---: |
| 有背後那一段（兩段繪製） | 1.17 ms | 3.9 ms | 4,880 B |
| 沒有（`UW1PORT_NO_BACK_PASS`） | 0.69 ms | 3.0 ms | 2,574 B |

database 增為約 1.7 倍，原 renderer 平均多 0.5 ms，仍在預算的 4% 以內。§14 選項 C 的「多畫兩側格子」成本與此同級。

### 13.3 HD renderer 的估算（依 13.1 推算，M3 實測）

HD renderer 只在遊戲交出新畫面時重畫，每秒最多 32 次；database 不到 5 KB，交給 presentation thread 的成本可忽略。
以重疊繪製 2.5 倍估算填像素量：

| 輸出 | 3D view 像素 | 每秒填像素 | 建議 |
| --- | ---: | ---: | --- |
| 1080p，4:3 視野（774×610） | 47 萬 | 約 3,800 萬 | 單一核心 |
| 1080p，16:9 寬視野（例 1259×610，§14 C） | 77 萬 | 約 6,100 萬 | 單一核心，留餘裕可分帶多執行緒 |
| 4K，16:9 寬視野（2518×1220） | 307 萬 | 約 2.5 億 | 多執行緒（依掃描帶切分）或 GPU 後端（M6） |

結論：**雙 renderer 的設計在效能上沒有問題**；真正的成本在開發（模型 bytecode、UW1 自有 opcode）。
M3 的第一個 gate 應同時記錄 HD renderer 每張畫面的時間，用同一份腳本比較。

## 14. 16:9 寬螢幕評估

使用者目標（2026-10-23）：除了原本的 4:3，也要支援 16:9 等現代比例。因為有遊戲的 C 原始碼，
可以在 port-only 程式碼加入「只畫、不影響遊戲狀態」的第二段格子處理（`wide-pitch` 已有同類做法：`port_view_passes`），這是可行的關鍵。

### 14.1 原版的幾何

- 投影：`PROJ_X` 是視窗寬度的一半，裁切面為 x = ±z（`instance.c` 49E7、`interp.c` 2801），所以**水平視角 90°**；
  視野格的邊界射線 ±0x2040（約 ±45.4°）與之一致。（zoom 字 `0x7ED2` 對投影的實際影響尚未追蹤。）
- 畫面比例：320×200 以 4:3 顯示，像素高寬比 1.2。UW1 的 view 172×113 → **1.27 : 1**；UW2 的 208×128 → **1.35 : 1**。
- 以「每度的大小不變、只往兩側加寬」計算，新比例 A 需要的半視角為 atan(A ÷ 原比例)：

| 3D view 比例 | UW1 水平視角 | UW2 水平視角 |
| --- | ---: | ---: |
| 原版 | 90° | 90° |
| 16:9（1.78） | 約 109° | 約 105° |
| 2.06（1080p 版面例，§13.3） | 約 117° | 約 113° |

### 14.2 三種做法

| | A：4:3 置中＋兩側裝飾 | B：16:9 視窗、視角不變（上下裁切） | C：16:9 寬視野（兩側多畫） |
| --- | --- | --- | --- |
| 看到的範圍 | 與原版相同 | 水平相同；垂直只剩原版約 71%（UW1） | 水平約 105–117°，垂直相同 |
| 視野格／遊戲狀態 | 不變 | 不變 | 原格照常計算經驗值與自動地圖；兩側另以「只畫」的第二段處理 |
| 點選（pick） | 不變 | HD 畫面是原 pick 範圍的垂直裁切，線性換算即可，**仍具確定性** | 中央沿用原 pick；兩側超出原 pick 範圍 |
| 依賴 | 只需 overlay | UI 版面重排；HD 畫面需 M3（或先用原 FB 放大裁切） | M3＋UI 版面重排＋只畫的第二段格子 |
| 類別 | presentation | presentation | 兩側不可點：presentation；兩側可點：gameplay |
| 工作量 | 小 | 中 | 大 |

**C 的細節：**

- **只畫的第二段：** 原版那一段照常執行（經驗值 `pipeexp`、自動地圖 `PlayersMap`、`AnimObjInPipe` 都只由它產生）；
  兩側的格子由 port-only 的第二段畫出，必須關閉上述所有寫入，只產生 database。比照 `wide-pitch` 的 `port_drawing_behind`。
  這樣遊戲狀態與 replay 完全不變，屬 presentation enhancement，可用 Exhume 既有的 presentation 檢查驗證。
- **視野格寬度：** 視野格 33 欄（±16）。半視角 54.5° 時，邊緣在第 11 列左右就超出 ±16 欄；更遠處需要 port-only 的較寬格
  （不動遊戲的 `glocs`），或以距離霧處理。UW1 遠處多半已是暗的，霧的觀感需實測。
- **兩側點選：** 原 pick frame 只涵蓋原視野。第一版兩側不可點（游標顯示為不可互動）；要可點，得讓 HD renderer 輸出物件 ID buffer，
  這會改變遊戲輸入，屬 gameplay enhancement，錄影須帶旗標（Exhume recording format 5 已支援）。
- **效能：** 多畫的格子量與 `wide-pitch` 同級，原 renderer 多約 0.5 ms（§13.2）；HD renderer 見 §13.3。

### 14.3 UI 版面（B 與 C 共用）

16:9 在邏輯高度 200 時，寬度相當於 426.7 個邏輯像素，比原版多約 107。UW1 的主畫面可切成幾塊：

| 區塊 | 邏輯範圍（x，y 由上往下） | 來源 |
| --- | --- | --- |
| 3D view | x 52–223，y 19–131 | `place_3d_view` |
| 左側指令圖示 | x 0–51 | `INTERACT.C` 的 icons 點擊區 |
| 右側物品欄、水瓶 | x 224–319 | `INVPANEL.C`、`INTERACT.C` 的 flask |
| 下方訊息捲軸、移動箭頭、符文、法術 | y 132–199 | `PLAYER.C`、`INTERACT.C` |

- **區塊重排（M4-lite）：** 先不重畫 UI，而是把原 320×200 畫面的各區塊（縮放後）擺到 16:9 的新位置，左右欄靠螢幕兩側，
  3D view 佔中間加寬的空間。滑鼠在 SDL 換算到邏輯座標時，加上同一份版面的反向對應，遊戲收到的仍是原本的邏輯座標，
  14 個 `input_addmouse` 點擊區不用改。HD 文字與 HD 游標（overlay）跟著同一份版面換算。
- **全螢幕模式退回 4:3：** 自動地圖（點擊區涵蓋整個 320×200）、對話、交易、角色建立等畫面不適用區塊重排，切換到這些模式時改用 A。
- 之後 M1／M4 以 HD 素材重畫 UI 時，版面設定沿用同一份。

### 14.4 建議

1. **A 隨 M1 先做：** overlay 已能畫兩側，成本低，馬上有 16:9 的完整畫面。
2. **B 在區塊重排完成後做：** 不改遊戲狀態、點選仍具確定性，是「16:9 填滿」最安全的做法。
3. **C 在 M3 的 1×／2× gate 通過後做：** 預設關閉的 presentation enhancement；兩側可點另立 gameplay 項目。
4. 版面、比例與視角都放進 P10T 同一份預設設定檔（例如 `aspect = 4:3 | 16:9`、`wide_view = off | crop | wide`、`wide_fov_max`），設定介面之後再談。
5. UW2 的版面（view 16,182 起、208×128）在 P14／M5 另做同樣的區塊表。
