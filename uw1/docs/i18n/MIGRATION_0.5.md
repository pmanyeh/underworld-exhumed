# 0.5 開發移機說明（私人資料）

`UW1-dev-handoff-v0.5.zip` 只供專案擁有者自己搬移：含自己的遊戲資料、原文、私人權威資料及玩家存檔。
不要上傳 GitHub 或公開分享。程式的乾淨攜帶包另外是 `UW1-zhTW-v0.5-Windows.zip`。

這是續接開發所需的資料包，並非整個舊工作區的完整備份：不含 MSYS2、舊 Python venv、約 48 GB 舊測試輸出、
舊 Mac 包和 Borland 工具鏈。原電腦資料不會被刪除；新電腦驗證前先保留原機。
包中 `workspace` 有必要的私人遊戲／翻譯／測試存檔、本機腳本、字型、原生函式庫與玩家 home 備份。

## 1. 校驗並解壓縮

將兩個 ZIP 與 `UW1-v0.5-SHA256SUMS.txt` 複製到新電腦，用 PowerShell `Get-FileHash -Algorithm SHA256` 比對。
把移機 ZIP 解壓到暫存資料夾（以下稱 `移機包`）。`MANIFEST.json` 列出每個內容檔的 SHA-256 和來源提交。
兩個 Git bundles 在 `bundles/`，可離線還原程式與歷史；程式碼仍以 GitHub 分支為主要開發來源。

## 2. 先還原 Git，再覆蓋私人資料

建議工作區仍使用 `D:\git\Ultima Underworld`。先安裝 Git for Windows。
工作區與 repo 目錄應是新目錄；已有 repo／存檔時先備份、檢查，不直接覆蓋。

有網路時可依 HANDOFF 的線上 clone 指令。離線方式如下（命令提示字元，路徑按實際解壓位置更換）：

```bat
git clone -b feature/zh-tw-native "移機包\bundles\underworld-exhumed.bundle" "D:\git\Ultima Underworld\underworld-exhumed"
git -C "D:\git\Ultima Underworld\underworld-exhumed" -c protocol.file.allow=always -c submodule.exhume.url="移機包\bundles\exhume.bundle" submodule update --init
git -C "D:\git\Ultima Underworld\underworld-exhumed" remote set-url origin https://github.com/pmanyeh/underworld-exhumed.git
git -C "D:\git\Ultima Underworld\underworld-exhumed\exhume" remote set-url origin https://github.com/pmanyeh/Exhume.git
git -C "D:\git\Ultima Underworld\underworld-exhumed\exhume" checkout feature/presentation-overlay
```

bundle 路徑請使用完整絕對路徑，避免 submodule 在不同工作目錄尋找。
兩個 repo 都設定 `git config core.autocrlf true`；用 `git log -1`／`git submodule status` 對照移機包 manifest。
之後將 `移機包\workspace\` 裡的內容複製到工作區，保留其相對目錄結構。
**不要先覆蓋私人資料再 clone 到非空 repo 目錄。**

## 3. 重建工具環境

舊 `.poc-tools/python` 依賴舊電腦的 Python 安裝路徑，不納入本包。安裝本次驗證使用的 Python 3.12，然後在工作區執行：

```bat
py -3.12 -m venv .poc-tools\python
.poc-tools\python\Scripts\python.exe -m pip install -r .poc-tools\handoff\requirements.txt
```

安裝 MSYS2、使用 CLANG64 環境。依 repo 的 `uw1/docs/BUILDING.md` 執行 `make setup-port`；
`.poc-tools/msys64` 需指向實際的 MSYS2 安裝目錄，可直接安裝到該處，或建立 junction。
本包已有字型、Nuked OPL3 和原生函式庫；setup-port 可按需要重建它們。

建立兩個歷史腳本使用的 junction（已存在就先檢查，不刪舊目的地）：

```bat
mklink /J D:\git\uw1-native-poc "D:\git\Ultima Underworld"
mklink /J D:\git\uw1-poc-libs "D:\git\Ultima Underworld\underworld-exhumed\uw1\tools\libs"
```

如果無法使用 D 槽，必須相應調整 `.poc-tools/handoff/env.sh` 及使用固定 D 槽路徑的歷史腳本。
首次接手不要假設它們已全部支援任意路徑。

## 4. 驗證與玩家存檔

依 HANDOFF 第 4 節，先 source env、編譯、跑 17 個單元程式和 16 個四模式存檔比對，再繼續開發。
本包未帶走所有歷史截圖／STATE.OUT；可由腳本重建。真實輸入法與 Mac 遊玩仍要在該平台實測。

玩家目前完整 home 備份在 `workspace/UW1-zhTW-playtest/home`。
可把乾淨 0.5 Windows 包解壓成一個新資料夾，再複製此 home；不要覆蓋新電腦已存在的較新存檔。
所有 SAVE0、SAVE1～SAVE4 等資料夾及 UTF 旁註檔都要保留。若路徑改變，首次啟動需重新選擇自己的 GOG 資料。

讀取原存檔、確認中文註記／名字、F11 設定和技能滾輪後，再回報新電腦可正常開發。
開發前請再讀 HANDOFF：空白鍵已決定保留攻擊，P13 尚待人工驗收，不要自動開新功能或改譯文審閱狀態。
