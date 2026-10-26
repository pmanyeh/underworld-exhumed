# 打包 UW1 繁中整合測試版（目前 0.5）

Windows 整合版是程式、相依 DLL、中文字型與 catalog 的私人攜帶包，不含原版遊戲與玩家 home。
版本號是攜帶包版本，不代表 P13 人工驗收完成。Mac 舊包不自動升為 0.5。

## 一、準備與驗證

Git 使用 PowerShell 的 Git for Windows；先確認主 repo 與 Exhume 沒有待處理的使用者改動。
Git Bash：

```sh
source /d/git/uw1-native-poc/.poc-tools/handoff/env.sh
"$PY" ../exhume/tools/portbuild.py --config exhume.toml
"$PY" /d/git/uw1-native-poc/.poc-tools/v05-check.py units
"$PY" tools/i18n/validate_authority.py
"$PY" tools/i18n/proper_names.py --check
"$PY" /d/git/uw1-native-poc/.poc-tools/v05-check.py replays
```

17 個單元測試程式及 16 個四模式 session 要通過。NUL 後地圖文字填充的唯一例外見 `P13_CHEATS.md`；
英文關閉模式仍必須和 DOS golden 原始位元組相同。不要用舊二模式比較把已知垃圾填充誤判成新問題。
更動文字繪製時另跑 `.poc-tools/p10a-b1-text-regression.py`；本機 DOS 重編譯 Gate 缺 Node.js，不宣稱已驗證。

## 二、catalog

目前使用 `build/i18n/p13-goblin/catalog-1.bin`；本輪未修改譯文，`catalog-1.bin` 與 `catalog-2.bin` 應相同。
catalog 含英文原文，**不能 commit 或公開上傳**。
改譯文後，先用 `tools/i18n/l10n.py validate`、`authority`、`build` 重建到新批次，連續兩次 build 雜湊相同，
再跑 proper-names 和 16 個 session。原始 UW1 PAK／CNV 路徑由 env.sh 的 `$D` 陣列提供。

## 三、Windows 新包

使用已進 Git 的安全工具，輸出資料夾和 ZIP **都必須不存在**，不會自動刪舊包：

```sh
"$PY" tools/i18n/package_playtest.py \
  --version 0.5 \
  --output 'D:\git\Ultima Underworld\UW1-zhTW-v0.5-Windows' \
  --catalog build/i18n/p13-goblin/catalog-1.bin \
  --toolchain /d/git/uw1-native-poc/.poc-tools/msys64/clang64
```

內容：EXE、遞迴解析後的非系統 DLL、粉圓字型、catalog、中文設定／輸入別名／專有名詞表、完整相依授權、
`開始遊戲.bat`、最新版 `說明.txt`、`VERSION.txt`、`MANIFEST.json`，以及同名 ZIP。
MANIFEST 記錄來源提交、子模組、catalog 和每個內容檔的 SHA-256（不包含 manifest 自身）。

啟動器只使用相對的 `UW`／`home`；沒有 UW 時由遊戲尋找 GOG 或開選擇資料夾視窗。
**不要寫死開發機路徑，不要加 `--locale`**，避免 F11 語言選項失效。
新包不預先放存檔、設定或開啟密技；玩家要自備自己的 GOG 資料。

## 四、實際驗證與交付

1. 用新包副本或獨立 `--home` 啟動；把 PATH 縮到 Windows 系統目錄，確認程式不依賴開發機 DLL。
2. 檢查中文設定／讀檔、英文模式、屬性頁數值、地圖與滾輪。測試 fixture 只放臨時 home，不能放進最終 ZIP。
3. 逐檔驗證 manifest、檢查 ZIP CRC；確認沒有 UW.EXE、遊戲 DATA、home、絕對開發路徑。
4. 確認原 `UW1-zhTW-playtest/home` 所有檔案的雜湊不變。不要用舊 `make_play.py` 重建使用者資料夾。
5. 更新 `RELEASE_0.5.md`、HANDOFF 和兩份根目錄副本，commit／push，確認 CI。
6. ZIP 及 SHA256SUMS 留本機；Git 只推安全程式、說明與摘要，不推原版資源或 catalog。

## 五、開發移機包

`UW1-dev-handoff-v0.5.zip` 另外保存兩個 repo 的 Git bundles，以及 HANDOFF 第 3 節列出的必要私人資料。
先 clone、再還原 workspace 覆蓋資料，依包內 `移機說明.md` 重建 Python／MSYS2，驗證後才繼續開發。
MSYS2、舊 Python venv 和約 48 GB 舊測試輸出不納入；不刪除原電腦的任何檔案。
移機包含自己的遊戲資料、原文與存檔，**只供自己搬移，不公開上傳**。

## 六、Mac 舊流程（本次未更新）

GitHub `uw1-release.yml` 可產生 macos artifact，再於本機合併 `UW1-zhTW-mac-kit` 的字型和中文檔。
catalog 不能上傳 GitHub。舊 `開始遊戲.command` 還有固定 locale 的限制，改動需在 Mac 實機驗證。
不可把舊 Mac 包直接標成 0.5 或宣稱已實機驗證。
