# UW2 中文化 Phase 14：共用模組與垂直切片（2026-10-31）

**狀態：** 垂直切片完成：UW2 載入繁中 catalog、對話以 HD 字型顯示中文；英文模式與 DOS 逐檢查點一致。
尚未交付玩家測試版（訊息欄的組句、選項、書本、物品名、介面標籤等見「下一步」）。

## 共用模組搬到 Exhume runtime

主計畫原則：UTF-8、catalog、CJK 排版、FreeType、HD 文字呈現由兩款遊戲共用，不複製 UW1 的整套。
這次把遊戲無關的模組從 `uw1/src/port/` 搬到 Exhume 子模組的 `runtime/port/`（兩款遊戲的建置都會編譯這裡）：

| 位置 | 模組 |
| --- | --- |
| `runtime/port/i18n/` | `utf8`、`layout`（CJK 斷行）、`localization`（catalog 查詢與顯示來源）、新增 `cp437`（遊戲原文轉 UTF-8） |
| `runtime/port/presentation/` | `hd_font`（FreeType）、`text_style`（字型與間距設定）、`scroll_text`（訊息欄／對話框排版）、`scroll_overlay`（疊圖繪製）、`hd_cursor` |

- 內容不變，只調整 include 路徑；FreeType 巨集由 `UW1_HAVE_FREETYPE` 改名 `HD_HAVE_FREETYPE`。
- 新增共用函式：`text_style_boxes()`（兩個文字框的樣式與字型檔解析，UW1 `main.c` 原有的 30 行改呼叫它）、
  `text_style_program()`（訊息前綴，預設 `uw1port`，UW1 輸出不變）、`cp437_to_utf8()`（UW1 `scroll_bridge.c` 改用它）、
  `plat_request_shot()`（測試用：下一幀截圖）。
- 遊戲原始碼（`.C`）的移植版掛鉤要 include 這些標頭：Exhume 與 UW2 的 `portcheck.py` 在遊戲 C 的 include 路徑**最後**加上 runtime 的 `port/`，
  排在所有既有目錄之後，不會遮蔽原本找得到的標頭。
- UW1 的 17 個測試腳本改經 `tools/i18n/portsrc.py` 找原始檔（先找 UW1，再找 runtime）。文件連結改指向 runtime。
- UW1 留在原處的是綁 UW1 的部分：`phrases`（中文句型表）、`ui_text`、`scroll_bridge`、`hd_label`、`caption`、`qty_dialog`、`text_entry`、
  `settings_text`、`name_gloss`、`input_alias`、`user_text`、`map_notes` 等；之後 UW2 需要時再依同樣方式抽出共用部分。

## UW2 的接點

- `uw2/tools/portbuild.py`：pkg-config 找得到 `freetype2` 時編入 HD 文字（`text: FreeType HD text`），找不到照常建置。
- `src/port/presentation/localized.c`（新）：UW2 的接線——選項 `--locale en|zh-TW`、`--i18n-file`、`--hd-scroll`、`--font`，
  測試用 `--scroll-log`、`--i18n-trace`、`--shot-on-text N`；catalog 預設找 home 的 `i18n/zh-TW/catalog.uw2loc`，再找目前資料夾；
  樣式讀 locale 的 `text-settings.ini` 與玩家的 `uw2port.cfg`（鍵名同 UW1 的 `font_cjk_*`）；遊戲執行緒量字、呈現執行緒畫字。
- 遊戲原始碼的掛鉤（全部在 `#ifndef __TURBOC__`，原檔維持 ASCII，DOS 建置不受影響）：
  - `GAMESTRN.C` `get_string`：記錄譯文的顯示來源（`port_i18n_observe`），遊戲字串本身仍是英文。
  - `SCROLL.C` `scroll_print`：HD 開啟時清紙面（UW2 紙色 0x71）、改寫入 HD 文字格，捲軸位置與顏色跟著 HD 排版。
  - `SCROLLIO.C`：`init_scroll`／`scroll_clear` 同步清除 HD 框；`wdialog`／`wyorn` 打字時暫停 HD（沿用原版字形，同 UW1）。
  - `BABL.C`：含 @ 變數的譯文模板以顯示用方式展開（`port_template_shown`／`port_convert_display`，同 UW1 P11.1），不碰腳本記憶體。
  - `CONVERSE.C`：`npc_say`、`play_respond`、`play_say`、`conv_print` 的 HD 分支。

