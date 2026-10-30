# 開發交接：UW1 繁中整合版／新對話入口（2026-10-31）

## 1. 新對話接手順序

1. 一律用繁體中文、台灣用語回覆。工作區為 `D:\git\Ultima Underworld`，主 repo 在其下的 `underworld-exhumed`。
2. 先確認 Git 狀態，再讀 `uw1/docs/i18n/P13_INPUT_STACKS.md`、`P13_QA.md`、`IDEAS_BACKLOG.md`。
3. **2026-10-28 新增三項強化功能**（對話打錯字回應 `unknown-topic`、拆分數量預設全部 `split-all`、E 撿起自動堆疊併入 `e-takes`）
   已實作、驗證、更新本機 Windows 測試版並推送。`split-all` 2026-10-30 使用者已確認 OK（當初回報「無效」是設定檔沒開這項，非程式問題）；
   另兩項**尚待使用者實測確認**。三項預設關，要在 F11「強化功能」開啟。
   2026-10-27 的新按鍵已由使用者確認 OK；既有功能與 Mac 0.5 亦已確認。
4. **P13.2 全遊戲遊玩驗證與 P13.3 譯文審閱**由使用者邊玩邊測，列為持續進行中，**不算專案未完成的因素**。
   不要自動重做已確認功能或開始候選功能。
5. **2026-10-28 按鍵說明頁**（F11「按鍵說明」分頁、Shift＋F1）2026-10-29 使用者已確認 OK；見 §4.3、`P13_KEY_HELP.md`。
6. **2026-10-29 地圖橡皮擦輔助** `easy-erase`（預設關）已實作並推送，待使用者實測確認；見 §4.4、`P13_EASY_ERASE.md`。
7. **2026-10-30 強化功能即時生效**（使用者同意）：F11「強化功能」9 項改為不用重啟，切換記進錄影；2026-10-31 使用者已確認 OK。見 §4.5、`P13_LIVE_ENHANCE.md`。
8. **2026-10-31 標題頁游標延遲修正**：中文 HD 大字的描邊每幀重畫上百次，標題頁只剩每秒 13 幀；改為描邊快取後回到 120 幀。
   2026-10-31 使用者已確認 OK（跟手了）；見 §4.6、`P13_HD_BORDER.md`。
9. 尚未開始第一層審閱批次，未授權 P13 tag，也未授權其他新功能。

根目錄 `HANDOFF.md` 與 repo 的 `uw1/docs/i18n/HANDOFF.md` 必須保持同內容。
下文 `uw1/...`、`docs/...` 相對主 repo；`.poc-tools/...`、`UW1-zhTW-playtest/...` 相對工作區。
完整規劃見工作區主計畫 v2.5；其開頭仍是舊進度，當前狀態以本文件及有日期的最新報告為準。

## 2. Git 與交付現況

| 項目 | 現況 |
| --- | --- |
| 主 repo／分支 | `pmanyeh/underworld-exhumed`／`feature/zh-tw-native` |
| 最新功能提交 | 2026-10-31 HD 描邊快取（標題頁游標延遲），提交以 `git log -1` 為準；之前 `4f95eb9`（強化功能即時生效）、`aee554b`（地圖橡皮擦輔助）、`3122640`（按鍵說明頁）、`b3249e6`（三項強化功能） |
| Exhume 子模組 | `pmanyeh/Exhume`，固定 `370b998bb5e1214b7b712731c504774fd86f8a02`（2026-10-28 按鍵說明頁） |
| 子模組 checkout | 本機為固定提交的 detached HEAD，工作區乾淨；遠端開發分支 `feature/presentation-overlay` |
| 階段／tag | P13 進行中；最後階段 tag 為 `zh-tw-native-p12`（`0169ca1`），尚未建立 P13 tag |
| 版本號 | 0.5 是攜帶測試包版本號，不代表 P13 驗收完成 |
| 本機 Windows 測試版 | `UW1-zhTW-playtest/uw1port.exe` 已包含新按鍵、2026-10-28 三項功能、按鍵說明頁、2026-10-29 地圖橡皮擦輔助、2026-10-30 強化功能即時生效與 2026-10-31 HD 描邊快取（強化功能皆預設關）；SHA-256 `c75f8823…`，前一版（`81486b36…`）備份 `uw1/build/i18n/hd-border/previous-playtest-uw1port.exe`，home 104 檔雜湊不變 |
| 已交付 Mac 0.5 | 2026-10-27 使用者回報實機測試 OK；現有 Mac ZIP 不含新按鍵與三項新功能 |
| ZIP／私人移機包 | 本次未重新打包或上傳私人資源 |

