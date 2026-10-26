# 交接：UW1 繁體中文化與遊戲體驗強化（2026-10-26）

給新的對話 Session 或從沒碰過這個專案的 Agent 接手用。照第 0 節的順序讀，就知道現在在哪、下一步做什麼、怎麼做。
換電腦時看第 2、3 節。

本文件在工作區根目錄（`D:\git\Ultima Underworld\HANDOFF.md`）；repo 裡的 `uw1/docs/i18n/HANDOFF.md` 是同一份的副本，
打包流程 `打包測試版流程.md` 的副本是 `uw1/docs/i18n/PLAYTEST_PACKAGING.md`。**改了要把副本一起更新再 commit**；
換電腦、根目錄的檔案不見時，從 repo 的副本複製出來。

## 0. 第一次接手：先做這幾件事

1. **回覆一律用繁體中文（台灣用語）。** 使用者是專案擁有者，不是程式設計師出身，但很熟這款遊戲；說明要白話、
   講結果，不要堆術語。
2. **這是什麼專案：** 把 1992 年的 DOS 遊戲 *Ultima Underworld*（UW1）的原生 port（`underworld-exhumed`，
   遊戲原始碼逐函式還原成 C，可以在 Windows／Mac／Linux 原生執行）做成繁體中文版，並加上可開關的現代化
   「強化功能」。玩家要自備 GOG 版遊戲資料。
3. **讀這三份：** 本文件 → `underworld-exhumed/uw1/docs/i18n/IDEAS_BACKLOG.md`（強化功能清單與做法）→
   `underworld-exhumed/uw1/docs/i18n/P13_QA.md`（目前階段的報告）。
4. **確認環境能跑**（第 3 節最後的四行指令）。全部通過才開始改東西。
5. **使用者的 Token 有限**：做完一個段落就 commit、push，並更新本文件第 1、6 節，讓下一個人接得上。

## 1. 現況

| 項目 | 狀態 |
| --- | --- |
| 主 repo | `pmanyeh/underworld-exhumed`，分支 `feature/zh-tw-native`，最新程式提交 `b353ef0`，已 push（後續交接文件提交見 git log） |
| 子模組 exhume | `pmanyeh/Exhume`，分支 `feature/presentation-overlay`，最新 `5e92215`，已 push |
| 本機未 commit 的改動 | 無 |
| 最後一個 tag | `zh-tw-native-p12`（在 `0169ca1`，2026-10-25） |
| 目前階段 | **P13 進行中**：中文化的自動化檢查都完成，等使用者實際遊玩與審閱譯文；同時在做使用者要的強化功能 |
| 測試資料夾 | `UW1-zhTW-playtest`：執行檔已含增亮強度 0～15 滑桿（`b353ef0`）、照明漸層與個體血量基準修正（`3a6bc51`），及攻擊觸發敵人血條（`820a1be`）、提高亮度（`d11a79b`）、無敵預開啟讀檔修正（`d1e37ea`）及密技選單首版（`822b5bf`）：全地圖、無敵與安全位置救援，以及先前中文註記、Z 地圖與戰鬥顯示功能，catalog 為 `p13-goblin`，`開始遊戲.bat` 不帶 `--locale` |

### 已完成

- **P0–P9：** 原生 port 基礎、執行期翻譯查詢、UTF-8 與中文排版、HD 疊加層、FreeType 字型、HD 游標、中文訊息捲軸。
- **P10：** 遊戲文字翻譯（對話 P10B-1～22、其餘 P10C）；P10T 中文字型與參數可設定。
- **P11（tag `zh-tw-native-p11`）：** 對話模板句、物品名稱與組句、中文輸入法與同義詞、過場字幕。
- **P12（tag `zh-tw-native-p12`）：** 報告 `uw1/docs/i18n/P12_UI.md`。HD 標籤層（點陣字與圖片上的英文改成 HD 中文）、
  NPC 名稱、`[MORE]`、狀態面板、主選單、角色建立、選項選單、結局畫面、F11 設定畫面中文化（「語言與文字」分頁）、
  數量滑桿、玩家可輸入中文名字與存檔說明（存檔欄位放 ASCII 替代值，中文存在 `SAVE0\NAME.UTF`、`DESC.UTF`）。