## 翻譯資料

- 同伴提供的 `i18n/uw2/zh-TW/master.tsv`（工作區、Git 外）以 UW1 的 `l10n.py normalize` 補上由原文推導的旗標
  （object-name 456、dynamic-template 391、control-code 74、兩者 14，共 935 列；譯文、狀態、雜湊未動）。原檔備份 `master.tsv.pre-normalize-20261031`。
- catalog：`l10n.py build --master <UW2 主表> --pak <UW2 DATA/STRINGS.PAK>`，9,543 筆、1,082,589 bytes，
  SHA-256 `2817925b…`，輸出在本機 `uw2/build/i18n/slice/`（不提交）。格式沿用 UW1（`UW1LOC1`）。

## 驗證

- Exhume 搬移後 UW1：Windows 建置、17 個單元測試、16 個公開 session × 四模式存檔一致、DOS golden 吻合，
  且與搬移前（`hd-border/replays.json`）逐一相同（`.poc-tools/uw2-shared-check.py`）。
- UW2：`tools/replay.py verify all` 12 個 session 與 DOS 一致。四模式（`.poc-tools/uw2-slice-check.py`）：
  英文與中文（載入 catalog、不開 HD）12 個 session 全部逐檢查點與 DOS golden 相同；
  中文 HD 與中文 HD 自訂參數（字級、間距、描邊、Noto 字型）無錯誤，`items`／`load` 的存檔與英文、DOS 相同。
- 畫面：`talk` 重播（`SDL_VIDEODRIVER=offscreen`、`--shot-on-text 1`）中 Nystul 的台詞顯示為
  「如果你能稍等一下，聖者，我想不列顛王就要開始演講了。」；讀檔後查看袋子，訊息欄以 HD 字型顯示（仍是英文組句）。
- UW2 UBSan 版：`verify all --debug` 12 個 session 與 DOS 一致；中文 HD 重播 `talk`、`items`、`lb` 無 UBSan 回報。
- repocheck、原版掛鉤 ASCII 通過。未執行 DOS 重編譯（gate 由 CI 執行）。
- 注意：在 Windows 上執行 `tools/portstubs.py` 會因路徑分隔符與 MinGW 的 CRT 符號產生假差異，與本次修改無關，不要提交其輸出。

## 第二批：HD 游標、共用 bridge、訊息欄組句（2026-10-31）

- HD 游標（提交 `89a5157`）：`MOUSE.C` 的 `drawMouse`／`draw3dMouse` 在 HD 捲軸開啟時改由呈現層畫游標（在 HD 文字之上），存背景等仍是遊戲自己的。
- 再搬一批到 Exhume runtime：`i18n/name_gloss`（專有名詞首次加註）、`input_alias`（中文輸入話題）、`user_text`（玩家中文姓名／存檔說明）、
  `presentation/scroll_bridge`（HD 捲軸的寫入、選項、書本、打字回答）。bridge 需要遊戲提供 `scroll_print`、`port_template_shown`（BABL.C）
  與各自的句型表 `i18n/phrases.h`（`port_zh_lit`、`port_zh_literal`）；兩款遊戲都有。新增測試用 `port_scroll_shot_after(N)`。
  UW1 測試替身補 `plat_request_shot`。
