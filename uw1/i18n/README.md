# Native localization data (Phase 1)

這裡目前只有文字盤點與翻譯資料；遊戲尚未讀取這些 TSV，也尚未繪製中文。
Phase 1 的驗收與證據見 [TEXT-INVENTORY.md](../docs/i18n/TEXT-INVENTORY.md)。

## Reproduce with your own game data

Python 3.10+ standard library 即可執行盤點與驗證。在 `uw1/` 執行：

```sh
python3 tools/i18n/extract_strings.py "$UW1_DATA/DATA/STRINGS.PAK" build/i18n/strings.tsv \
  --cnv "$UW1_DATA/DATA/CNV.ARK" --metadata build/i18n/metadata.json
python3 tools/i18n/scan_literals.py build/i18n/literals.tsv
python3 tools/i18n/validate_poc.py "$UW1_DATA/DATA/STRINGS.PAK" \
  "$UW1_DATA/DATA/CNV.ARK" i18n/zh-TW/poc.tsv
python3 -m unittest discover -s tools/i18n -p test_i18n.py -v
```

Windows 用原有 MSYS2 CLANG64 / Python 環境；本機路徑與 libraries 設定沿用
[Phase 0](../docs/i18n/PREFLIGHT.md)。原版資料位於玩家指定的位置，不會由工具下載。
完整盤點輸出放在已忽略的 `build/i18n/`，不提交原版資料或完整英文文字庫。

Native C 交叉驗證需先成功 `make port`，沿用相同 compiler、DLL/library 環境：

```sh
python3 tools/i18n/verify_native.py "$UW1_EXE" "$UW1_DATA/DATA/STRINGS.PAK" \
  --report build/i18n/native-comparison.json
```

這會連結原有 game/runtime objects 與小型 test driver，直接呼叫未修改的
`GAMESTRN.C` decoder。程式不初始化遊戲畫面、不修改遊戲來源；對每個 PAK entry
比較 C 與 Python 輸出的 bytes，另驗證 block 0 與兩個 dynamic block。
這個交叉驗證 driver 針對本 PoC 的 UW1 GOG baseline，並要求 Bragit 的文字存在。

## TSV contract

UTF-8、無 BOM、LF 換行。先使用標準 TSV/CSV reader（delimiter 為 tab）讀欄位，
再逐欄呼叫 `extract_strings.unescape()`。引號遵循 CSV quoting，不能以單純 `split('\t')`
取代 reader。欄位內的 `\\`、`\n`、`\r`、`\t`、`\0` 分別代表 literal backslash、
newline、CR、tab、NUL。其他反斜線 escape 拒絕接受；不得用 `eval` 或通用 Python escape decoder。

例如遊戲的反斜線加 `m` 在 TSV 中寫成 `\\m`；真正換行寫成 `\n`。
兩者不可混淆。原版 bytes 以 CP437 可逆映射至 Unicode；這次 10,984 筆實際輸出皆為 ASCII。
解碼結果遵循遊戲 C string 語意：第一個 NUL 結束、最多 511 bytes，不是原始壓縮 bitstream 的備份。

`strings.tsv` 欄位為 `block, index, category, source_en`，block/index 使用十進位。
穩定排序為 block、index；保留空字串及所有磁碟 entry，包括超出 raw ID 的 41 個尾端 entries。
後者屬於盤點，不能直接當作 runtime `get_string` key。

`zh-TW/poc.tsv` 欄位為 `kind, block, index, source_key, role, source_en, zh_tw, note`。

| kind | identity | 用途 |
| --- | --- | --- |
| `pak` | `pak:<resolved block>:<index>` | 靜態字串，例如 `pak:3651:3` |
| `literal` | `literal:<repo path>:<call>:<text SHA256 前16碼>:<occurrence>` | 獨立 namespace；block/index 留白 |

