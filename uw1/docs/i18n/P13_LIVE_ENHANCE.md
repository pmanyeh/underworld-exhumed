# 強化功能即時生效（2026-10-30）

## 起因

使用者問：F11 很多設定要重新啟動才有用，為什麼？說明原因（錄影重播要和遊玩過程一致、規則類功能不能在動作做到一半時改變、有些功能在啟動時就裝好）後，
使用者指出一般玩家不太會用錄影重播，問能不能不用重啟。我提出的方向是：「只在動作當下讀開關」和「每個畫面重畫」這兩類改成即時生效，
並照密技分頁的做法把切換記進錄影；啟動時就裝好的功能仍維持重啟。使用者同意照這個方向做。

## 行為

- F11「強化功能」以下 9 項改為即時生效，不再標示「重新啟動後生效」：
  `invert-look`、`flask-numbers`、`auto-key`、`e-takes`、`damage-numbers`、`enemy-health`、`split-all`、`unknown-topic`、`easy-erase`。
- 切換後立刻寫入設定檔（和以前相同）。關閉設定畫面、回到 3D 畫面且游標上沒拿東西時套用，訊息欄顯示「設定已生效。」（英文 "Setting applied."）。
- 其餘維持重新啟動後生效：啟動時就裝好的（`modern-keys`、`mouse-look`、`rune-keys`、`perspective`、`wide-pitch`、`full-sprites`、`mouse-wheel` 等），
  以及只在開場或讀檔時起作用的（`skip-intro`、`restore-at-rest`）。`mouse-wheel` 在平台輸入那一層讀開關，本次不動。
- 密技分頁的 4 項原本就即時生效，不變。

## 實作

- 沿用密技分頁的機制（`src/port/ui/cheat_menu.c`）：設定畫面的開關只把指令放進有界佇列，遊戲執行緒在主迴圈安全點執行，並以安全點序號記進錄影（format 6，`menu-enhance NAME 0|1`）。
  錄影一開始沒有任何強化功能或密技時，第一次切換會照既有機制把錄影從 format 4／5 升級為 6。重播時在同一安全點套用，不需要測試環境變數。
- 新的公用指令只接受上面 9 個名稱與 0／1，其他名稱（例如 `god-mode`、`modern-keys`、`mouse-wheel`）一律拒絕。
- 逐項確認這 9 項只在動作或繪圖當下讀 `ENHANCED()`，啟動時沒有依開關另外準備狀態。唯一要補的是血瓶數值：
  關掉時已發布的數字會留在畫面上，所以新增 `flask_text_clear()`，在關閉的那一刻清除。傷害數字與血條關閉時原本就會隱藏，標記緩衝每畫一次都會清空。
- 按鍵說明頁每次顯示時才依 `ENHANCED()` 列出，`e-takes` 和 `easy-erase` 的說明行會跟著切換。
- 沒有修改 Exhume 子模組；原版 `.C/.H` 沒有改動（只動 `src/port/`）。

## 驗證

- Windows 正常建置通過（FreeType、Nuked OPL3、libmt32emu）。
- `tools/i18n/test_cheat_menu.py`，正常版與 `--sanitize`（ASan＋UBSan）：237,372 項檢查通過。新增的檢查包括：
  - 9 個設定列都接到強化功能分頁，各自只切換自己那一位元，每次切換都寫入錄影；
  - 不在安全時間點時等待，回到安全時間點才套用；
  - 關掉血瓶數值時會清除畫面；
  - 不在清單內的名稱、不是 0／1 的值、參數數量錯誤都會被拒絕，且只檢查時不改變狀態；
  - 含 `menu-enhance split-all 1` 的 format 6 錄影，在沒有測試環境變數時可以重播並套用。
- 17 個單元測試程式全部通過。
- 實機情境（本機工具 `.poc-tools/live-enhance-check.py live`，原有測試存檔的獨立副本）：啟動時沒開 `split-all`，遊戲中才用公用指令打開，
  把背包 5 枚金幣拖進袋子後按 Enter，整疊 5 枚都移進袋子；打開又關掉時，Enter 只移 1 枚、剩 4 枚。中英文各跑一次，
  錄影重播後 checkpoint 的事件、時間、輸入呼叫次數與存檔完全一致，重播記錄也有執行該指令。
- 16 個公開 session × 英文、中文、中文 HD、中文 HD 自訂參數四模式（未開任何強化功能），存檔原始雜湊一致，有 DOS golden 的亦吻合。
- 產物在 `uw1/build/i18n/live-enhance/`（`units.json`、`replays.json`、`live.json`、`delivery.json`）。
- 本機 Windows 測試版只換 EXE（`.poc-tools/live-enhance-deliver.py`）：搭配原有 DLL、僅系統 PATH 的重播通過；新 SHA-256
  `81486b36210fc25c868e3c5b031e06ca28407f1ccc1a4016ca7107c90354dbda`，舊檔備份同目錄 `previous-playtest-uw1port.exe`；玩家 home 107 檔更新前後雜湊一致。
- 未驗證：經由真正的 F11 畫面操作這 9 列（設定畫面的列與佇列由單元測試覆蓋，實機由使用者實測）。未執行 DOS 重編譯。
