# 交接：UW1 繁體中文化（2026-10-24）

換電腦繼續做專案用。內容：進度、要帶走的檔案、新電腦的設定、常用指令、規則，以及下一步。

## 1. 現況

| 項目 | 狀態 |
| --- | --- |
| 主 repo | `pmanyeh/underworld-exhumed`，分支 `feature/zh-tw-native`，最新 `5fa366f`，已 push |
| 子模組 exhume | `pmanyeh/Exhume`，分支 `feature/presentation-overlay`，最新 `56f128a`，已 push |
| 本機未 commit 的改動 | 無 |
| 最後一個 tag | `zh-tw-native-p10a-batch1`（P11 的 tag `zh-tw-native-p11` 還沒建，見第 6 節） |

### 已完成

- **P0–P9：**
  - 原生 port 基礎
  - 執行期翻譯查詢
  - UTF-8 與中文排版
  - HD 疊加層、FreeType 字型、HD 游標
  - 中文訊息捲軸
  - Bragit 對話試作
- **P10 遊戲文字翻譯：**
  - 對話 P10B-1～22，加上其餘字串 P10C；
  - 整體覆蓋率 97.54%；
  - 譯文全部是 `draft`，還沒有人工審閱紀錄。
  - 進度見 [P10B_PROGRESS.md](P10B_PROGRESS.md)、[P10C_REPORT.md](P10C_REPORT.md)。
- **P10T：** 中文字型、大小、間距可以設定，見 [P10T_TEXT_SETTINGS_REPORT.md](P10T_TEXT_SETTINGS_REPORT.md)。
- **P11：**
  - P11.1 對話模板句：[P11_1_TEMPLATES.md](P11_1_TEMPLATES.md)。
  - P11.2 物品名稱與組句（兩個階段）：[P11_2_NAMES.md](P11_2_NAMES.md)。查看、狀態、食物、修理、使用物品、讀書、容器、神殿誦唸祈禱語，都改成中文組句。
  - P11.4 中文輸入法與同義詞對照：[P11_4_INPUT_ALIASES.md](P11_4_INPUT_ALIASES.md)。
  - 開場與過場動畫字幕：[P11_SUBTITLES.md](P11_SUBTITLES.md)。
- **發行打包：**
  - GitHub Actions `uw1-release` 已經能編 Windows、Linux 和 Mac universal（Mac 的 FreeType 用靜態連結）。
  - Windows 版使用者實測 OK。
  - Mac 版由朋友測試中。
  - 依使用者決定，進度多一些之後再重編 Mac 版。

### 每次改完都要通過的檢查（目前全部 PASS）

- `tools/i18n/test_*.py` 單元測試；
- 16 個公開 session 在英文與 zh-TW HD 模式重播，存檔兩者相同，也與 DOS golden 相同；
- 遊戲原始碼（非 port 的 `.C`／`.H`）維持純 ASCII。

## 2. 帶到新電腦的東西

工作區是 `D:\git\Ultima Underworld`（下稱「工作區」）。腳本與設定檔用的是不含空白的捷徑路徑：

- `D:\git\uw1-native-poc` → 工作區
- `D:\git\uw1-poc-libs` → `工作區\underworld-exhumed\uw1\tools\libs`

**最簡單的做法：** 把整個工作區複製過去（含隱藏的 `.poc-*` 資料夾），再建這兩個捷徑。

**git 拿不到、一定要帶的（無法重建，或重建很麻煩）：**

