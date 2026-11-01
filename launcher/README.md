# The release's front page

`launcher.c` is `Underworld.exe` in the Chinese release of both games: one page with the two
games' title screens, which starts `uw1port` or `uw2port` and comes back when the game ends.

- **Finding the games.** `uw1port --find-game` and `uw2port --find-game` (sys/main.c) search as
  a player's start does (exhume's `sys/gamedir.c`: the remembered folder, GOG's registry entries,
  GOG's install folders, a GOG CD image copied out once), remember the folder and exit 0, or exit
  1. The page asks both on a thread. A game not found is started all the same and asks for its
  folder in its own dialog; the page asks again when it ends.
- **The pictures** are the player's own: UW1's `DATA/OPSCR.BYT` and UW2's block 5 of
  `DATA/BYT.ARK` (LZSS), palette 2 of `DATA/PALS.DAT`. Nothing of the games is shipped.
- **Text** is the games' HD font through `presentation/hd_font.c`, drawn into an RGBA canvas
  that SDL scales to the window.

The release folder (`package.py`): `Underworld.exe`, `uw1port.exe`, `uw2port.exe`, the DLLs they
import, `fonts/`, `licenses/`, `說明.txt` (from `README.zh-TW.txt`), `MANIFEST.json`, and the
games' home folders `UW1/` and `UW2/` with their Chinese files in `i18n/zh-TW`. Each game runs with
its home folder there and the release folder as its current one, so it finds its catalog and
the fonts as a playtest folder's game does, and starts in Chinese with every enhancement off.

    python3 launcher/build.py --version 0.7
    python3 launcher/package.py --output DIR --uw1-catalog FILE --uw2-catalog FILE \
        --toolchain MSYS2_CLANG64 --version 0.7

Tests: `Underworld.exe --screenshot FILE.bmp` writes the page once both games are searched for and
quits; with `--test-play 0` or `1` it first starts that game (which quits after four seconds) and
waits for it.
