# P11.2 — 物品名稱與組句訊息（第一階段）

**結果：PASS（第一階段）。** 名稱類字串全部翻譯，查看、狀態、食物、修理、生物態度、使用物品等常見的組句訊息
在 HD 中文文字下改以中文組成。只影響顯示；16 個公開 session 的存檔在英文與中文 HD 模式相同，也與 DOS golden 相同。

## 問題

物品名稱是 `a_sword&swords` 的格式，由 `get_name`／`fix_name_string` 依冠詞、單複數改寫；句子再由程式碼裡寫死的
英文（"You see "、" belonging to"、"The "…" is locked."）、字串檔片段與名稱拼成。逐字翻譯片段無法得到中文語序。

## 翻譯（P11.2a、P11.2b）

| 範圍 | 筆數 | 說明 |
| --- | ---: | --- |
| 物品名稱（block 4、7） | 413 | 依 Authority；中文只留名詞，冠詞、單複數、數量在組句時處理 |
| 品質、狀態、態度詞與鑰匙描述（block 5） | 103 | 品質詞依 Authority；組句時補「的」 |
| 法術與附魔名（block 6） | 213 | 依 Authority |
| 牆面、地板描述（block 10） | 264 | 63 種描述 |
| 遊戲訊息片段（block 1） | 254 | 譯文照遊戲印出的順序接得通，例如「你在深淵的」＋「第三」＋「層。」 |
| 角色建立用語（block 2） | 49 | 技能、職業、屬性依 Authority（圖片介面本身是 P12） |
| 告示開頭與牆上物件（block 8） | 30 | |

檢查工具修正：物品名稱的 `_`、`&` 視為字的邊界（`a_hand axe` 對到「hand axe」而非裡面的「axe」）。

## 程式

| 位置 | 做什麼 |
| --- | --- |
| `GAMESTRN.C` | `port_zh*` 組句工具：取字串或名稱的譯文，沒有譯文就用英文；`game_strings_3` 改接三段譯文；`get_name` 記下英文名稱與其譯文 |
| `src/port/i18n/phrases.c` | 程式碼寫死英文的中文片語（遊戲原始碼維持純 ASCII，只用代號）；單獨印出的整段短句對照（"The "、" is locked.\n"、"Yes"…） |
| `scroll_bridge.c` | 顯示前：剛由 `get_name` 寫出的名稱換成中文；整段短句對照；接在中文後的 "."、", "、" and "、"very near" 換成「。」「、」「就在附近」 |
| `LOOK.C` | 查看物品（品質、魔力、附魔、充能、擁有者）、生物（態度、種類、名字）、書、骨頭、墓碑與牆上物件 |
| `INTERACT.C` | 查看牆面地板、生命力與法力 |
| `USEITEMS.C`、`WORLDEV.C`、`CRITTIME.C` | 「要把X用在什麼上？」、食物味道、修理估計與結果、生物察覺你的舉動 |
| `src/port/sys/cheats.c` | 測試指令 `look ITEM [QTY [QUALITY [LORE]]]`、`lookwall N`（只在測試環境） |

## 驗證

| 檢查 | 結果 |
| --- | --- |
| validate／authority（整份 master） | PASS；0 errors、0 warnings |
| 單元測試、`test_scroll_text.py` | OK；新增名稱記錄、短句對照、中文句點的檢查 |
| 16 個公開 session，英文與 zh-TW HD | 存檔兩種模式相同，也與 DOS golden 相同；`verify_localized_replay.py` PASS |
| 遊戲內（測試指令） | 「你看到堪用的長劍。」「你看到 3 個隔日的玉米。」「你看到不悅的腐蟲。」「你看到石牆。」「你看到庫爾尼斯的寶石切割器。」 |
| 遊戲內（點羅盤） | 「你目前吃得很飽，而且清醒。」「你在深淵的第一層。」「今天是你被囚禁的第一天。」「你猜現在大概是傍晚。」 |

**重播抓到的錯誤：** 「Use key on what?」的中文分支一度提早 return，跳過了把鑰匙放上游標的遊戲邏輯，
fallrestore／fallpause 的存檔因此不同；已改為只替換印出的文字。

## 尚未涵蓋（下一階段）

戰鬥訊息、法術施放結果、背包與容器訊息（INVPANEL、COMBAT、SPELLS 等其餘組句點）、神殿祈禱語輸入框、
存檔選單的其他字、遊戲標題字串。