| 位置（工作區下） | 內容 |
| --- | --- |
| `.poc-private\game\UW` | GOG 版 UW1 遊戲資料。重播、測試都指向這裡 |
| `underworld-exhumed\uw1\i18n\zh-TW\private-authority\` | 手冊翻譯權威檔（本機私有，git 忽略）；`l10n.py authority` 檢查要用 |
| `underworld-exhumed\uw1\build\poc\native-v22.toml`、`replay.toml` | 重播設定（`EXHUME_CONFIG`） |
| `underworld-exhumed\uw1\build\i18n\` | 各批的 `translations.py` 與中間檔；`p11-2b` 是目前重播檢查用的 catalog |
| `underworld-exhumed\uw1\build\replay\verify-en-p6\` | 遊戲內測試（`p11-2-game.py`）用的存檔 |
| `.poc-tools\*.py` | 批次翻譯與檢查腳本（`p10b-batch.py`、`p11-1-check.py`、`p11-2-game.py` 等） |
| `.poc-tools\handoff\env.sh`、`make_play.py` | 建置與重播的環境變數、打包測試資料夾的腳本 |
| 工作區根目錄的 `ULTIMA_UNDERWORLD_NATIVE_ZHTW_HD_MASTER_PLAN_v2.5.md` | 主計畫書（不在 git 裡） |
| 工作區根目錄的 `Translation_Guide.md`、`手冊翻譯權威檔.md` | 翻譯指引 |
| `UW1-zhTW-playtest\home\` | 使用者自己的存檔（SAVE1～3） |

**可以重建、但直接帶比較快的：**

| 位置 | 重建方式 |
| --- | --- |
| `.poc-tools\python` | Python 3，需要 `fontTools`（檢查字型用） |
| `.poc-tools\msys64` | MSYS2 的 clang64 工具鏈（clang、cmake、ninja、freetype、pkgconf） |
| `uw1\tools\libs` | `make setup-libs`（SDL3、libmt32emu） |
| `uw1\tools\fonts` | `jf-openhuninn-2.1.ttf`、`NotoSansCJKtc-Regular.otf` |
| `uw1\tools\nuked-opl3` | `make setup-sound` |

**不用帶：**
- `mac-artifact\`、各個 `*.zip` 測試包，需要時可以重新產生；
- `uw1\build\port\`，編譯產物。

**Claude 的記憶：**
- 檔案在 `C:\Users\pmany\.claude\projects\d--git-Ultima-Underworld\memory\`，內容是兩條規則：回覆用繁體中文、翻譯連續做不停批。
- 新電腦上，專案資料夾的名稱會依工作區路徑產生。工作區路徑一樣的話，直接複製這個資料夾；不一樣的話，請 Claude 依第 5 節重新記下。

## 3. 新電腦設定

```bat
:: 1. 工作區（整個複製過來，或 clone 後再補第 2 節的檔案）
git clone --recurse-submodules -b feature/zh-tw-native https://github.com/pmanyeh/underworld-exhumed.git "D:\git\Ultima Underworld\underworld-exhumed"
cd /d "D:\git\Ultima Underworld\underworld-exhumed\exhume" && git checkout feature/presentation-overlay

