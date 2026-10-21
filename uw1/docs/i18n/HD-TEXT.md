# Phase 5 — FreeType HD Text

## Status

Implementation checkpoint；Windows renderer tests 已通過，跨平台 CI 與完整本輪 evidence 正在整理。
本階段仍依 [Native-only 規則](VALIDATION.md)，沒有 DOS／Borland gate。
只有獨立文字示範；未將譯文接入 conversation、未取代原游標，也未開始 Phase 6。

## Baseline

| 項目 | Phase 開始版本 |
| --- | --- |
| Underworld upstream/main | `e5deadd6945b492636aadccc14f52caa908b4186` |
| Underworld feature | `2963fb5c868a5687bca1b58b96e6713101395ec7` |
| Exhume upstream/master | `c54e00e9176c14d0913b5806318a30f85a8f4636` |
| Exhume overlay pin | `89278495bb0b8b224b5bb58e53da2fd367c0d8c0` |

本階段不需修改 Exhume。所有 font/layout policy、demo 與 tests 均在 UW1。

## Renderer contract

[hd_font.h](../../src/port/presentation/hd_font.h) 提供 opaque `HdFont`、init/shutdown、
native/logical measure 與 logical-coordinate draw；header 不暴露 SDL 或 FreeType types。
Font、cache 及所有呼叫均由 presentation thread 擁有；main 在啟動前建立，`plat_run` 返回後釋放。
Web browser loop 未返回時不提早釋放；無 FreeType build 使用明確 `HD_UNAVAILABLE` stub。
demo 只讀固定文字與 callback geometry，不接觸 game-thread mutable strings。

每個 `HdFont` owns 一個 scalable Unicode face。Cache key 為 face ownership、pixel size、Unicode
codepoint 與 render mode；目前只實作 `FT_RENDER_MODE_NORMAL` grayscale。512 entries / 8 MiB bitmap
雙重上限，LRU eviction，glyph mask owns copy，不保留 FreeType 可變 glyph-slot 指標。
Stats 可觀察 hit/miss/rasterization/eviction/missing glyph；不存在的字以 U+FFFD 或 `.notdef` 呈現。

使用 Phase 3 UTF-8 validator / layout，整數 26.6 advances、中文標點禁則與 logical wrapping。
使用 backend float viewport，X/Y scale 分開；font ppem 按 logical em × Y scale 四捨五入，範圍 1–256。
文字 glyph 在 output 保持自然比例；native width / X scale、native height / Y scale 得到 logical metrics。
重新設定輸出尺寸時，按新 ppem 直接從 outline 產生 mask，既有大小則從 cache 取用。
沒有把 glyph 寫進或放大 320×200 framebuffer。

Baseline 使用 face ascender；多行依 face line height，metrics 是 advance bounds，不是 ink bounding box。
Draw 以 clipped、straight-alpha source-over 合成到 RGBA，支援已有透明背景的 layer。
UTF-8 不合法、NUL、無效 geometry 會在畫 pixel 前拒絕。PoC 不做 kerning、shaping、完整 grapheme 或
game control execution；控制碼仍是 zero-width units。

FreeType 行為參考官方 [glyph slot/render](https://freetype.org/freetype2/docs/reference/ft2-glyph_retrieval.html)
與 [bitmap pitch/gray](https://freetype.org/freetype2/docs/reference/ft2-basic_types.html#ft_bitmap)。

## Font / License

- Font：**Noto Sans CJK TC Regular**，Version 2.004，OTF/CFF。
- Copyright metadata：© 2014–2021 Adobe (http://www.adobe.com/)。
- Source：[notofonts/noto-cjk](https://github.com/notofonts/noto-cjk)，commit `f8d157532fbfaeda587e826d4cd5b21a49186f7c`，
  `Sans/OTF/TraditionalChinese/NotoSansCJKtc-Regular.otf`。
- Font SHA-256：`dce08bd4fd91aa8aa76ed8fea4b694c2dfb8550f67871e326843212ddbeb88b4`。
- License：SIL Open Font License 1.1，原文見 [Noto-OFL.txt](licenses/Noto-OFL.txt)。
  原檔下載／cache／顯示，不修改或 subset，也不將 font binary 提交 repository。
  可以與軟體一起散布，須保留 copyright / license，不能單獨出售字型；衍生版本須遵守 OFL 命名與授權條件。
  這裡只散布 fetch tool 與 renderer；目前未製作新的 release package。

[fetch_font.py](../../tools/i18n/fetch_font.py) 固定 commit 與兩份檔案 SHA-256，
同時下載 font 與授權至 ignored `tools/fonts/`。參見官方 [OFL text](https://raw.githubusercontent.com/notofonts/noto-cjk/main/Sans/LICENSE)。

FreeType 使用 FreeType License（FTL），見 [FTL.txt](licenses/FTL.txt)。本機測試版本 2.14.3，
官方 tag `VER-2-14-3`；獨立 source build 關閉 HarfBuzz/Brotli/BZip2/PNG/zlib optional deps。
This software uses the FreeType project. Copyright © 1996–2026 The FreeType Project.
FreeType source/binary 未提交；若之後製作套件，須包含相應 library 與 license。

## Build / Validation

依 [README](../../i18n/README.md) 安裝 development package、下載 font、建置與執行 `--test-hd-text`。
三句示範：`你好，冒險者。`、`Ultima Underworld 中文測試`、`English + 中文 123`。
無 FreeType 或字型無法載入時，明示 opt-in 會輸出診斷並返回非零；原英文路徑仍可建置執行。

`test_hd_font.py` 將真實 renderer、Phase 3 core 與 PNG writer 編譯成 standalone C executable，
不需遊戲 assets。測量、glyph cache、AA、alpha over existing layer、malformed UTF-8、不改寫失敗 output、
wrapping、不同 font instance / size、800 個 CJK glyphs 的 LRU 壓力、pitch padding 與 guard bytes 都有 assertions。
`--sanitize` 以 ASan/UBSan 重跑；`--window` 復用 Exhume generic fixture，通過真實 SDL backend compositor。

本輪最終測試結果、圖片、CI run 與同步 Gate 將於驗收完成後填入。
