# 高解析中文字的設定

中文字以視窗解析度、jf open 粉圓字型繪製（高解析中文字）。字型、字級、字距、行距等都可以調整；
設定名稱與 Ultima VIII 中文化的 `scummvm.ini` 相同。設定介面之後再做，目前請直接修改設定檔。

## 一、設定檔在哪裡

設定有三層，後面的優先：

| 順序 | 檔案 | 用途 |
| --- | --- | --- |
| 1 | 程式內建 | 下表的預設值 |
| 2 | `i18n/zh-TW/text-settings.ini` | 繁體中文的預設值，隨中文版提供。**請不要改這個檔案**，更新時會被覆蓋 |
| 3 | `uw1port.cfg` | 你自己的設定。Windows 在 `%APPDATA%\uw1port`，macOS 與 Linux 在 `~/.uw1port` |

想調整時，把要改的那一行加到 `uw1port.cfg` 即可；刪掉那一行就回到預設值。

1. **先關閉遊戲**。遊戲的設定畫面（F11）存檔時只改它自己的設定，你加的中文設定與註解都會保留，但開著遊戲修改，可能被覆蓋。
2. 用文字編輯器開啟，存成 **UTF-8**。
3. 一行一個設定：`名稱=值`。以 `#` 開頭的行是註解；數值後面不要加註解。
4. 同一個名稱寫了兩次，以後面那一行為準。
5. 數值打錯或超出範圍時，遊戲會在記錄中說明，並使用原本的值，不會無法啟動。

## 二、設定項目

單位是**遊戲像素**（畫面寬 320），可以有小數，例如 `0.3`；遊戲會依視窗大小自動換算，換視窗大小不用重調。

| 設定 | 預設 | 範圍 | 說明 |
| --- | --- | --- | --- |
| `font_cjk_hd_file` | `jf-openhuninn-2.1.ttf` | | 字型檔（`.ttf` 或 `.otf`）。見第四節 |
| `font_cjk_hd_fallback_file` | （無） | | 缺字時改用的字型檔。目前的譯文 jf open 粉圓都有，不需要設定 |
| `font_cjk_hd_size` | `5` | 2～16 | 字級 |
| `font_cjk_hd_letter_spacing` | `0` | -2～8 | 字與字之間多加的間距 |
| `font_cjk_hd_latin_spacing` | `0` | -2～8 | 中文與英文字母、數字相接處再多加的間距 |
| `font_cjk_hd_line_spacing` | `0` | -3～16 | 行與行之間多加的間距 |
| `font_cjk_hd_baseline` | `0` | -4～4 | 文字往下移的距離，負數往上。只改位置，不影響換行 |
| `font_cjk_hd_border` | `0` | 0～2 | 文字黑框的粗細，`0` 為無黑框 |
| `font_cjk_hd_antialiasing` | `true` | | 反鋸齒：`true` 平滑；`false` 不平滑（點陣感） |

- 字級、字距、行距會改變**一行放幾個字、一頁放幾行**：字越大、行距越大，越常出現「[MORE]」翻頁。
  這只影響顯示，不影響遊戲進行與存檔（已用公開錄影驗證）。
- 字級比行高（8）大很多時，上下兩行的字會互相重疊，請同時加大行距。

## 三、只改某一類文字

設定名稱改成 `font_cjk_<類別>_<項目>`，就只影響那一類；沒寫的項目沿用 `font_cjk_hd_` 的設定。

| 類別 | 範圍 |
| --- | --- |
| `message` | 主畫面下方的訊息捲軸。**對話時的選項也顯示在這裡**，所以跟著這一類 |
| `talk` | 對話時 NPC 說話的文字框 |
| `book` | 書本與卷軸。之後的版本才會以高解析顯示，目前設定沒有作用 |

例如對話框的字大一點、行距寬一點：

```ini
font_cjk_talk_size=6
font_cjk_talk_line_spacing=1.5
```

## 四、換字型

把字型檔放進下列任一個 `fonts` 資料夾，再設定 `font_cjk_hd_file=檔名`；也可以直接寫完整路徑。

1. 遊戲設定資料夾（`uw1port.cfg` 所在處）裡的 `fonts`
2. 啟動遊戲時所在資料夾裡的 `fonts`（直接點兩下執行檔時，就是執行檔所在處）
3. 開發環境的 `tools/fonts`

字型必須是 TrueType（`.ttf`）或 OpenType（`.otf`）。如果有字顯示成方塊，表示該字型缺字，請換回
`jf-openhuninn-2.1.ttf`，或用 `font_cjk_hd_fallback_file` 指定一個字比較齊全的字型補字。
Windows 內附的字型（例如微軟正黑體）只能自己使用，不能隨遊戲散布。

## 五、常見的調整範例

**字大一點，行距跟著加大**：

```ini
font_cjk_hd_size=6
font_cjk_hd_line_spacing=1
```

**字距鬆一點，中英文之間留點空隙**（與 Ultima VIII 中文化相同）：

```ini
font_cjk_hd_letter_spacing=0.3
font_cjk_hd_latin_spacing=1
```

**想要點陣感**：

```ini
font_cjk_hd_antialiasing=false
```

## 六、字型授權

jf open 粉圓 2.1（justfont）以 SIL Open Font License 1.1 授權，可隨遊戲散布；授權全文
`jf-openhuninn-OFL.txt` 須與字型檔放在一起。開發用的取得方式：`python3 tools/i18n/fetch_font.py`
（固定版本與 SHA-256，下載到不提交的 `tools/fonts/`）。