Literal identity 不依賴行號；path、call、內容或同值出現次序改動會使 key 改變，需重新審查。
`source_en` 是 drift check，來源有變時拒絕套用舊翻譯。譯文是 PoC 草稿，尚待畫面與用語校對。
`role` 是翻譯備註，不控制遊戲流程；同一個 key 可由多個 script branch 引用。

## Runtime identity and lifetime

以 [GAMESTRN.C](../src/ui/GAMESTRN.C)、[BABL.C](../src/conv/BABL.C) 與
[CONVERSE.C](../src/conv/CONVERSE.C) 為準：

1. `raw id` 先轉 uint16，`block = id >> 9`、`index = id & 0x1ff`。
2. **先找 made/dynamic blocks**。`0x7C` 是對話執行期組成的字串，`0x7D` 是玩家姓名。
   它們繞過 static lookup；不得把使用者輸入／執行期結果當作英文原文翻譯。
3. 若 raw block 為 0，以 `CutsceneOrConversationStringBlock` 解析真正 block；
   對話從 CNV.ARK header 取得，過場由 `CUTS.C` 設定為 `0xC00 + n`。
4. 其餘直接以 `(block,index)` 讀 PAK。對話的 resolved block 常為 `0xE00+...`，
   但必須讀 header，不能假設所有 slot 都符合公式。高 block 不可再塞回 16-bit packed ID。
5. `read_string` 使用八個 512-byte 循環 slots，最多輸出 511 bytes。
   翻譯 lookup 的未來生命週期管理不能改壞原本 buffer contract。

Bragit 的 CNV slot / whoami 為 67，header 指向 `0xE43`（3651），有 1,695 code words；
PAK 共 50 entries，0 為空，1–49 為 template fragment、玩家選項與 NPC 台詞。
姓名在 block 7、index 83。PoC 全數涵蓋這 49 個非空 entries 與姓名；
另收 block 1 的 2、5 及 `SCROLLIO.C` 的 `[MORE]`，共 53 筆。

靜態 template 可以翻譯，但 `@SS1` 等 BABL markers 必須完整保留；它們在顯示前由
`convert_string` 展開。`@GI` 是整數、`@GS` 是字串、`S` 為 local、`P` 為間接 local、
`C` 為常數，`@@` 表示 literal `@`；索引本身可遞迴引用整數變數。
驗證器保留 marker 次數與完整索引、控制碼順序及明示換行數；未知語法直接報錯。
目前沒有實作 dynamic template translation strategy。

原版物品名稱的 `article_singular&plural` 是 `fix_name_string` 的語法，不能當作普通整句替換。
這階段只盤點，不翻譯物品名稱；文法與中文物品命名留待相應階段處理。

## Hard-coded literal triage

掃描 `uw1/src`、固定 pin 的 `exhume/runtime/port` 及本工具的 C driver。
先去除 comments，再處理字元常數、C escapes、相鄰字串與 multiline literal；
保留原始 C spelling、path、行號、最近 call context、classification 與 reason。
全數輸出分為 `PLAYER_VISIBLE / DEBUG_ONLY / PORT_UI / TEST_ONLY / UNKNOWN`。

這是保守的 lexical inventory，沒有執行 preprocessor 或完整 interprocedural data-flow。
`UNKNOWN` 包含 include/path、format、命名陣列與尚待人工追蹤的 UI 文案；
不能將 UNKNOWN 當成 DEBUG_ONLY，也不宣稱已列出每個實際 runtime display event。
未在遊戲目錄外掃描原版 binary 的 literals。

已識別範例包含 `scroll_print` / `string_to_screen` 的直接文案、
`INTERACT.C` 的 `You see ` 片段、角色建立的 Str/Dex/Int/Vit labels、
`SCROLLIO.C` 的 `[MORE]`，以及 native settings 的命名 label arrays、`plat_message` dialogs。
片段須連同後續拼接與文法審查；尚未實作任何 literal runtime replacement。
