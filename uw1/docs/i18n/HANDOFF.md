# 開發交接：UW1 繁中整合版 0.5（2026-10-27）

這份文件是換電腦與新對話的入口。先讀本文件，再讀 `uw1/docs/i18n/P13_QA.md`、`IDEAS_BACKLOG.md`。
工作區根目錄 `HANDOFF.md` 與 repo 的 `uw1/docs/i18n/HANDOFF.md` 必須同步；
`打包測試版流程.md` 與 `uw1/docs/i18n/PLAYTEST_PACKAGING.md` 也必須同步。

## 1. 目前停在哪裡

| 項目 | 狀態 |
| --- | --- |
| 本次交付 | **繁中整合測試版 0.5**：Windows x64、macOS 通用版（arm64／x86_64）；版本號不代表 P13 已驗收 |
| 主 repo | `pmanyeh/underworld-exhumed`，分支 `feature/zh-tw-native` |
| 最新功能提交 | `6b00da4`：技能清單滾輪、補正滾輪錄影；完整文件提交以 git log 為準 |
| Exhume 子模組 | `pmanyeh/Exhume`，分支 `feature/presentation-overlay`，固定 `5e92215` |
| 翻譯 catalog | `uw1/build/i18n/p13-goblin/catalog-1.bin`，不是公開 Git 的一部分 |
| 翻譯進度 | 可翻譯 6,983 筆、已翻 6,893 筆（98.71%）；其餘 90 筆為比對文字，譯文全部仍是 draft |
| 開發階段 | **P13 進行中**：自動檢查已完成，仍待實際遊玩與人工譯文審閱 |
| 最後階段 tag | `zh-tw-native-p12`（`0169ca1`）；本次不建立 P13 驗收 tag |
| 玩家的資料夾 | `UW1-zhTW-playtest`，保留原存檔；不得用重新打包程式覆蓋 |

本次檔案：

- `UW1-zhTW-v0.5-Windows.zip`：乾淨的 Windows 整合測試包，沒有 home、遊戲本體或寫死的資料路徑。
- `UW1-zhTW-v0.5-macOS.zip`：GitHub Actions 編譯後於本機合併中文檔的 Mac 通用版；校驗檔 `UW1-v0.5-macOS-SHA256SUMS.txt`。
- `UW1-dev-handoff-v0.5.zip`：**私人移機資料**，含 Git bundles、必要的 Git 外資料、玩家 home 備份及移機說明。
- `UW1-v0.5-SHA256SUMS.txt`：兩包的 SHA-256。全部保留在本機，不上傳含 catalog／原版資料的 ZIP。
- `uw1/docs/i18n/RELEASE_0.5.md`：本次來源、功能與驗證摘要；執行檔／catalog 來源雜湊另見整合包的 `MANIFEST.json`。

### 0.5 本機交付驗證

- 17 個單元程式、權威／專有名詞檢查、16 個四模式 session 全部通過。
- 新包 32 檔（含 manifest），SHA-256／ZIP CRC 檢查通過；拒絕覆蓋舊資料夾的保護已實測。
- 在只有 Windows 系統 PATH 的環境，實際執行 BAT：首次中文設定、中英文讀檔、HD 數值和技能滾輪正常；相對 UW 資料夾可用。
- 私人移機包已以 Git bundles 離線還原到另一個暫存工作區，重新編譯並通過全部 17 個單元程式；原生依賴使用還原副本。
  本機共用既有編譯器／Python 來驗證，新電腦仍須依說明重裝工具鏈。
- 原玩家 home 的 141 個檔案、其中 33 個存檔檔案雜湊不變；備份與原檔一致。
- 私有權威資料已加入 repo 的 `.gitignore`，新 clone 不再依賴舊機的本機 exclude。

## 2. 已完成、已確認與待確認