- **P13（進行中）：** 報告 `uw1/docs/i18n/P13_QA.md`。
  - P13.1 剩下的英文全部分類；P13.4 語言切換與退回英文；P13.5 四平台 CI、sanitizer、四種模式存檔比對：全部通過。
  - 發行前修正：FreeType 授權進所有平台的包；`--data` 可指向含遊戲或 `game.gog` 的資料夾。
  - 用詞：Goblin(s) 改「哥布林」（灰／綠哥布林、Gobs＝哥布仔、Goblin Home＝哥布林家園），兩份指引檔也已改。
  - 專有名詞加註英文：對話與書本／卷軸中，人名、地名、組織第一次出現顯示「加拉蒙（Garamon）」；
    F11「語言與文字」的「專有名詞加註英文」（預設每段第一次）。名單 `i18n/zh-TW/proper-names.tsv` 由
    `tools/i18n/proper_names.py` 產生，**改了 authority 或譯文要重新產生**（`test_name_gloss.py` 會檢查）。
- **強化功能（使用者 2026-10-25 提出，清單見 `IDEAS_BACKLOG.md`）：**

  | 項目 | 開關（F11「強化功能」分頁） | commit |
  | --- | --- | --- |
  | 1. 戰鬥傷害數字、敵人血條 | `damage-numbers`「戰鬥傷害數字」、`enemy-health`「敵人血條」，兩項獨立、預設關 | `06be938`（exhume `438441b`） |
  | 2. 背包有對的鑰匙就自動開鎖（門與箱子） | `auto-key`「自動開鎖」 | `a37584e` |
  | 3. 滑鼠中鍵切換滑鼠視角 | 原有的 `mouse-look` | `57f777e` |
  | 4. C 鍵切換戰鬥模式（原本 C 的「關容器」改到 X） | 原有的 `modern-keys` | `4c8d2a2` |
  | 5. E 鍵把可拿的東西直接放進背包 | `e-takes`「E 鍵撿起」（要搭配 `modern-keys`） | `a7e8d7d` |
  | 6. 血瓶、法力瓶顯示 25/32 與飄動的增減數字 | `flask-numbers`「血瓶法力瓶數值」 | `771147f` |
  | （追加）滑鼠滾輪：捲動打開的袋子、翻過「繼續」；F11 設定畫面一律可用滾輪捲動 | `mouse-wheel`「滑鼠滾輪」 | `6486815`（exhume `32fd048`） |

  使用者已實測 1～6 項都 OK；滑鼠滾輪、戰鬥傷害數字與敵人血條均已於 2026-10-25 確認 OK；戰鬥報告 `uw1/docs/i18n/P13_COMBAT.md`。
- **Z 開關地圖（2026-10-25，`30d41de`）**：開「現代按鍵」時，地圖上再按 Z 關閉；編寫註記時 Z 仍輸入文字，Enter 結束後才可用 Z 關閉。遊戲內已驗證反覆開關、編寫 ZZ 並保存、Esc 關閉；待使用者實測。截圖 `build/i18n/p12/shots/map-z-*.png`，工具 `.poc-tools/map-z-check.py`。
- **地圖中文註記（P13.7 第 5 項，2026-10-25，`cc4e3f0`）**：使用者已於 2026-10-25 實測確認 OK；中文 HD 模式點地圖即可輸入，Enter 完成、Esc 取消。中文另存 `NOTES.UTF`，隨正常存讀檔複製；英文模式也顯示已有中文。報告 `uw1/docs/i18n/P13_MAP_NOTES.md`。
- **密技選單首版（2026-10-26，`822b5bf`）**：F11「密技」新增全地圖、無敵、記住／返回安全位置，回到遊戲後生效，待使用者實測。報告 `uw1/docs/i18n/P13_CHEATS.md`。
- **提高亮度（2026-10-26，`d11a79b`）**：F11「密技」的 `bright-view`，只調整 3D 明暗，不改光源、探索範圍或存檔；預設關，回到遊戲後生效。測試版已更新，待使用者實測（見 6.2 C）。
- **F11 設定畫面**：項目超過一頁時，右側有捲軸、底部右邊顯示「▼ 下面還有 N 項」（exhume `88ce7f2`，使用者反映原本的小 v 看不到）。
- **覆蓋率：** 可翻譯 6,983 筆，已翻 6,893 筆（98.71%）；剩下 90 筆是比對字，依規則不翻。譯文全部是 `draft`。

### 每次改完都要通過的檢查（目前全部 PASS）

- `tools/i18n/test_*.py` 單元測試（含 `test_name_gloss.py`）；
- 16 個公開 session 在英文與 zh-TW HD 模式重播，存檔兩者相同，也與 DOS golden 相同（`p11-1-check.py p13-goblin`）；
- 遊戲原始碼（`src/` 底下、`port/` 以外的 `.C`／`.H`／`.ASM`）維持純 ASCII（用 Python 逐位元組檢查，第 4 節有指令）；
- 改 authority 時另跑 `tools/i18n/validate_authority.py`（CI 會跑；GUIDE-ALIGNMENT、SUMMARY.json 要跟著改）；
- 改到 HD 文字、呈現層或字串查找時，另跑 `p10a-b1-text-regression.py`（sanitizer 與原版字串探測）；
- push 後看 CI：`gh run list --repo pmanyeh/underworld-exhumed --limit 4`（`uw1-port` 與 `repocheck` 都要綠）。