- UW2 自己的句型表 `src/port/i18n/phrases.c`（同 UW1 的 API，內容只放 UW2 用得到的）。
- UW2 的組句（全部 `#ifndef __TURBOC__`、只在 HD 捲軸＋catalog 時，顯示用）：
  - `GAMESTRN.C`：`PortZh` 工具（`ui.h` 宣告）、`get_name` 記下名稱譯文（單獨印出的名稱顯示中文）、`game_strings_3`。
  - `LOOK.C`：查看物品／生物／書本／骨頭的整句（數量、品質、魔力／詛咒、黑岩寶石的冷暖、法術與充能、毒、所有者）。
  - `USEITEMS.C`：手錶時間、食物、閱讀；`INTERACT.C`：查看牆面／地面；`GAMESCR.C`：生命／法力 N／M；
    `INVDATA.C`：裝備損壞；`CRITTIME.C`：NPC 發現偷竊；`SKILLS.C`：解除陷阱。
  - `CONVERSE.C`：對話選項經 bridge 顯示譯文、點選以 HD 文字格命中（UW2 的點擊區從 x 0x10 起），選擇後回顯譯文。
- 字面對照（bridge 對單獨印出的英文片段）：`The `、`You see `、`trap`、`poison trap`、`Not a spell`、存檔清單標籤、滑鼠視角訊息等。

## 第三批：中文輸入話題、專有名詞加註（2026-10-31）

- `.poc-tools/uw2-cnv-unpack.py`（本機）把 UW2 的 LZSS 壓縮 CNV.ARK 解成 UW1 格式，UW1 的 `babl_audit.py` 照用：
  102 段對話、7,304 個對話字串，被比對的 22 個（與先前稽核相同）。
- `i18n/zh-TW/input-aliases.tsv`（提交）：22 列，別名取自譯文裡玩家看到的寫法——賈納的暗號「烽火」（畫面是「beacon（烽火）」）、
  畢夏普、致命追蹤彈權杖、默贊賣的藥水（鋼鐵之軀、耐火術、解毒術、大／小醫療術、恢復法力、隱形術、法力增幅、耐擊術；
  「resist fire」用「抗火術」）、尼斯圖的「王座廳」「凌晨四點」、哥布林守衛的「一號」。哥布林守衛的通關密語
  deco morono、xyzzy 照畫面英文輸入，不配中文。
- `CONVERSE.C` 的 `babl_ask`：HD 捲軸開啟且對照表已載入時走共用的 `port_scroll_ask`（輸入法打中文，對照成腳本比對的英文），否則原版 `wdialog`。
- `tools/i18n/proper_names.py`（提交）產生 `i18n/zh-TW/proper-names.tsv`（93 個名稱）：UW1 權威資料的人名／地名／組織、
  UW2 詞彙表的新專有名詞（`i18n/zh-TW/proper-nouns.tsv`）、遊戲 NPC 名牌（block 7）的中文；只收 UW2 對話／書本譯文用到的。
  `--check` 比對提交的表。`localized.c` 載入兩個表；`strt_converse` 每段對話重設加註。實測 `lb`：「不列顛尼亞（Britannia）」「派特森（Patterson）」首次加註。
- `tools/i18n/test_input_alias.py`（提交）：對照表格式、同段對話別名不重複、與 STRINGS.PAK 的雜湊與比對字一致，並用 runtime 的
  `input_alias.c` 實測對照結果。實機輸入法路徑尚未遊玩驗證（UW2 沒有傳送到 NPC 的測試指令；有 babl_ask 的對話不在既有重播裡）。

## 已知與下一步

- 訊息欄的組句（「You see a sack.」等由程式拼接的句子）仍是英文：需移植 UW1 P11.2 的 `port_zh` 組句與 `phrases`。
- 對話選項（選單）、輸入話題、書本／卷軸、物品名、介面標籤（`string_to_screen`）、過場字幕、存檔名稱尚未接。
- HD 游標未開：遊戲游標會壓在 HD 文字上（UW1 開 HD 捲軸時一律開 HD 游標，需 UW2 `MOUSE.C` 掛鉤）。
- `[MORE]` 改為「[繼續]」，與 UW1 相同。