| 功能 | 現況／報告 |
| --- | --- |
| P0～P12 中文化、HD 文字／游標、中文輸入、數量滑桿、中文姓名與存檔說明 | 已實作；`P12_UI.md`、`P13_QA.md` |
| 戰鬥傷害數字／敵人血條、血瓶法力瓶數值、自動開鎖、中鍵滑鼠視角、C 戰鬥／X 關容器、E 撿起 | 使用者已確認原有功能 OK；`IDEAS_BACKLOG.md`、`P13_COMBAT.md` |
| 地圖中文註記 | 使用者已確認 OK；`P13_MAP_NOTES.md` |
| 滑鼠滾輪：袋子、「繼續」、屬性頁技能清單 | 使用者已確認 OK；技能清單於 2026-10-26 確認，`P13_STATS_WHEEL.md` |
| 無敵預開啟讀檔閃退 | 已修正，使用者已確認解除；`P13_CHEATS.md` |
| 個體血量作為血條最大值 | 使用者已確認 OK；`P13_COMBAT.md` |
| 提高亮度、0～15 增亮強度、自然明暗漸層 | 使用者已確認結案；`P13_CHEATS.md` |
| 敵人攻擊時亮血條、物件不損耗、Z 開關地圖、其餘已實作密技安全情境 | 使用者已於 2026-10-27 確認 OK；`P13_COMBAT.md`、`P13_CHEATS.md` |
| 中文模式屬性頁數值／英文姓名、地圖英數註解 HD 字型 | `8d5562d`，使用者已於 2026-10-27 確認 OK；`P13_HD_ASCII.md` |
| Mac 繁中整合測試版 0.5 | 使用者已於 2026-10-27 回報實機測試 OK；`MAC_0.5.md` |
| 現代按鍵：空白鍵跳躍、X 僅關容器、切換按鍵每次按下只觸發一次 | 2026-10-27 依使用者新決定修改；驗證與交付狀態見 `P13_MODERN_KEYS.md`，待使用者確認 |

最近的實作提交：`cc4e3f0` 地圖中文、`822b5bf` 密技首版、`d1e37ea` 無敵載入修正、
`820a1be` 攻擊亮血條、`3a6bc51` 自然照明／個體 HP、`b353ef0` 亮度滑桿、
`3c4c9e1` 物件不損耗、`8d5562d` 英數 HD、`6b00da4` 技能滾輪。
歷史實作細節和驗證數字以對應報告為準，不要把舊報告當成新的待做項目。

**按鍵新決定（2026-10-27，取代 2026-10-26 的空白鍵結案決定）：** 「現代按鍵」空白鍵改為跳躍，取消沿用上次攻擊方式。
Shift／j 跳躍與 Ctrl+Shift／大寫 J 的另一種原版跳躍保留；攻擊仍可用滑鼠與 `.`／`;`／`p`。
`rune-keys` 的 Ctrl+Alt+空白鍵仍用於施法。X 移除慢速前進，只關閉容器；C 等切換按鍵按住不連續觸發。

## 3. 換電腦：先帶走什麼

程式碼與公開測試由 Git 還原；私人移機包另保存以下 Git 外資料：

- `.poc-private/`：自己的 GOG UW1 資料與本機資料；私有原文不可 commit。
- `uw1/i18n/zh-TW/private-authority/`：私有翻譯權威資料。
- `.poc-tools/` 第一層腳本、`handoff/`：本機驗證工具和環境設定。
- `uw1/build/poc/*.toml`、翻譯批次的 `translations.py`、最新 catalog／manifest、必要摘要報告。
- `uw1/build/replay/verify-en-p6/` 下的 SAVE 測試資料、`tools/fonts`、`tools/libs`、`tools/nuked-opl3`。
- 根目錄計畫書／指引、`.poc-evidence`、`translation`、`Game Resources`，以及 `UW1-zhTW-playtest/home` 備份。

移機包不是工作區逐檔備份；**不含約 48 GB 舊測試輸出、MSYS2、Python 虛擬環境、舊 Mac 包或 Borland 工具鏈**。
舊電腦先保留，確認新電腦可編譯、讀存檔後再自行整理。
原 `.poc-tools/python` 是虛擬環境，指向舊電腦的使用者目錄；必須在新電腦重建，不可直接沿用。

### 新電腦步驟

建議維持 `D:\git\Ultima Underworld`；多支歷史腳本仍使用下列 junction 路徑。
若磁碟配置不同，先調整 junction 與 `.poc-tools/handoff/env.sh`，勿以為舊腳本已全部可攜化。

