# 繁中整合測試版 0.6（2026-10-31）

使用者決定：UW1 中文化告一段落，重新打包 Windows 與 Mac 版（版本 0.6），並建立階段 tag `zh-tw-native-p13`；之後進入 UW2 中文化。
本次為私人整合測試包；譯文 6,900 列中 485 列 reviewed，其餘仍為 draft，往後依使用者審閱紀錄繼續提升。

## 0.6 相對 0.5 的變更

- 現代按鍵（2026-10-27，已確認）：空白鍵改為跳躍；X 只關容器；C、Z、R、F 等按一次只動作一次。
- 強化功能（皆預設關，已確認）：拆分數量預設全部 `split-all`、E 撿起自動堆疊（`e-takes`）、對話打錯字回應 `unknown-topic`、地圖橡皮擦輔助 `easy-erase`。
- 強化功能即時生效（已確認）：9 項在 F11 切換後回到 3D 畫面就生效，切換記入錄影。
- 按鍵說明頁（已確認）：F11「按鍵說明」分頁、Shift＋F1。
- 標題頁游標延遲修正（已確認）：HD 描邊快取，中文標題頁回到顯示器幀率。
- 翻譯資料：蜥蜴人語加註、權威資料修正（`dd27dd5`）；哥布林等譯文修正（0.5 之後）。
- 說明檔改寫：按鍵與 0.6 新增內容（`39eda58`）。

## Windows x64

- `release-v0.6/UW1-zhTW-v0.6-Windows.zip`，7,549,990 bytes，SHA-256 `a430bac4e0590d5cb51256571f60f02f4cdb030b94c147260cd3d9ac7a24619b`；32 檔。
- 由 `tools/i18n/package_playtest.py --version 0.6` 產生。MANIFEST 的來源提交為 `33011eb`（EXE 與全部程式碼皆出自此提交，含 HD 描邊修正）；
  其中 `說明.txt` 取自當時尚未提交、隨後以 `39eda58` 提交的新範本，內容相同。Exhume `370b998`。
- catalog：由目前主表重建兩次，雜湊相同，SHA-256 `7a3d2c23763087f955934ae8ffb0d6b0a026126fdd4b0ddd4e2d25df1eceb905`（6,900 entries），與測試版使用中的相同。
- 驗證（本機工具 `.poc-tools/v06-verify.py`，報告 `uw1/build/i18n/v0.6/package-verification.json`）：
  MANIFEST 31 檔逐檔雜湊、ZIP CRC 與內容逐位元組比對、無 UW.EXE／DATA／home／SAVE、說明與設定檔無開發機路徑或固定 locale。
  解壓副本以僅 Windows 系統目錄的 PATH 經 `開始遊戲.bat` 啟動：自動選中文、標題頁與讀檔進遊戲正常（截圖於同目錄）。
  註：本代理的 shell 設有 `NoDefaultCurrentDirectoryInExePath`，會讓 bat 找不到同目錄的 EXE；驗證時比照一般玩家環境移除此變數。
- 同一程式碼已通過：17 個單元測試、16 個 session × 四模式存檔與 DOS golden（`uw1/build/i18n/hd-border/`）、validate_authority、proper-names 檢查；CI `33011eb` 的 uw1-port 與 repocheck 通過。

## macOS 通用版

- [uw1-release run 41094295701](https://github.com/pmanyeh/underworld-exhumed/actions/runs/41094295701)，來源 `39eda58`，artifact `macos`（ID 12995471874）。
  macos 建置 job 成功（lipo、codesign 驗證、相依路徑、--help）；ad-hoc 簽章，未經 Apple 公證。
- 後段 test-* 工作在 `uw1-assets`（取得私有遊戲資料）步驟失敗，與 0.5 相同，未進入遊戲重播；不宣稱 Actions 遊戲測試通過。
- `release-v0.6/UW1-zhTW-v0.6-macOS.zip`，9,300,345 bytes，SHA-256 `c1d62169cc294f1d1a1862b57ea2e70311a12635c189f5ced8b8d548b61ddb42`。
  本機工具 `.poc-tools/mac06-package.py`（由 `evidence/mac-v0.5/package.py.txt` 改寫）：原 app 8 檔逐位元組不變、主程式與 3 個 dylib 皆含 arm64／x86_64、
  權限保留、`.command` 為 LF／0755 且 `sh -n` 通過、ZIP CRC 通過；同一 catalog、字型與文字設定。報告 `uw1/build/i18n/mac-v0.6/package-verification.json`。
- Mac 上 0.6 尚未實機遊玩；0.5 Mac 版已於 2026-10-27 實機確認。

## 交付位置與限制

- 兩個 ZIP 與 `UW1-v0.6-SHA256SUMS.txt` 在工作區 `release-v0.6/`，只留本機；catalog 含英文原文，不上傳公開 GitHub。
- 包內不含遊戲本體、玩家 home 或存檔；玩家自備 GOG 資料。原 `UW1-zhTW-playtest` 未被打包工具改動。
- 工作區根目錄有一個打包時產生後清空的 `UW1-zhTW-v0.6-Windows` 空資料夾（代理無權刪除工作目錄），可手動刪除。
- 本次沒有重做開發移機包；DOS 重編譯 Gate 仍未在本機執行。
