# 打包 UW1 繁中測試版（攜帶包）

給接手的人或 Agent 照著做。所有路徑都在這台電腦上；腳本與工具在 git 之外，見 `HANDOFF.md` 第 2 節。

> **最重要的一條：** `make_play.py` 會**先刪掉輸出資料夾**再重建。**絕對不要**把輸出指到
> `UW1-zhTW-playtest`（裡面有使用者自己的存檔 `home\SAVE1～3`）。一律輸出到新的資料夾。

## 一、準備（Git Bash）

```sh
source /d/git/uw1-native-poc/.poc-tools/handoff/env.sh   # 設好 $PY、PATH，並 cd 到 uw1
git -C .. status --short                                  # 確認沒有未 commit 的改動（git 請用 PowerShell 的，見 HANDOFF）
```

## 二、編譯與檢查（全部要通過才打包）

```sh
"$PY" ../exhume/tools/portbuild.py --config exhume.toml          # → build/port/uw1port.exe
for t in tools/i18n/test_*.py; do "$PY" $t | tail -1; done        # 全部 OK／PASS
"$PY" tools/i18n/validate_authority.py                            # PASS
"$PY" tools/i18n/proper_names.py --check                          # up to date
"$PY" /d/git/uw1-native-poc/.poc-tools/p11-1-check.py p13-goblin  # all_saves_match: true
```

- 改過譯文（`i18n/zh-TW/master.tsv`）時，要先重建 catalog，見第三節。
- 不要用 MSYS 的 `timeout` 包住指令（會拿掉 TMP，clang 失敗）。

## 三、catalog（譯文檔）

- 目前最新的是 `build/i18n/p13-goblin/catalog-1.bin`。
- 譯文有改時，重建到新的批次資料夾，例如 `p13-xxx`：

```sh
D='D:/git/uw1-native-poc/.poc-private/game/UW/DATA'; A="--pak $D/STRINGS.PAK --cnv $D/CNV.ARK"
W=build/i18n/p13-xxx; mkdir -p $W
"$PY" -X utf8 tools/i18n/l10n.py validate $A                      # PASS
"$PY" -X utf8 tools/i18n/l10n.py authority $A                     # 0 errors, 0 warnings
for n in 1 2; do "$PY" -X utf8 tools/i18n/l10n.py build $A --output $W/catalog-$n.bin --manifest $W/manifest-$n.json; done
sha256sum $W/catalog-*.bin                                        # 兩個要相同
"$PY" /d/git/uw1-native-poc/.poc-tools/p11-1-check.py p13-xxx     # all_saves_match: true
```

## 四、Windows 攜帶包

```sh
OUT='D:\git\Ultima Underworld\UW1-zhTW-test-YYYYMMDD'      # 新資料夾，不能是 UW1-zhTW-playtest
"$PY" /d/git/uw1-native-poc/.poc-tools/handoff/make_play.py "$OUT" build/i18n/p13-goblin/catalog-1.bin ""
```

`make_play.py` 會放進：`uw1port.exe` 與它用到的 DLL、`fonts\jf-openhuninn-2.1.ttf`、`i18n\zh-TW\`
（catalog、`text-settings.ini`、`input-aliases.tsv`、`proper-names.tsv`、`SETTINGS.zh-TW.md`）、`licenses\`、`開始遊戲.bat`。

接著手動：

1. **遊戲資料路徑：** 第三個參數是寫進 `開始遊戲.bat` 的 `--data`。
   - 給**別人**的包：傳 `""`，然後打開 `開始遊戲.bat`，把 `--data ""` 整段刪掉。程式會自己找 GOG 的安裝，
     找不到時跳出視窗讓玩家選資料夾，選過會記住。
   - 只在**這台電腦**用：傳 `D:\git\uw1-native-poc\.poc-private\game\UW`。
2. **說明檔：** 從 `UW1-zhTW-playtest\說明.txt` 複製一份進去，內容如有新功能就補上（例如 F11「語言與文字」的設定）。
3. **確認：** 雙擊 `開始遊戲.bat`，第一次會出現 F11 設定畫面；進遊戲看中文、存一次檔。測完把 `home\` 刪掉再壓縮
   （`home\` 是測試時產生的存檔與設定）。
4. **壓縮：** 整個資料夾壓成 zip。**不要**放進遊戲資料（UW.EXE、DATA 等）；玩家要自備 GOG 版。

只更新使用者自己的 `UW1-zhTW-playtest` 時，不要用 `make_play.py`，只複製檔案：

```sh
P="/d/git/Ultima Underworld/UW1-zhTW-playtest"
cp build/port/uw1port.exe "$P/"
cp build/i18n/p13-goblin/catalog-1.bin "$P/i18n/zh-TW/catalog.uw1loc"
cp i18n/zh-TW/{proper-names.tsv,input-aliases.tsv,text-settings.ini} "$P/i18n/zh-TW/"
```

（DLL 有變時才需要整包重做；用 `llvm-objdump -p uw1port.exe` 看 DLL 名單。）

## 五、Mac 版

1. 在 GitHub 編譯（PowerShell）：

   ```powershell
   gh workflow run uw1-release.yml --repo pmanyeh/underworld-exhumed --ref feature/zh-tw-native
   gh run list --repo pmanyeh/underworld-exhumed --workflow uw1-release.yml --limit 1   # 等它完成
   gh run download <run id> --repo pmanyeh/underworld-exhumed -n macos -D mac-artifact
   ```

   artifact 裡是 `UW1-*-macos.zip`（內含 `UW1.app`），**沒有**中文檔案。
2. 本機的 `UW1-zhTW-mac-kit\` 是中文配件：`fonts\`、`i18n\zh-TW\`、`licenses\`、`說明.txt`、`開始遊戲.command`。
   先更新配件：

   ```sh
   K="/d/git/Ultima Underworld/UW1-zhTW-mac-kit"
   cp build/i18n/p13-goblin/catalog-1.bin "$K/i18n/zh-TW/catalog.uw1loc"
   cp i18n/zh-TW/{proper-names.tsv,input-aliases.tsv,text-settings.ini} "$K/i18n/zh-TW/"
   ```

3. 把 kit 複製一份，`UW1.app` 放進去，整個壓成 zip。玩家雙擊 `開始遊戲.command`。
4. **catalog 含英文原文，不可上傳到 GitHub**（只能在本機合併、私下傳給測試者）。
5. 注意：`開始遊戲.command` 還帶著 `--locale zh-TW`（P12 以前的寫法），會讓 F11 的語言選項失效。
   要讓 Mac 版也能在設定畫面切換語言，把 `--locale zh-TW --i18n-file ... --hd-scroll` 拿掉
   （資料夾裡有 catalog 時，第一次啟動就是中文）——改之前先在 Mac 上測。

## 六、交付前檢查表

- [ ] 第二節的檢查全部通過，git 已 commit、push
- [ ] 輸出資料夾是新的，不是 `UW1-zhTW-playtest`
- [ ] 包裡沒有遊戲資料、沒有 `home\`
- [ ] `開始遊戲.bat` 沒有寫死這台電腦的路徑（給別人的包）
- [ ] 說明檔已更新
- [ ] 在 `HANDOFF.md` 記下這次打包用的 commit 與 catalog