1. 解壓私人移機包到暫存位置，閱讀其中 `移機說明.md`。
2. **先 clone repo，再還原 workspace 私人覆蓋資料**；不要先把私人資料放進待 clone 的 repo 目錄。
3. 可在線 clone，或使用移機包內的兩個 Git bundle 離線還原。完成後把 origin 設回 GitHub。
4. 重建 Python 3.12 虛擬環境，依移機包的 `requirements.txt` 安裝；安裝 MSYS2 CLANG64，照 repo 的 `uw1/docs/BUILDING.md` 執行 `make setup-port`。
5. 依下方建立 junction，確認兩個 repo 的分支與 HEAD；先編譯和跑測試，再繼續開發。
6. 玩家 home 從備份另行還原，不要拿乾淨測試包覆蓋現有 home。保留所有 SAVE 資料夾及 NAME.UTF／DESC.UTF／NOTES.UTF。

在線還原（Windows 命令提示字元；Git 請使用 Git for Windows）：

```bat
git clone --recurse-submodules -b feature/zh-tw-native https://github.com/pmanyeh/underworld-exhumed.git "D:\git\Ultima Underworld\underworld-exhumed"
git -C "D:\git\Ultima Underworld\underworld-exhumed\exhume" checkout feature/presentation-overlay
mklink /J D:\git\uw1-native-poc "D:\git\Ultima Underworld"
mklink /J D:\git\uw1-poc-libs "D:\git\Ultima Underworld\underworld-exhumed\uw1\tools\libs"
```

Windows 兩個 repo 都設 `git config core.autocrlf true`，並在新機設定自己的 Git 身分與 GitHub 登入（不搬移憑證）。已有 junction 時先查目的地，不可直接刪除或覆蓋。
MSYS2 可以安裝到 `.poc-tools/msys64`，或讓該位置連到實際安裝目錄。

## 4. 接手後的驗證與常用工具

Git Bash，工作區 junction 已建立：

```sh
source /d/git/uw1-native-poc/.poc-tools/handoff/env.sh
"$PY" ../exhume/tools/portbuild.py --config exhume.toml
"$PY" tools/i18n/validate_authority.py
"$PY" tools/i18n/proper_names.py --check
"$PY" /d/git/uw1-native-poc/.poc-tools/v05-check.py units
"$PY" /d/git/uw1-native-poc/.poc-tools/v05-check.py replays
```

| 用途 | 指令／位置 |
| --- | --- |
| 除錯版 | `"$PY" ../exhume/tools/portbuild.py --config exhume.toml --debug` |
| 文字 sanitizer／SDL 回歸 | `"$PY" /d/git/uw1-native-poc/.poc-tools/p10a-b1-text-regression.py` |
| 四模式存檔檢查 | `v05-check.py replays`；16 個公開 session，英文關閉模式要與 DOS golden 原始位元組相同 |
| repo 檢查 | 在主 repo 跑 `"$PY" exhume/tools/repocheck.py --root .` |
| 截圖 | `.poc-tools/p12-shot.py NAME SHOT_MS --save '事件' ...`；中文正常、`P12_EN=1` 英文 |
| 最新功能情境 | `.poc-tools/stats-wheel-check.py`、`stats-ascii-check.py`、`no-wear-check.py`、`brightness-slider-check.py`，各支可查看 `--debug` 等選項 |
| 新打包工具 | `uw1/tools/i18n/package_playtest.py`，拒絕覆蓋已存在資料夾；完整指令見 `打包測試版流程.md` |

**存檔比對的明確例外：** 原版 ATM 地圖註記第一個 NUL 後可能留有畫面垃圾色碼；四模式測試只在額外語意比較中
歸零這段未使用填充，文字、座標及其餘存檔仍完整比較，原始雜湊另存。見 `P13_CHEATS.md`。
不要再把舊 `p11-1-check.py` 的所有原始位元組必須相同，直接套到目前的四模式測試。
本機 DOS 重編譯 Gate 缺 Node.js，**尚未驗證**；不能用原生測試宣稱 DOS 重編譯已通過。

## 5. 規則、陷阱與已知限制