:: 2. 捷徑（系統管理員不必要；junction 一般使用者就能建）
mklink /J D:\git\uw1-native-poc "D:\git\Ultima Underworld"
mklink /J D:\git\uw1-poc-libs "D:\git\Ultima Underworld\underworld-exhumed\uw1\tools\libs"
```

注意：exhume 子模組 clone 下來會停在固定的 commit，要切到 `feature/presentation-overlay` 分支才能 commit。

**確認環境**（Git Bash）：

```sh
source /d/git/uw1-native-poc/.poc-tools/handoff/env.sh      # 會 cd 到 uw1
"$PY" ../exhume/tools/portbuild.py --config exhume.toml     # 應看到 third-party: ... FreeType HD text
for t in tools/i18n/test_*.py; do "$PY" $t | tail -1; done  # 全部 OK／PASS
"$PY" /d/git/uw1-native-poc/.poc-tools/p11-1-check.py p11-2b # all_saves_match: true
```

## 4. 常用指令

都在 `source env.sh` 之後、`uw1` 目錄下執行。

| 做什麼 | 指令 |
| --- | --- |
| 編譯 | `"$PY" ../exhume/tools/portbuild.py --config exhume.toml` → `build/port/uw1port.exe` |
| 批次翻譯：準備 | `"$PY" /d/git/uw1-native-poc/.poc-tools/p10b-batch.py prep NAME` |
| 批次翻譯：寫入與檢查 | 編好 `build/i18n/NAME/translations.py`（`T`、`GRAMMAR`、`WAIVERS`、`NOTES`，可選 `HANDLINGS`）後，執行 `p10b-batch.py fill NAME`。會跑 validate、authority、layout_qa、build 兩次、coverage，證據寫到 `docs/i18n/evidence/NAME` |
| 重播檢查 | `"$PY" /d/git/uw1-native-poc/.poc-tools/p11-1-check.py <catalog 所在批次>`，另可再跑 `tools/i18n/verify_localized_replay.py` |
| 遊戲內測試 | `p11-2-game.py NAME SHOT_MS '9000 click 147 142 left' '20000 cheat mantra' ...` |
| 打包測試資料夾 | `"$PY" .poc-tools/handoff/make_play.py OUT CATALOG GAME_DIR` |

遊戲內測試的補充：
- 開啟測試指令要設 `EXHUME_TEST_CHEATS=1`，可用的指令有 `look`、`lookwall`、`give`、`talk`、`mantra`、`goto`、`set`、`scroll`。
- 9000 毫秒那一下是點「Journey Onward」進遊戲。
- 時機不固定，偶爾會沒進去，重跑即可。
- 滑鼠按下和放開要寫成兩個事件。

打包腳本 `make_play.py` 會先刪掉 OUT 資料夾再重建：
- 要保留的存檔請先移出來；
- 只換執行檔的話，直接複製 `uw1port.exe` 就好。

**Mac 版：** 到 GitHub 執行 `gh workflow run uw1-release.yml --repo pmanyeh/underworld-exhumed --ref feature/zh-tw-native`，完成後下載 artifact `macos`。
- 打包檔裡沒有中文檔案；
- `catalog.uw1loc` 含英文原文，不可上傳，要用本機的 `UW1-zhTW-mac-kit` 合併。

## 5. 規則與使用者決定（一定要遵守）

**授權與安全**
- 完整英文原文只留在本機，不 commit。evidence 只放 key 與計數，用 LF 換行。
- 不可自動把譯文標成 `reviewed`；`reviewed` 只能來自人工審閱紀錄。
- 不改 rune、mantra、內部 ID、BABL 語意、存檔語意。
- UTF-8 不可進入原版 VM 的 buffer。
- 遊戲原始碼維持純 ASCII。中文片語放在 `src/port/i18n/phrases.c`，遊戲程式只用 key 引用。
- 不 reset、不丟棄使用者的改動。

**使用者決定**
- 翻譯連續做下去，不逐批停下等確認，也不產生審閱工作表（2026-10-23）。
- 騎士團統一叫「安卡十字騎士團」。
- Seers of the Moonstone 兩處都譯「月之石預言者」。
- 中文輸入法加同義詞對照（P11.4）是發行前必做，已完成。
- 書裡的暗語、祈禱語、符文維持英文。玩家要照英文輸入，例如 Folanae、AAM。
- Mac 版等進度多一些再重新編譯。
- 回覆一律用繁體中文（台灣用語）。

**技術上的注意事項**
- 在 Git Bash 的 heredoc 裡，`\\n` 會被吃掉。要寫含反斜線的 C 字串時，改用 Edit 工具，或在 Python 裡用 `chr(92)`。
- 有些檔案是 CRLF 換行（例如 `SKILLS.C`），有些是 LF。修補前先確認；`.sh` 一律 LF。

## 6. 下一步

1. **P11 收尾：**
   - 確認 Acceptance Gate P11（主計畫書 §Phase 11）；
   - 經使用者同意後，建 tag `zh-tw-native-p11`。
2. **P12：處理硬寫字與點陣 UI，並接上 HD UI**（主計畫書 §Phase 12）：
   - 主選單、角色建立、存讀檔介面、`[MORE]`、狀態與技能面板、背包與人物欄的文字；
   - 對話畫面標題的 NPC 名稱；
   - 結局畫面（`game_stats`，「A level N 職業」「after D days in the Abyss」）；
   - 遊戲裡選語言（English／繁體中文），設定不寫進存檔；
   - 文字顯示參數的設定介面（細節要和使用者討論）。
3. **還是英文的小地方：**
   - 存檔選單的其他字；
   - 遊戲標題字串；
   - 玩家自己的名字；
   - 內部錯誤與除錯訊息。
4. **發行前要補的事：**
   - Mac 打包的 `licenses` 還沒附 FreeType 授權（FTL）；
   - `--data` 直接指定 GOG 資料夾時不會讀 `game.gog`，但 `--help` 說會（不指定 `--data` 時的自動搜尋是正常的）；
   - 譯文全部還是 `draft`，要等人工審閱。
5. **P13：** 完整遊玩 QA，在遊戲裡實際檢查譯文是否合適。
