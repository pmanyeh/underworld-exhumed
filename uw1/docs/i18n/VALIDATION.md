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
