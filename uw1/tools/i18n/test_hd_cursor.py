#!/usr/bin/env python3
"""Build/run cursor pixel and concurrent publication checks (no game assets)."""
import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--window", action="store_true", help="real SDL compositor resize/fullscreen checks with HD text")
    args = parser.parse_args()
    uw1 = Path(__file__).resolve().parents[2]
    root = uw1.parent
    out = uw1 / "build/hd-cursor"
    out.mkdir(parents=True, exist_ok=True)
    stem = "window" if args.window else "cursor"
    target = out / (stem + ("-asan" if args.sanitize else ""))
    if os.name == "nt":
        target = target.with_suffix(".exe")
    cc = os.environ.get("CC") or shutil.which("clang") or "cc"
    env = dict(os.environ)
    env["PKG_CONFIG_PATH"] = str(uw1 / "tools/libs/lib/pkgconfig") + os.pathsep + env.get("PKG_CONFIG_PATH", "")
    flags = shlex.split(subprocess.check_output(["pkg-config", "--cflags", "--libs", "sdl3"] +
        (["freetype2"] if args.window else []), text=True, env=env))
    command = [cc, "-std=c11", "-g", "-O1", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
               "-I" + str(uw1 / "src/port"), "-I" + str(root / "exhume/runtime/port/platform"),
               "-I" + str(root / "exhume/runtime/port"),
               str(Path(__file__).with_name("cursor_window_test.c" if args.window else "hd_cursor_test.c")),
               str(uw1 / "src/port/presentation/hd_cursor.c"), "-o", str(target)]
    if args.window:
        command += ["-DUW1_HAVE_FREETYPE"] + [str(uw1/"src/port"/p) for p in [
            "presentation/presentation.c", "presentation/hd_demo.c", "presentation/overlay_demo.c",
            "presentation/hd_font.c", "i18n/utf8.c", "i18n/layout.c"]]
        command += [str(root/"exhume/runtime/port/platform/png.c")]
        if os.name != "nt":
            command += ["-lm"]
    command += flags
    if args.sanitize:
        command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    subprocess.run(command, check=True)
    env["ASAN_OPTIONS"] = "halt_on_error=1:detect_leaks=" + ("0" if os.name == "nt" else "1")
    env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    env["PATH"] = str(uw1 / "tools/libs/bin") + os.pathsep + env["PATH"]
    run = [str(target)]
    if args.window:
        run += [str(uw1/"tools/fonts/NotoSansCJKtc-Regular.otf"), str(out/stem)]
    result = subprocess.run(run, capture_output=True, text=True, env=env)
    print(result.stdout, end="", flush=True)
    print(result.stderr, end="", flush=True)
    result.check_returncode()


if __name__ == "__main__":
    main()
