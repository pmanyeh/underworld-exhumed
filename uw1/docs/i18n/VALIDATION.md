# Native PoC 驗收規則

2026-10-21，依使用者最新決定：本 PoC **只執行 Native 驗收，不要求 DOS 驗證**。
此決定取代工作區 v2.2 計劃與先前報告中「DOS optional / deferred」的驗收要求。

- 不需要安裝、尋找或執行 Borland Turbo C++ / TASM。
- `make check`、DOS exact-match、DOS replay 執行與重新生成 DOS goldens 不屬於 PoC gate。
- 後續報告將 DOS 驗證記為 **NOT APPLICABLE — Native-only PoC**，不再記為 blocker、驗證缺口或 deferred 工作。
- 驗收依 Native build、native launch、Native/public regression 與各 Phase 專項測試；
  跨平台、UTF-8、中文排版、FreeType、overlay 與遊戲行為仍依原計劃逐階段驗證。
- 可沿用 repository 已提交的 golden snapshots，比對 Native 結果；這不需要執行 DOS 或 Borland。
- PoC 的 `verify_localized_replay.py` 跳過 upstream verifier 自動嘗試的 DOS reference rebuild，
  保留原本 Native checkpoint、screen、save 與 translation-hit assertions。
- 保留上游既有 DOS 相容程式碼與工具；不以移除它們作為本 PoC 工作。

Phase 0–2 的歷史 logs、manifest 與當時的 NOT RUN 結果保留原樣，並非目前尚待解決的阻礙。
標準驗收不再要求 `verify_dos_branch.py`；它只保留作為既有的結構檢查工具。

本次更新已實測：沒有嘗試 DOS reference rebuild，16 組 Native replay 全數通過，
12 個翻譯 key 的命中內容與 TSV 相同。此更新不開始 Phase 3。

## P10 起：P8 HD 錄影與 catalog

HD 模式會把排版後的 scroll 游標寫回遊戲記憶體，所以 P8 的 HD 錄影（walk、dialogue、restore、resize）
只在錄製時的 catalog 下逐 byte 相同（[P10A F1](P10A_BATCH1_REPORT.md#i-findings--blockers)）。
依 2026-10-23 的決定，在 UW1 完成（P13）重錄之前：

- P8 HD 錄影一律用 **P8 catalog** 重播：由 `zh-tw-native-authority-ready` tag 的 master 重建，
  SHA `fd133b9a…dff98`（`test_l10n.py` 的 P8 catalog 測試會驗證可重現）。它們是 HD 程式路徑的回歸。
- 新譯文的 runtime 驗證用 `verify_localized_replay.py`（每個 hit 符合 master）與 HD scroll log 的 QA 重播。

## 本機環境注意事項

- 工作區路徑含空白時，pkg-config 的 `-I` 會被斷開，Native build 找不到 FreeType／mt32emu header。
  請經由不含空白的 junction（例如 `D:\git\uw1-native-poc`）建置與執行回歸。
- MSYS2 的 `usr/bin/git` 與 Git for Windows 的 `core.autocrlf` 不同；前者會把整個工作樹顯示為已修改。
  檢查狀態、提交請用 Git for Windows。
- Windows 上以文字模式寫出的 evidence（JSON、log）是 CRLF；提交前轉成 LF，`git diff --check` 才會通過。