## 2. 環境

工作區是 `D:\git\Ultima Underworld`。腳本與設定用不含空白的捷徑路徑（junction）：

- `D:\git\uw1-native-poc` → 工作區
- `D:\git\uw1-poc-libs` → `工作區\underworld-exhumed\uw1\tools\libs`

**git 拿不到、一定要留著的：**

| 位置（工作區下） | 內容 |
| --- | --- |
| `.poc-private\game\UW` | GOG 版 UW1 遊戲資料 |
| `underworld-exhumed\uw1\i18n\zh-TW\private-authority\` | 手冊翻譯權威檔（本機私有） |
| `underworld-exhumed\uw1\build\poc\native-v22.toml`、`replay.toml` | 重播設定（`EXHUME_CONFIG`） |
| `underworld-exhumed\uw1\build\i18n\` | 各批 `translations.py` 與 catalog；**`p13-goblin` 是目前最新的 catalog** |
| `underworld-exhumed\uw1\build\replay\verify-en-p6\` | 遊戲內測試用的存檔（`items/port/SAVE1`，第一層） |
| `.poc-tools\*.py`、`.poc-tools\handoff\` | 本機工具（見第 4 節） |
| `ULTIMA_UNDERWORLD_NATIVE_ZHTW_HD_MASTER_PLAN_v2.5.md` | 主計畫書（不在 git 裡） |
| `Translation_Guide.md`、`手冊翻譯權威檔.md` | 翻譯指引（使用者的） |
| `打包測試版流程.md` | 打包測試版的步驟（給其他 Agent 照做） |
| `UW1-zhTW-playtest\` | 使用者的測試資料夾與存檔（SAVE1～3），**不要動存檔、不要用 make_play.py 覆蓋** |
| `UW1-zhTW-mac-kit\` | Mac 版的中文配件 |

**可重建：** `.poc-tools\python`（Python 3 + fontTools）、`.poc-tools\msys64`（clang64 工具鏈）、
`uw1\tools\libs`（`make setup-libs`）、`uw1\tools\fonts`、`uw1\tools\nuked-opl3`（`make setup-sound`）。

**Claude 的記憶**在 `C:\Users\pmany\.claude\projects\d--git-Ultima-Underworld\memory\`：回覆用繁體中文、翻譯連續做不停批、先讀本文件。

## 3. 換電腦時

```bat
git clone --recurse-submodules -b feature/zh-tw-native https://github.com/pmanyeh/underworld-exhumed.git "D:\git\Ultima Underworld\underworld-exhumed"
cd /d "D:\git\Ultima Underworld\underworld-exhumed\exhume" && git checkout feature/presentation-overlay
mklink /J D:\git\uw1-native-poc "D:\git\Ultima Underworld"
mklink /J D:\git\uw1-poc-libs "D:\git\Ultima Underworld\underworld-exhumed\uw1\tools\libs"
```

兩個 repo 都要設 `git config core.autocrlf true`，否則 git 會把上千個 CRLF 檔案顯示成已修改。

確認環境（Git Bash）：

```sh
source /d/git/uw1-native-poc/.poc-tools/handoff/env.sh      # 會 cd 到 uw1，設好 $PY 與 PATH
"$PY" ../exhume/tools/portbuild.py --config exhume.toml     # 應看到 linked build\port\uw1port.exe
for t in tools/i18n/test_*.py; do "$PY" $t | tail -1; done  # 全部 OK／PASS
"$PY" /d/git/uw1-native-poc/.poc-tools/p11-1-check.py p13-goblin # all_saves_match: true
```

## 4. 常用指令與工具

都在 `source env.sh` 之後、`uw1` 目錄下執行。

| 做什麼 | 指令 |
| --- | --- |
| 編譯 | `"$PY" ../exhume/tools/portbuild.py --config exhume.toml` → `build/port/uw1port.exe` |
| 重播存檔比對 | `"$PY" /d/git/uw1-native-poc/.poc-tools/p11-1-check.py p13-goblin` |
| ASCII 檢查 | `"$PY" -X utf8 -c "import pathlib;print([str(p) for p in pathlib.Path('src').rglob('*') if p.suffix.upper() in('.C','.H','.ASM') and 'port' not in p.parts and any(b>127 for b in p.read_bytes())])"`（要印出 `[]`） |
| sanitizer 文字回歸 | `"$PY" /d/git/uw1-native-poc/.poc-tools/p10a-b1-text-regression.py`（約 3 分鐘，每項要 0） |
| 遊戲內截圖 | `"$PY" /d/git/uw1-native-poc/.poc-tools/p12-shot.py NAME SHOT_MS [--save] [--keep-settings] '事件' ...` → `build/i18n/p12/shots/NAME.png` |
| 四種模式的存檔比對 | `.poc-tools/p10-11-saves.py` |
| 批次翻譯 | `p10b-batch.py prep NAME BLOCK...`，寫好 `build/i18n/NAME/translations.py` 後 `p10b-batch.py fill NAME` |
| 傳送到 NPC 測對話 | `.poc-tools/p11-4-ime.py SLOT` |
| 打包測試版 | 見 `打包測試版流程.md`（`make_play.py` **會先刪掉輸出資料夾**） |

**截圖工具 `p12-shot.py` 的用法與眉角**
- `--save`：先讀測試存檔（第一層的起點，面對一面牆），約 9000 毫秒時已在遊戲中，事件從 9000 之後排。
- 開強化功能：`export P12_EXTRA="--enhance modern-keys,e-takes"`。**不用時要 `unset P12_EXTRA`**，設成空字串會讓程式拿到空參數。
- 英文模式：`P12_EN=1`。
- 事件：`key e`、`down 3`／`up 3`（按住／放開）、`move X Y`（游標移到遊戲座標）、`click X Y left|right|middle`、
  `mdown`／`mup`、`look DX DY`（滑鼠視角的移動）、`wheel N`（滾輪 N 格，往下為正）、`cheat 指令`。滑鼠按下與放開要分兩個事件。
- **截圖是 960×720，遊戲座標 320×200：x 除以 3，y 除以 3.6**（4:3 比例校正）。用截圖找位置時別忘了。
- 要看腳下的物品：開 `modern-keys` 後 `down 3` 按住約 1.5 秒往下看。

**測試指令**（只在輸入腳本裡、`EXHUME_TEST_CHEATS=1` 時有效；`src/port/sys/cheats.c`）：
`goto 層 X Y`、`give 物品 [數量 [品質]] [drop]`（**鑰匙的「數量」是它開的鎖號**）、`set hp|mana|str|… 值`、
`talk NPC編號`、`look 物品 [數量 [品質 [描述等級]]]`（**書本／卷軸 0x130～0x13F 的「數量」是文字編號**）、`lookwall`、
`mantra`、`scroll 檔案`（檔案路徑要短，例如 `build/long.txt`，太長會被拒絕；多行文字可測「繼續」）、`endstats`、`use 物件編號 X Y`（直接使用門或家具）。
- 改到 MOUSE.C 或平台層時：`tools/i18n/mouse_cursor_test.c` 與 `exhume/tools/tests/overlay/presentation.c` 是替身檔，新函式要在那裡補空的實作，否則測試連結失敗。
- 寫新的測試指令時注意：**指令在載入腳本時會先以 `run=0` 檢查參數，那時遊戲物件還沒載入**；
  讀物件要放在 `if (!run) return NULL;` 之後，否則程式會當掉（0xC0000005）。
- 找第一層上鎖的門：`.poc-tools` 外的草稿腳本已不在，做法是解析存檔的 `LEV.ARK`（每層 64×64 格的 4 位元組 tile，
  bits 22–31 是第一個物件；物件 0–255 是 27 位元組、256–1023 是 8 位元組；門 0x140–0x145 的 link 指向鎖 0x10F，
  鎖的 link & 0x1FF 是鎖號）。已知：門 725 在 (9,19)，鎖號 2。

**Mac 版：** `gh workflow run uw1-release.yml --repo pmanyeh/underworld-exhumed --ref feature/zh-tw-native`，
下載 artifact `macos`；`catalog.uw1loc` 含英文原文，不可上傳，用本機 `UW1-zhTW-mac-kit` 合併（`打包測試版流程.md`）。

## 5. 規則與使用者決定（一定要遵守）

**授權與安全**
- 完整英文原文只留在本機，不 commit。evidence 只放 key 與計數，用 LF 換行。
- 不可自動把譯文標成 `reviewed`；`reviewed` 只能來自人工審閱紀錄（使用者直接指定的用詞替換可保留原狀態並加註記）。
- 不改 rune、mantra、內部 ID、BABL 語意、存檔語意。UTF-8 不可進入原版 VM 的 buffer。
- 遊戲原始碼維持純 ASCII。中文放在 `src/port/i18n/phrases.c`、`settings_words.c`，遊戲程式只用 key 或掛鉤。
- 中英文模式玩同一段操作，存檔必須相同。原版沒有的功能一律做成可開關的「強化功能」，預設關。
- 不 reset、不丟棄使用者的改動；不動使用者的存檔。

**使用者決定**
- 翻譯連續做下去，不逐批停下等確認（2026-10-23）。
- 騎士團統一叫「安卡十字騎士團」；Seers of the Moonstone 兩處都譯「月之石預言者」；Goblin(s)＝哥布林（2026-10-25）。
- 書裡的暗語、祈禱語、符文維持英文（例如 Folanae、AAM）。
- 專有名詞加註英文：人名、地名、組織；對話與書本／卷軸，一般訊息不加（2026-10-25）。預設「每段第一次」，待使用者確認。
- 強化功能（2026-10-25）：
  - 戰鬥數字與血條：**預設關**；英文模式也顯示（Block、Miss）；**玩家自己受傷的數字顯示在血瓶旁**（已由 `flask-numbers` 做到），不在 3D 畫面上。
  - C 鍵給戰鬥模式，關容器改 X。
  - E 鍵：對著可撿的物品撿起，對著門、開關等仍是使用。
  - 全地圖：**只在畫地圖時當成全開，不寫進存檔**。
- P8 HD 錄影暫停、不作為 Gate，恢復時機由使用者決定（2026-10-23）。
- 設定畫面：語言與文字參數放在 F11 的「語言與文字」分頁；設定畫面本身也顯示中文（2026-10-24）。
- 玩家的中文名字與存檔說明：存檔欄位放 ASCII 替代值，中文另存旁註檔（2026-10-24）。
- Mac 版等進度多一些再重新編譯。
- 待使用者確認的譯名：主選單（開場介紹／創造角色／製作群／繼續旅程）、選項選單、設定畫面用語、「名叫華倫的幽靈」。

**技術注意事項**
- **git 一律用 PowerShell 裡的 Git for Windows。** `source env.sh` 之後 Git Bash 的 `git` 是 MSYS2 的，讀不到使用者身分。
  若 PowerShell 找不到 git，用完整路徑 `& "C:\Program Files\Git\cmd\git.exe"`，或 VS Code 的「原始檔控制」。
- **commit 訊息含雙引號時**，PowerShell 5.1 傳給 git 會被拆開：用多個 `-m`，或寫進檔案再 `git commit -F 檔案`。
- **工具的自動權限檢查偶爾連續失敗**（顯示 classifier no verdict）：不是指令本身有問題，等一下再試；
  失敗前的指令可能其實已經執行（曾發生 commit 成功但沒回報）。重試前先看 `.git/logs/HEAD` 或 `git log`。
- **Git Bash 的 heredoc 會吃掉 `\\`**：含反斜線的 C 字串改用 Edit 工具；中文透過 heredoc 傳給 Python 也會轉碼。
- **MSYS 的 `timeout` 會拿掉 `TMP`／`TEMP`**，clang 因此失敗；跑測試時不要包 `timeout`。
- **遊戲原始碼多半是 CRLF**（`.C`、`main.c`、`settab.c`、`cheats.c`…），有些是 LF（`BABL.C`、`presentation.c`、新檔）。
  用 Edit 工具改會保留原換行；改完可用 `grep -c $'\r$' 檔案` 與 `wc -l` 對照確認。
- `--home` 要用反斜線路徑（`C:\...`）；`C:/...` 形式在 `files.c` 的 `mkdirs` 會失敗。
- exhume 的 `setcheck.py` 在這台電腦跑不起來（寫死 `~/UWGOG/UW1`）；設定畫面改用截圖驗證
  （`p12-shot.py NAME 4000 --keep-settings '1000 key tab' …`，按 5 次 tab 到「語言與文字」）。

## 6. 下一步

### 6.1 做一個強化功能的標準步驟（照這樣做，前五項都是這樣完成的）

1. **找原版的程式**：用 Grep 在 `uw1/src` 找相關函式。檔頭註解寫得很完整，先讀註解。
2. **掛鉤只加 ASCII 呼叫**：在遊戲程式（`src/` 的 `.C`）裡用 `#ifndef __TURBOC__ … #endif` 包住，呼叫 port 端的函式；
   實作放 `src/port/`（例如 `src/port/ui/autokey.c`、`src/port/presentation/flask_text.c`）。新檔會自動被編譯。
