# Mac 繁中整合測試版 0.5（2026-10-26）

依使用者要求透過 GitHub Actions 編譯目前分支，並在本機合併中文配件。

- 工作流程：[uw1-release run 40308423602](https://github.com/pmanyeh/underworld-exhumed/actions/runs/40308423602)。
- 分支 `feature/zh-tw-native`；來源 `4c662c17753f1a794057ba947420b5f10df9deb9`，Exhume `5e92215`。
- macos job `121523233438` 成功；artifact `macos`，ID `12643726437`。
- 原始下載 `mac-artifact/run-40308423602/UW1-feature-zh-tw-native-macos.zip`。
- 原始 app 的主程式及三個 dylib 均含 arm64 和 x86_64；FreeType 靜態連結。
  CI 的 lipo、codesign --verify --deep --strict、otool 相依路徑、圖示和 --help 檢查通過。
- ad-hoc 簽章；未經 Apple 公證。

## 私人中文整合包

`UW1-zhTW-v0.5-macOS.zip`，9284795 bytes。
SHA-256：`31ed75d08f7746ab646d066c6a36c53f35302d5ba7400ad7d25ac9081fd5b363`。
獨立校驗檔 `UW1-v0.5-macOS-SHA256SUMS.txt`；未更動先前 Windows／私人移機 ZIP 及校驗檔。

- app 和原授權原樣保留，合併最新 p13-goblin catalog、粉圓字型與 OFL、文字設定／輸入別名／專有名詞。
- 用 ZipInfo 保留 Unix 權限與可能的符號連結，原 app 的 8 個檔案逐位元組雜湊不變。
- .command 使用 LF 和 0755 權限，先 cd 到包目錄，再指定 home 啟動，不固定 locale。
  中文預設選取依現有 main.c 的相對 catalog 判斷；實機測試回報見下方驗證狀態。
- ZIP CRC、manifest 全檔雜湊、主程式／dylib 雙架構及執行權限通過；shell 語法檢查通過。
- 不含遊戲本體與 home，沒有改動原玩家存檔。中文 ZIP 僅留本機，沒有上傳公開 GitHub。

## 驗證狀態（2026-10-27 更新）

使用者已於 2026-10-27 回報 Mac 繁中整合測試版 0.5 實機測試 OK。

Actions 的 test-macos 後續工作在抓取私有測試資料時回報 SSH key invalid format／Permission denied，
尚未進入遊戲重播；整個 release workflow 不能視為全部通過。
上述 Actions 私有資料重播尚未完成的狀態仍保留；實機確認依使用者回報記錄。
打包當時本機為 Windows，檢查範圍為 ZIP 結構、檔案／簽章內容未變、架構、權限與啟動腳本語法。

本機工具 `.poc-tools/mac05-package.py`；報告 `uw1/build/i18n/mac-v0.5/package-verification.json`。
實際合併腳本另留存 `evidence/mac-v0.5/package.py.txt`；新電腦可取回，重跑前應調整 run／來源路徑與新輸出名稱。
這次是額外的 Mac 包，前次 0.5 移機 ZIP 仍是當時的 Windows 交付快照。