- **一律用繁體中文（台灣用語）回覆。** 使用者熟遊戲，說明講結果，不堆程式術語。
- 完整英文原文、catalog、遊戲檔、私有權威檔只留本機；ZIP 不可 commit 或上傳公開 GitHub。
- 不自行把 draft 改成 reviewed；人工審閱紀錄才可變更。rune、mantra、內部 ID、BABL 與存檔語意不改。
- 原始 `src/` 中 `port/` 以外的 `.C/.H/.ASM` 保持純 ASCII。中文放 port；原版掛鉤用 `#ifndef __TURBOC__`。
- 強化功能預設關；中文與英文操作的遊戲狀態相同。密技回到安全的遊戲主迴圈套用，錄製公用指令。
- 不 reset、不丟棄使用者改動、不修改玩家存檔。`make_play.py` 會刪輸出，不可指向 `UW1-zhTW-playtest`。
- `GAME_TIME()` 會消耗重播事件：呈現動畫只讀 `*Time`。新輸入要經原有錄影路徑，避免滾輪漏記事件重演。
- 測試密技會先以 `run=0` 檢查參數，這時不可讀遊戲物件；`run=1` 才操作。
- Git 一律由 PowerShell 呼叫 Git for Windows；Git Bash source env 後的 MSYS git 不認使用者身分。
- PowerShell 5.1 的中文字串和引號易損壞，中文腳本寫成 UTF-8 檔再執行；多行 commit 用 `-F`。
- 保留原換行，不要因編輯把整份舊 C 的 CRLF／LF 轉換。Git show 的 LF 與 checkout 的 CRLF 不一定相同。
- 不用 MSYS `timeout` 包 clang（會弄掉 TMP／TEMP）；`--home` 需使用 Windows 反斜線絕對路徑。
- 截圖 960×720 對遊戲 320×200：x 除 3、y 除 3.6。mdown／mup 分開事件；`P12_EXTRA` 不用就 unset。
- 中譯字型、專有名詞加註等設定在 F11「語言與文字」；有 catalog 時首啟預設中文，啟動器不要硬帶 `--locale`。
- 保留既定用詞：安卡十字騎士團、月之石預言者、哥布林（Goblin）。暗語／祈禱語／符文維持原文。
- 專有名詞人名／地名／組織在每段首次加英文，`proper-names.tsv` 由工具產生，改譯文後執行檢查。
- Mac 0.5 已由 GitHub Actions 編譯：run `40308423602`，來源 `4c662c1`。雙架構、簽章完整性、函式庫路徑與 --help 檢查通過；未經 Apple 公證。
  新獨立 Mac 0.5 包保留 app 位元組與 Unix 權限，啟動器依當前資料夾自動找中文，不固定 locale。
  使用者已於 2026-10-27 回報 Mac 0.5 實機測試 OK。Actions 完整私有資源測試仍因 SSH 金鑰格式／權限失敗而未執行。舊 Mac kit 未覆寫。報告 `MAC_0.5.md`。

## 6. 下一步

1. 使用者在新電腦完成還原和驗證後，依回報修問題。不要自動重做已確認功能。
2. P13.2／P13.3 人工遊玩與譯文審閱：各樓層對話、書本、交易、存讀檔、戰鬥與長時間標籤殘留。
3. 既有功能與 Mac 0.5 實機測試已由使用者確認；2026-10-27 新按鍵修改待確認，見 `P13_MODERN_KEYS.md`。仍保留全遊戲人工 QA 與譯文審閱。
4. 候選功能（未授權動工）：打字選話題改選擇、錯字反應、放下物品時才選數量、其餘密技。
5. P8 HD 錄影仍暫停，不列 Gate；HD 介面／貼圖／寬螢幕與 UW2 中文化另待決策，見 modernization 稽核與主計畫。
6. P13 全部 Gate 與人工驗收完成後，**經使用者同意**才建立 `zh-tw-native-p13` tag。

功能變更收尾：正常／必要除錯建置 → 對應單元與情境 → 16 個四模式存檔 → 必要文字回歸／ASCII／repocheck →
更新報告與交接副本 → commit、push → 確認 CI。只更新舊測試版時複製 EXE，更新前後比對玩家存檔雜湊。