3. **開關**：`src/port/portgame.h` 的 `ENH_` 列舉最後加一個（`ENH_COUNT` 之前）；`src/port/sys/enhtab.c` 照同樣順序加一列
   （名稱、英文說明、`ENH_GAMEPLAY` 或 `ENH_PRESENTATION`）；程式裡用 `ENHANCED(ENH_xxx)` 判斷。
4. **中文**：`src/port/i18n/settings_words.c` 加開關名稱與說明的中文（英文字串要和 enhtab.c **一字不差**）；
   遊戲訊息放 `src/port/i18n/phrases.c`（`port_zh() ? port_zh_lit("key") : "English"`）。
5. **文件**：`docs/ENHANCEMENTS.md` 的表格加一列（在 repo 根目錄的 docs，不是 uw1/docs）。
6. **驗證**：編譯 → 用 `p12-shot.py` 開關各截一張對照 → 第 1 節的檢查全部跑 → commit、push → 看 CI。
7. **收尾**：把執行檔複製到 `UW1-zhTW-playtest\`；在 `IDEAS_BACKLOG.md` 該項標題後註明「已完成、commit、開關名」；更新本文件。
8. **回報使用者**：做了什麼、F11 哪個開關要打開、截圖確認了什麼、哪些沒驗證到。

### 6.2 強化功能後續

**A. 第 1 項：戰鬥傷害數字與敵人血條（已完成，使用者已確認 OK）**

- 主 repo `06be938`（Linux 連結修正 `732949d`）、exhume `438441b`，均已 push；執行檔已更新到 `UW1-zhTW-playtest`。
- F11「強化功能」最下方：「戰鬥傷害數字」`damage-numbers`、「敵人血條」`enemy-health`，各自開關、預設關。
- 實作與驗證報告：`uw1/docs/i18n/P13_COMBAT.md`。傷害、揮空、格擋來自原版事件；英文模式也顯示 Block／Miss。
- 2026-10-26 追加（`820a1be`，待使用者實測）：敵人對玩家真正啟動近戰、射擊、攻擊性施法時也顯示血條，不必命中或先傷到它。沿用 `enemy-health`；防禦施法、未啟動攻擊、其他 NPC 目標不觸發。
- 新版執行檔已更新到 `UW1-zhTW-playtest`；只複製 EXE，25 個原有存檔檔案雜湊不變。
- 新觸發只更新獨立呈現狀態，不修改敵人 HP、AI、亂數，也不製造傷害飄字。637 項呈現與 55 項真實 AI 函式檢查通過 ASan／UBSan；17 個單元測試、13 項文字回歸及 8 種 SDL 合成、16 個案例的中英文／顯示開關四模式存檔比對通過。
- 實際遊戲的敵人 HP 維持 43，從攻擊前無血條到攻擊後有血條；中文、英文、關閉對照與 UBSan 均驗證，開著無敵也適用。工具 `.poc-tools/attack-bars-check.py`、`attack-bars-replay.py`；報告與截圖 `build/i18n/attack-bars/`。
- 位置使用原版繪圖的 framebuffer 像素歸屬；完全被遮住的生物不顯示。血條上限已依 2026-10-26 回報改為首次觸發血條時的個體 HP，後續觀察到更高 HP 才上調，不使用種類平均值。
- 動畫只讀 `*Time`，**不可新增 `GAME_TIME()`**（會消耗重播事件）。換樓層、讀檔、新遊戲、物件回收時清除暫存。
- 原版首批 624 項測試含 ASan／UBSan、既有中文化測試、文字回歸、8 種 SDL 視窗合成都通過；16 個案例存檔比對一致。
- 截圖：`build/i18n/p12/shots/combat-final.png`、`combat-en-block.png`、`combat-off.png`、`combat-health-only.png`、`combat-miss.png`、`combat-settings.png`。
- 本機 DOS 重編譯 Gate 缺 Node.js，尚未驗證；原生建置、ASCII、asm2c 產物一致性與 repocheck 已通過。
- 使用者已於 2026-10-25 確認兩項功能 OK；密技選單首版已依 2026-10-26 的授權完成（見 B），其餘候選功能未自動動工。

**B. 密技選單首版：全地圖、無敵與安全位置救援（2026-10-26 已實作，待使用者實測）**

- F11「密技」分頁：全地圖 `full-map`、無敵 `god-mode` 預設關；另有「記住安全位置」「返回安全位置」。中英文都可用，關閉設定回到可操作的 3D 畫面後生效。
- 全地圖只讀地圖資料、使用獨立繪圖陣列；探索紀錄與遊戲亂數不變，已以正常存檔逐位元組比對。
- 讀檔修正（2026-10-26）：無敵預先開啟時，原版讀檔合法的 `GrSq=-1` 曾被當成越界，提早救援並向未初始化捲軸印字而閃退。現在等待原版放置玩家；未進入遊戲不救援，未初始化捲軸不印字。
  新增啟動階段單元測試；四個使用者存檔的獨立副本均通過「先開無敵再載入」及 Ctrl+R 重讀，位置／HP 保持一致；另有英文與關閉對照。
  工具 `.poc-tools/god-load-check.py`、報告 `build/i18n/god-load/report.json`。使用者已於 2026-10-26 複測確認問題解除。
- 無敵先攔截扣血，另處理虛空、反噬與直接死亡；越界和傳送失敗恢復到經驗證的安全地面，不清除 NPC 來強迫放置。
- 安全點每層各一個、只保留於本次遊玩，讀檔與新遊戲清除；救援時重新檢查占用。找不到合法位置時停止異常物理並提示讀檔，保留選單操作。
- 原版 `finish_player` 的阻塞等待在無敵時有步進上限；正常飛行、浮空、墜落沒有時間到就救援的計時器。
- 虛空測試另外修正 `AI.C/set_up_target` 空目標解參考；原生分支把它視為無有效目標，DOS 分支不變。
- 公用密技只有六個 `menu-*` 指令，透過遊戲執行緒安全點執行並記入錄影；4／5 升級 6 且保留原始串流。含私人測試指令的重播仍受環境變數保護。
- 主 repo `822b5bf`、exhume `5e92215`，已 push；測試版執行檔已更新。實作與驗證報告 `uw1/docs/i18n/P13_CHEATS.md`，原始需求保留在 `P13_CHEATS_PLAN.md`。
- 新測試 `tools/i18n/test_cheat_menu.py --sanitize` 已加入三個原生平台 CI；本機遊戲情境與截圖在 `build/i18n/cheats/`。
- 首版完成全地圖、無敵及必要救援；2026-10-26 使用者要求下一項後新增「提高亮度」（見 C）。補血／法力、傳送、物品、屬性技能、時間停止尚未加入。

**C. 提高亮度（2026-10-26 已新增數值滑桿，待使用者複測）**

- 主 repo `d11a79b`，測試版執行檔已更新；原有 25 個存檔檔案雜湊保持不變。
- F11「密技」→「提高亮度」`bright-view`，預設關；另有「增亮強度」滑桿 0～15（預設 4），可拖曳或用左右鍵；關閉設定回到 3D 遊戲後生效。
- 數值滑桿提交 `b353ef0`，測試版 EXE 已更新；25 個使用者存檔檔案雜湊不變。
- 0 是原始明暗、4 維持上一版溫和效果、15 最亮，保存於 `bright-strength`，關閉開關仍保留數字。
- 連續拖曳合併為最後數值，由主迴圈安全點的 `menu-bright-level` 套用並錄影；非預設啟動值也記錄，舊錄影忽略本機數值並從 4 開始。
- 使用者回報首版全亮缺乏過渡；現改成在建立與繪製一般 3D 畫面時，將當前光源 `smooth_div` 乘 `(16-強度)/16`（至少 1）、`smooth_base` 減 `ceil(強度/4)`（至少 0）；預設 4 等同先前效果，沿用原版 `preset_grid`。色表不改寫，保留不同光源與 16 階明暗。
  不改光源、火把消耗、視野或探索範圍，不新增存檔欄位。
- 核心 237269 項 ASan／UBSan 檢查、17 個既有單元測試程式、13 項文字回歸與 8 種 SDL 視窗合成、
  16 個 session 的中英文／DOS golden 存檔比對通過。
- 檢查八組原版光源 × 16 種強度、可見遮罩、地圖原始 shade、快取失效與網格還原；保留剛建立的牆面高度位元給拾取。
- `GRIDDB.C` 兩處 shade < 8 的地圖細節判定使用原始 shade，避免增亮後提前記錄地形／門。實際數值 7／0 保存檔案一致；F11／數值保存／正常與 UBSan 重播通過。
- 本輪照明與血條一起啟用的 16 組四模式，所有有效存檔資料一致。`items` 的原版地圖註記 NUL 後會殘留畫面色碼；
  測試只對這段未使用文字填充歸零作額外比對，保留原始雜湊，關閉模式仍與 DOS golden 原始位元組一致。
  不可宣稱本輪所有原始存檔位元組完全相同。詳見 `P13_CHEATS.md` 的明確例外。
- 無敵與提高亮度一起預先開啟：四個使用者存檔的副本均通過 UBSan 啟動載入及 Ctrl+R 重新讀檔，位置／HP 保持一致；英文與無敵關閉對照也通過。工具 `.poc-tools/bright-god-check.py`，報告 `build/i18n/god-load/report.json`。
- 最新工具 `.poc-tools/light-hp-check.py`（含 `--static`、`--hp`、`--record`、`--debug`）、`light-hp-replay.py`；報告／截圖 `build/i18n/light-hp-fix/`。
- 本次修正提交 `3a6bc51`；測試版 EXE 已更新，25 個使用者存檔檔案雜湊不變。使用者已於 2026-10-26 確認血條最大值問題解除；亮度滑桿待複測。
- 血條另以種類平均 45、個體 HP 3 的存檔副本驗證：攻擊前不顯示，攻擊後 3/3 綠，實際兩次各扣 1 後 1/3 黃；正常與 UBSan 均測，不改使用者存檔。
- 最新數值工具 `.poc-tools/brightness-slider-check.py`（含 `--images`、`--debug`）、`brightness-slider-save.py`、`brightness-slider-replay.py`；報告／截圖 `build/i18n/brightness-slider/`。
- 原工具 `.poc-tools/bright-check.py` 與 `build/i18n/brightness/` 是首版全亮的歷史報告。

### 6.3 P13.7 第 5 項：地圖中文註記（已完成，使用者已確認 OK）

- 主 repo `cc4e3f0`，已 push，測試版執行檔已更新。2026-10-25 使用者授權繼續原計畫，先完成剛討論的地圖中文輸入；報告 `uw1/docs/i18n/P13_MAP_NOTES.md`。
- 沿用 P12 的中文輸入面板，在中文 HD 模式使用；編寫時 Z 照常輸入，Enter 完成／Esc 取消後才可關閉地圖。
- UTF-8 不進原版 ATM 結構：只放四字元 ASCII 編號；`SAVE0\NOTES.UTF` 依編號、樓層、座標對應。
- 旁註檔隨正常存讀檔複製；英文模式也顯示已有中文；缺旁註檔時顯示 ASCII 編號，不猜測譯文。
- 原版沒有就地編輯，修改要刪除再重寫。刪除最後一則中文時寫一筆原版可理解的已刪除記錄，避免原版空區塊造成註記復活。
- 新增單元測試、既有全套、文字 sanitizer、SDL 視窗合成、16 個重播與實際遊戲生命週期均通過。自動測試的中文提交由測試介面模擬；使用者已於 2026-10-25 實測確認地圖中文註記功能 OK。
- 本機生命週期工具 `.poc-tools/map-cn-check.py`；產物 `build/i18n/map-cn/report.json`、各案例 PNG／log，測試只用可拋棄的新資料夾。
- 羽毛筆比一般游標寬，英文模式也發布真實游標範圍供文字遮擋判斷；不要用固定 25×25 方框估計。

### 6.4 其他待辦（等使用者）

1. **P13.2、P13.3：使用者實際遊玩與審閱譯文。** 戰鬥傷害數字、敵人血條與地圖中文註記已確認 OK；無敵預開啟的讀檔閃退已確認解除；Z 開關地圖、密技其他情境、新加提高亮度及攻擊觸發血條仍待使用者確認。 測試清單在 `P13_QA.md` 最後一節。使用者回報問題時，先用截圖工具重現再修。
2. **待使用者決定：**
   - P8 HD 錄影何時重錄（§P13.6）；
   - §P13.7 的改進項目：打字問話題能否改成選擇、關鍵字打錯時 NPC 沒反應、數量滑桿改在放下時出現、
     **密技選單的其他候選功能**（首版已實作，見 6.2 B）；地圖中文註記已完成並獲使用者確認（見 6.3）；
   - **HD 化的時程：** 目前只有文字與游標 HD 化。還沒做 M2（介面圖案重畫）、M3（HD 3D 繪製）、M4（HD 貼圖與寬螢幕）、
     16:9（A 置中加兩側裝飾／B 上下裁切／C 真寬視野）。稽核在 `docs/modernization/UW1_UW2_SHARED_HD_MODERNIZATION_AUDIT.md`。
   - **UW2 中文化（到 P14 再決定）：** 同一個 repo，先把 `uw1/src/port/` 共用的部分搬到共用位置，UW2 只補自己的對應與掛鉤；
     發行包建議先維持 UW1、UW2 分兩包。
3. **發行前要補的事：** 譯文全部還是 `draft`，要等人工審閱；遊戲標誌圖「Ultima Underworld」維持英文（需要美術素材）；
   Mac 版的 `開始遊戲.command` 還帶 `--locale zh-TW`，會讓 F11 的語言切換失效（見 `打包測試版流程.md` 第五節）。
4. **P13 Gate 與 tag `zh-tw-native-p13`：** 上面完成後，依主計畫書 §Acceptance Gate P13 確認，經使用者同意再建。