已核實 CI：功能提交 `f50151e` 的 Windows／macOS／Linux／Web 與 repocheck 全通過，
[run 40483012701](https://github.com/pmanyeh/underworld-exhumed/actions/runs/40483012701)。
文件提交 `156f368` 的 `uw1-port` 與 repocheck 亦通過，
[run 40483332761](https://github.com/pmanyeh/underworld-exhumed/actions/runs/40483332761)。
2026-10-28 功能提交的 CI 結果請以 GitHub Actions 為準，新對話先核實。
CI 建置通過不等於已重新交付含新按鍵的 Mac 整合包。

## 3. 使用者已確認的功能

| 功能 | 紀錄／報告 |
| --- | --- |
| 戰鬥傷害數字、敵人血條、血瓶／法力瓶數值、自動開鎖、中鍵視角、C 戰鬥／X 關容器、E 撿起 | 既有功能已確認；`IDEAS_BACKLOG.md`、`P13_COMBAT.md` |
| 地圖中文註記 | 已確認；`P13_MAP_NOTES.md` |
| 袋子、「繼續」、屬性頁技能清單滾輪 | 已確認；`P13_STATS_WHEEL.md` |
| 無敵預開啟時讀檔閃退修正、個體 HP 作為血條最大值 | 已確認；`P13_CHEATS.md`、`P13_COMBAT.md` |
| 提高亮度、0～15 強度、自然明暗漸層 | 2026-10-26 已確認結案；`P13_CHEATS.md` |
| 敵人攻擊時顯示血條、物件不損耗、Z 開關地圖、其餘已實作密技安全情境 | 2026-10-27 已確認；不代表全地圖風險或全遊戲長時間 QA 已完成 |
| 中文模式屬性頁英文姓名／數值、地圖英數註解 HD 字型 | 2026-10-27 已確認；`P13_HD_ASCII.md` |
| Mac 繁中整合測試版 0.5 | 2026-10-27 實機測試 OK；`MAC_0.5.md` |
| 按鍵說明頁（F11 分頁、Shift＋F1） | 2026-10-29 已確認；`P13_KEY_HELP.md` |
| 拆分數量預設全部 `split-all` | 2026-10-30 已確認；`P13_INPUT_STACKS.md` |
| 強化功能即時生效（9 項不用重啟） | 2026-10-31 已確認；`P13_LIVE_ENHANCE.md` |
| 標題頁游標延遲修正（HD 描邊快取） | 2026-10-31 已確認；`P13_HD_BORDER.md` |

P0～P12 中文化、HD 文字／游標、中文輸入、數量滑桿、中文姓名與存檔說明已實作，見 `P12_UI.md`。
操作功能確認不會自動把譯文改成 reviewed，也不等於 P13 全部驗收完成。

## 4. 最新按鍵決定：已實作、使用者已確認（2026-10-27）

**2026-10-27 新決定取代 2026-10-26「保留空白鍵攻擊」的舊決定。** 沿用 F11「現代按鍵」，預設關閉，中英文皆適用。

| 操作 | 最新行為 |
| --- | --- |
| 空白鍵 | 跳躍，與 `j` 相同；取消「沿用上次攻擊方式」 |
| 攻擊 | 滑鼠與 `.`／`;`／`p` 的刺擊、揮砍、敲擊與蓄力保留 |
| Shift／j、Ctrl+Shift／大寫 J | 既有兩種原版跳躍保留 |
| Ctrl+Alt+空白鍵 | 啟用符文快捷鍵時仍為施法 |
| X | 只關閉容器；移除同時綁定的慢速前進，未另指定慢速前進替代鍵 |
| C、Z、R／F 等單次操作 | 每次按下只觸發一次；放開再按才再次執行 |
| 移動、俯仰、背包捲動、攻擊蓄力 | 仍支援按住；文字輸入維持原有連續輸入 |

X 的前進來自移動表持續輪詢；C 反覆切換來自每 30 ticks 的按住按鍵重複派送。
修正使用既有 `KEY()` 錄影邊界與按下計數，沒有另讀平台狀態或新增錄影格式。
舊 modern-keys 錄影中的 Space／X 意義已變，不宣稱其舊情境結果不變；未覆寫既有錄影、goldens 或 verifier。
完整範圍與結果見 `P13_MODERN_KEYS.md`。

## 4.1 2026-10-28 三項強化功能：已實作、待使用者確認

| 功能 | 開關 | 行為 |
| --- | --- | --- |
| 對話打錯字回應 | `unknown-topic` | 打了 NPC 不認得的字（含不在對照表的中文詞），NPC 用自己原本的「不知道」回答；腳本仍不回話時補「我不知道這是什麼……」 |
| 拆分數量預設全部 | `split-all` | 原版「Move how many?」與 HD 數量面板從整疊開始；Enter 全拿，打數字取代預設值 |
| E 撿起自動堆疊 | 併入 `e-takes` | 目前背包面板（背包或打開的袋子）有能合併的同一疊就疊上去，沿用原版合併規則 |

原因調查：英文打錯字時原版腳本幾乎都有回應；沒反應的是中文詞對照後變空字串，腳本當成沒打字。
三項都歸 gameplay、預設關。實作、驗證與新增測試指令（`test-inv`、`test-take`、`UW1_TEST_ASK`）見 `P13_INPUT_STACKS.md`。

## 4.3 2026-10-28 按鍵說明頁：已實作、2026-10-29 使用者已確認

- 使用者決定：F11 新增「按鍵說明」分頁，快速鍵 **Shift＋F1**（在該頁再按一次關閉），內容只顯示目前有效的按鍵。
  單按 F1 仍是原版選項畫面。不是強化功能，沒有開關；只影響設定畫面顯示，不改遊戲狀態、存檔或錄影。
- 內容依啟動時的強化功能（`modern-keys`、`e-takes`、`mouse-look`、`invert-look`、`mouse-wheel`、`rune-keys`）、
  滑鼠鎖定與 `--record` 動態列出，中英文各一套；清單與用詞經使用者確認，原版 Ctrl 快捷鍵依原始碼定稿。
- 執行環境（Exhume 子模組）新增 `SET_TAB_HELP`、`settings_set_help()`、`settings_show_tab()`，分頁寬度改依名稱分配；
  UW1 新增 `src/port/ui/key_help.c`。`key_help.c` 不可引入 `compat.h`（`pack(1)` 會讓結構配置不同，曾因此閃退）。
- 實作與驗證見 `P13_KEY_HELP.md`；`docs/SETTINGS.md` 已補「Keys」一節。

## 4.4 2026-10-29 地圖橡皮擦輔助：已實作、待使用者確認

- 使用者回報橡皮擦「失效」：重播其錄影查得是原版規則。判定點在橡皮擦圖示正中央，範圍只有字高；落空就放下橡皮擦，下一下變成寫註記。非退化。
- 使用者選方案 3，強化功能 `easy-erase`（「地圖橡皮擦輔助」，gameplay、預設關）：註記上方 6、下方 3、左右 3 像素內也算點中；
  沒點中保留橡皮擦；右鍵、Esc 或在右側面板落空時放下。原版擦除程式碼不動，只在 `ui/AUTOMAP.C` 加移植版專用的 `port_erase_pick`。
- 實作與驗證見 `P13_EASY_ERASE.md`；按鍵說明頁在開啟時多一行「右鍵、Esc　放下地圖的橡皮擦」。

## 4.5 2026-10-30 強化功能即時生效：已實作、2026-10-31 使用者已確認

- 使用者問 F11 為何很多設定要重啟，說明後同意：只在動作或繪圖當下讀開關的 9 項改為不用重啟——
  `invert-look`、`flask-numbers`、`auto-key`、`e-takes`、`damage-numbers`、`enemy-health`、`split-all`、`unknown-topic`、`easy-erase`。
  關閉 F11、回到 3D 畫面且游標沒拿東西時套用，訊息欄顯示「設定已生效。」。其他（`modern-keys`、`mouse-look`、`perspective`、`skip-intro` 等）仍要重啟。
- 沿用密技分頁的有界佇列與安全點：公用指令 `menu-enhance NAME 0|1`（只接受上述 9 個名稱）記入 format 6 錄影，重播不需測試環境變數。
  關閉 `flask-numbers` 時以新增的 `flask_text_clear()` 清掉畫面上的數字。只改 `src/port/`，Exhume 子模組不變。
- 實作與驗證見 `P13_LIVE_ENHANCE.md`；本機工具 `.poc-tools/live-enhance-check.py`（units／replays／live），產物在 `uw1/build/i18n/live-enhance/`。

## 4.6 2026-10-31 標題頁游標延遲：已修正、2026-10-31 使用者已確認

- 使用者回報標題頁滑鼠不跟手、選存檔畫面正常。英文模式量不出差異；改用使用者設定（中文、`--hd-scroll`、5 倍視窗）重現：
  標題頁每秒約 13 幀、每幀繪製 75～88 毫秒、游標延遲中位數 108 毫秒。
- 原因：`hd_font.c` 的 `hd_scroll_glyph_border` 把描邊畫成「每個位移各畫一次字形」，半徑隨放大倍率變大；標題頁大字每字每幀上百次。
- 修正：不透明時把描邊合成一張遮罩，存在字形快取（換半徑重建、淘汰時釋放）；半透明維持原畫法。改後標題頁 120 幀、延遲 16.4 毫秒。
  新舊逐像素比對只有四捨五入的 1～2 色階差。只改 `src/port/`。
- 量測用的暫時程式已移除未提交。注意：`env.sh` 的 MSYS2 git 會把整個 repo 誤報為 CRLF 改動，狀態一律用 Git for Windows 看。
- 實作與驗證見 `P13_HD_BORDER.md`；本機工具 `.poc-tools/hd-border-check.py`（units／replays）、`hd-border-deliver.py`，產物在 `uw1/build/i18n/hd-border/`。

## 4.2 2026-10-28 翻譯資料：蜥蜴人語、權威資料、UW2 譯文

- **蜥蜴人語加註**（使用者同意）：UW1 3599、3698、3861、3864、3865 的 98 句蜥蜴人語保留原文，後面加全形括號中文意思，
  例如「Bica, sor'click.（你好，陌生人。）」。字義依穆爾戈（3608）的字典；玩家輸入的關鍵字與混有中文的玩家選項不動。
- **權威資料**（決策 UD-20261028-001）：刪除把 isili（我）誤判為人名的「伊西里」詞條；Thepa 保留「瑟帕」為族群自稱，
  註明意思是蜥蜴人，改為 approved。權威 979 詞、22 詞人工核准；validate_authority、l10n validate／authority、相關單元測試通過。
- **本機測試版翻譯資料**：catalog 原為 10/24 舊版（不含哥布林等之後的譯文修正），已換成目前主表重建的
  `uw1/build/i18n/lizard/catalog-1.bin`（6,900 entries，SHA-256 `7a3d2c23…`），舊檔備份在同目錄 `previous-playtest-catalog.uw1loc`。
  測試版原本缺 `proper-names.tsv`（專有名詞首次出現加註英文沒有作用），已補上並確認載入 116 個名稱。玩家 home 未動。
- **UW2 譯文**（同伴提供，工作區 `i18n/uw2/zh-TW/`，Git 外）：9,547 筆、格式同 UW1 主表、全部 draft。
  已依 UW1 權威資料重新對照 `GLOSSARY.md` 並修正 `master.tsv` 202 列（惡鬼→哥布林、Blackthorn→布萊克索恩、
  英勇→勇氣、魔法箭→魔法之箭、技能名、美德名、全形括號等），原檔保留為 `*.orig-20261028`。
  已用使用者的 UW2 遊戲資料核對（GOG 光碟映像 `game.gog` 的 `UW2\`，解出到工作區 `Game Resources/Ultima Underworld 2/UW2/`，
  Git 外；`UW2.EXE` SHA-256 `bf233abb…`，與 UW2 移植版相同）：9,547 列雜湊全部相符、替換標記無問題、沒有不存在的鍵。
  CNV.ARK（UW2 為 LZSS 壓縮）解開後用 UW1 的 `babl_audit` 稽核 102 段對話：比對玩家輸入的字串 22 個，
  其中 4 個被翻成中文（3593/45、3601/89、3629/70、3680/79），已依 UW1 規則改回英文（untranslated），
  修改前備份 `master.tsv.pre-inputkeys-20261028`。未收錄的 226 筆是除錯報告、程式代碼、加密外星語與輸入關鍵字，屬正常。
  UW2 程式面中文化尚未開始。

## 5. 本次驗證與玩家資料保護

2026-10-28 三項功能：Windows 正常建置、17 個單元測試、16 個 session × 四模式存檔一致（新功能關閉）及 DOS golden 吻合；
中英文實機情境 9 項通過，背包情境與英文對話情境錄影重播的 checkpoint 與存檔一致；authority、專有名詞、repocheck、
原版掛鉤 ASCII、`git diff --check`（Git for Windows）通過。未執行 UBSan 建置與 DOS 重編譯。
測試版只換 EXE：新 SHA-256 `a1a382b615dc7d81a2d7a2b225e556fd8e17fd2ee112ddce789e918c9dcf3afb`，
舊檔備份 `uw1/build/i18n/p13-features/previous-playtest-uw1port.exe`；搭配原有 DLL、僅系統 PATH 的重播通過；
玩家 `home` **100 個檔案**更新前後雜湊一致。產物與摘要在 `uw1/build/i18n/p13-features/`。

注意：`env.sh` 把 MSYS2 的 git 放進 PATH，它對子模組 CRLF 檔案的 `git diff --check` 會誤報整份檔案；檢查 git 一律用 Git for Windows。

2026-10-27 按鍵修改的實際結果：

- Windows 正常／UBSan 建置通過，含 FreeType、Nuked OPL3、libmt32emu；17 個單元測試程式全通過。
- `test_mouse_cursor.py --sanitize`：滑鼠 45 項、技能滾輪 140 項、新按鍵 5,473 項通過。
  涵蓋長按超過 30 秒、快速重按、地圖、文字、符文施法、移動與 KEY 狀態重播。
- 中英文遊戲情境確認 X 不移動、C 長按不反覆切換、空白鍵起跳與落地、F 切換及 Z 開關地圖。
  情境錄影重播的 checkpoint 事件、時間、呼叫次數與存檔一致。
- 不含私人指令的公用輸入錄影，由正常／UBSan 版重播皆通過。
- 16 個公開 session × 英文／中文／中文 HD／中文 HD 自訂參數：存檔原始雜湊一致，有 DOS golden 的亦吻合。
  這輪為既有模式回歸，新現代按鍵未啟用；無須地圖填充例外。
- authority、專有名詞、portcheck、原版掛鉤 ASCII、repocheck、diff 空白檢查通過。

Windows 測試版只換 EXE；搭配原有 DLL、僅 Windows 系統 PATH 的重播檢查通過。
更新前後玩家 `home` 的 **98 個檔案雜湊全部一致**，未覆寫存檔、設定或中文旁註。

| 本機證據 | 位置／值 |
| --- | --- |
| 產物目錄 | `uw1/build/i18n/modern-keys/` |
| 摘要 | `units.json`、`replays.json`、`live.json`、`public.json`、`delivery.json` |
| 新 EXE SHA-256 | `0eac47f49748d5469680b3649af16b910fce072b24b75e6534c6e1c1fd57ffa7` |
| 舊 EXE 備份 | 同產物目錄的 `previous-playtest-uw1port.exe` |
| home 更新前雜湊 | 同產物目錄的 `player-home-before.json` |

0.5 原交付的打包、141 檔 home／33 檔存檔備份及離線移機驗證，屬先前電腦的歷史結果，
見 `RELEASE_0.5.md`、`MIGRATION_0.5.md`；不要與本機此次的 98 檔更新檢查混用。
Mac 0.5 來源 `4c662c1`、Actions run `40308423602`，使用者實測已確認；該 release 的私有資料自動重播
仍因 SSH 金鑰格式／權限失敗而未執行，未經 Apple 公證。見 `MAC_0.5.md`。

## 6. 翻譯與本機資源現況

- 既有覆蓋率報告：可翻譯 6,983 筆、已翻 6,893 筆（98.71%）；其餘 90 筆為刻意保留的對話比對字。
- 本次直接讀取 `uw1/i18n/zh-TW/master.tsv`：共 **6,900 列，draft 6,415、reviewed 485**。
  舊報告「所有譯文都是 draft」已過時；覆蓋率、主表列數與 catalog entries 是不同統計，勿混用。
  本次只核對統計，沒有改動任何譯文或審閱狀態。
- 目前可用 catalog：`uw1/build/i18n/modern-keys/catalog-1.bin`，由目前 master 與本機遊戲資料重建，
  6,900 entries、681,996 bytes；SHA-256 `78543defe8762bb545ca9b09d06cfae7097d96cd934990a035745f238748ff75`。
  來源與主表雜湊見同目錄 `catalog-manifest.json`。
- 本機未找到舊 `uw1/build/i18n/p13-goblin/catalog-1.bin` 與 `.poc-tools/v05-check.py`，不要照舊路徑直接執行。
  Python venv、MSYS2 CLANG64、遊戲資料、函式庫與既有 SAVE 測試副本已可用，已完成編譯及驗證。
- 本機新增工具：`.poc-tools/modern-keys-check.py`（units／replays／live）、`modern-keys-public.py`、
  `modern-keys-deliver.py`；2026-10-28 另有 `p13-features-check.py`（units／replays／live）與 `p13-features-deliver.py`，
  均屬 Git 外私人工作區工具。這台電腦不能建立 symlink，兩個新工具把 `data_skip` 清空（遊戲資料夾本來就沒有 SAVE1～4）。replays／錄製工具的固定輸出目錄已存在，
  重跑前應改為新的輸出位置；deliver 有 EXE 替換副作用，不要當一般測試重跑。
- 舊情境腳本如 `p12-shot.py`、`stats-wheel-check.py`、`stats-ascii-check.py`、`no-wear-check.py`、
  `brightness-slider-check.py` 是歷史路徑，使用前先確認是否存在。
- 0.5 ZIP 與移機包正式檔名見 `RELEASE_0.5.md`、`MAC_0.5.md`、`MIGRATION_0.5.md`。
  本機根目錄未找到 `UW1-zhTW-v0.5-Windows.zip`、`UW1-zhTW-v0.5-macOS.zip`、`UW1-dev-handoff-v0.5.zip`；
  不要把文件中的交付檔名視為此電腦已還原該 ZIP，也不要把根目錄其他舊包當成最新版本。

## 7. 這台電腦的常用指令

Git 用 PowerShell 的 Git for Windows；編譯／測試使用 MSYS2／Git Bash 環境。
兩個 junction 已存在且目的地正確，不要刪除或重建：

- `D:\git\uw1-native-poc` → `D:\git\Ultima Underworld`
- `D:\git\uw1-poc-libs` → `D:\git\Ultima Underworld\underworld-exhumed\uw1\tools\libs`

在 Bash：

```sh
source /d/git/uw1-native-poc/.poc-tools/handoff/env.sh
export PKG_CONFIG_PATH="$UW1/tools/libs/lib/pkgconfig"
export EXHUME_CONFIG="$UW1/exhume.toml"
"$PY" ../exhume/tools/portbuild.py --config exhume.toml
# 需要除錯版時：
"$PY" ../exhume/tools/portbuild.py --config exhume.toml --debug
"$PY" tools/i18n/test_mouse_cursor.py --sanitize
"$PY" tools/i18n/validate_authority.py
"$PY" tools/i18n/proper_names.py --check
"$PY" ../exhume/tools/repocheck.py --root ..
```

`env.sh` 會切到 UW1，並預設舊的 `build/poc/native-v22.toml`；上面明確改用 repo 的 `exhume.toml`。
`PKG_CONFIG_PATH` 是這台電腦找到 FreeType 的必要設定。
重建 catalog 時，明確傳 `--pak "$DATAW/DATA/STRINGS.PAK"`、`--cnv "$DATAW/DATA/CNV.ARK"`，
輸出到新的本機 build 目錄；不要改玩家資料夾。

## 8. 下一步與完成條件

1. 等使用者實測 `unknown-topic`、E 撿起自動堆疊、2026-10-29 地圖橡皮擦輔助（皆需在 F11 開啟），依回報修正。標題頁游標延遲修正已於 2026-10-31 確認 OK。
   強化功能即時生效已於 2026-10-31 確認 OK。`split-all` 已於 2026-10-30 確認 OK。按鍵說明頁已於 2026-10-29 確認 OK。新按鍵已於 2026-10-27 確認 OK。
   Mac 舊 ZIP 不含新按鍵與新功能，需要交付新版時才另行編譯／打包。
2. **P13.2／P13.3**（進行中，使用者邊玩邊測，不算專案未完成因素）：各樓層對話、書本、主線／終局、交易、存讀檔、
   戰鬥、長時間遊玩與譯文品質。依使用者回報修正；需要時可整理第一層對話與書本的本機審閱批次，尚未產生。
3. 只依人工審閱紀錄提升 reviewed；功能與 Mac「測試 OK」不等於全部譯文或全主線已驗收。
4. 2026-10-27 授權的三項（`IDEAS_BACKLOG.md` 第 11～13 項）已於 2026-10-28 實作，待確認。
   尚未授權：打字選話題改選擇、其他密技。「放下物品時才選數量」已依使用者決定刪除。
5. P8 HD 錄影仍暫停，不列 Gate。HD 介面／貼圖／寬螢幕與 UW2 中文化依主計畫另行決策。
6. P13 全部 Gate 與人工驗收完成後，經使用者同意才建立 `zh-tw-native-p13`；不可因 0.5、功能確認或跨平台建置而提前建立。

## 9. 必須保留的工作規則

- 完整英文原文、catalog、遊戲檔、私有權威檔及含這些資料的 ZIP，只留本機，不提交或上傳公開 GitHub。
- 不 reset、不丟棄使用者改動、不修改玩家存檔。`make_play.py` 會刪輸出，不能指向 `UW1-zhTW-playtest`。
  更新玩家測試版只在完成檢查後替換 EXE，保留舊檔並比對 home 雜湊。
- 強化功能預設關；語系不改遊戲狀態或存檔語意。rune、mantra、內部 ID、BABL 不改。
- `src/port/` 以外的原始 `.C/.H/.ASM` 保持純 ASCII，原版掛鉤置於 `#ifndef __TURBOC__`；
  中文放 port。保留原換行，不因編輯把整份舊 C 轉換 CRLF／LF。
- `GAME_TIME()` 會消耗重播事件，呈現動畫只讀 `*Time`；新輸入走既有錄影路徑。
  密技操作回到安全主迴圈生效；測試指令 `run=0` 不得讀遊戲物件，`run=1` 才操作。
- 原版 ATM 註記第一個 NUL 後可能留有垃圾色碼。既有特定情境只在額外語意比較中歸零未使用填充，
  保留原始雜湊，文字、座標與其餘存檔完整比較。見 `P13_CHEATS.md`；不可擴大忽略範圍。
  這次按鍵回歸無須此例外，不代表其他情境也不需要。
- 本機未重跑 DOS 重編譯，不宣稱已通過；主計畫目前採 Native-only，不擅自把 DOS／Borland 重編譯升為當前阻塞條件。
- 語言、字型、專有名詞加註在 F11「語言與文字」。有 catalog 時首啟預設中文，啟動器不要硬帶 locale。
  固定用詞：安卡十字騎士團、月之石預言者、哥布林；暗語／祈禱語／符文保留原文。
- 人名／地名／組織每段首次加英文；`proper-names.tsv` 由工具產生，改譯文後檢查。
- PowerShell 中文腳本用 UTF-8 檔，多行 commit 用 `-F`。不用 MSYS timeout 包 clang；
  `--home` 用 Windows 反斜線絕對路徑。960×720 截圖對 320×200：x 除 3、y 除 3.6。mdown／mup 分開事件，`P12_EXTRA` 不用就 unset。
- 功能修改收尾：必要建置 → 對應單元／情境 → 四模式存檔 → 必要文字回歸／ASCII／repocheck →
  更新報告與兩份交接 → commit、push → 確認 CI。文件修改只做適當的文件與一致性檢查。

換電腦時另讀 `MIGRATION_0.5.md`：Git 不含所有私人資源、工具鏈與舊輸出；新電腦要重建 Python venv／MSYS2，
不可沿用指向舊使用者目錄的虛擬環境。先 clone，再還原私人覆蓋資料；確認可編譯、讀存檔後再整理舊機。
`打包測試版流程.md` 與 `uw1/docs/i18n/PLAYTEST_PACKAGING.md` 也是同步副本，涉及打包時再核對。
