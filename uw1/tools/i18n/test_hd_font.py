#!/usr/bin/env python3
"""Compile/run real C FreeType rendering tests; no SDL window or game assets."""
from portsrc import RUNTIME, port_src
import argparse
import os
from pathlib import Path
import shutil
import shlex
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    uw1 = Path(__file__).resolve().parents[2]
    parser.add_argument("--font", type=Path, default=uw1 / "tools/fonts/jf-openhuninn-2.1.ttf")
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--window", action="store_true", help="also run actual SDL resize/fullscreen checks")
    parser.add_argument("--no-freetype", action="store_true", help="compile/link/run the optional dependency stub")
    args = parser.parse_args()
    root = uw1.parent
    out = uw1 / "build/hd-text"
    out.mkdir(parents=True, exist_ok=True)
    cc = os.environ.get("CC") or shutil.which("clang") or "cc"
    env = dict(os.environ)
    pc = str(uw1 / "tools/libs/lib/pkgconfig")
    env["PKG_CONFIG_PATH"] = pc + os.pathsep + env.get("PKG_CONFIG_PATH", "")
    if args.no_freetype and args.window:
        parser.error("--no-freetype and --window are separate checks")
    flags = [] if args.no_freetype else shlex.split(subprocess.check_output(
        ["pkg-config", "--cflags", "--libs", "freetype2"] +
        (["sdl3"] if args.window else []), text=True, env=env))
    source = "hd_stub_test.c" if args.no_freetype else "hd_window_test.c" if args.window else "hd_font_test.c"
    stem = "stub" if args.no_freetype else "window" if args.window else "font"
    stem += "-asan" if args.sanitize else ""
    exe = out / (stem + (".exe" if os.name == "nt" else ""))
    common = [str(port_src(n)) for n in
              ["presentation/hd_font.c", "i18n/utf8.c", "i18n/layout.c"]]
    if args.window:
        common += [str(port_src('presentation/' + n)) for n in ["hd_demo.c", "overlay_demo.c"]]
    command = [cc, "-std=c11", "-g", "-O1", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
               "-I" + str(uw1 / "src/port"), "-I" + str(RUNTIME),
               "-I" + str(root / "exhume/runtime/port/platform"),
               "-I" + str(root / "exhume/runtime/port"),
               str(Path(__file__).with_name(source))] + common + [
               str(root / "exhume/runtime/port/platform/png.c"), "-o", str(exe)]
    if args.sanitize:
        command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    if not args.no_freetype:
        command += ["-DHD_HAVE_FREETYPE"]
    subprocess.run(command + flags + (["-lm"] if os.name != "nt" else []), check=True)
    env["ASAN_OPTIONS"] = "halt_on_error=1:detect_leaks=" + ("0" if os.name == "nt" else "1")
    env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    env["PATH"] = str(uw1 / "tools/libs/bin") + os.pathsep + env["PATH"]
    result = subprocess.run([str(exe), str(args.font.resolve()), str(out / (stem + ".png"))],
                            capture_output=True, text=True, env=env)
    print(result.stdout, end="", flush=True)
    print(result.stderr, end="", flush=True)
    result.check_returncode()
    print("PASS " + ("optional stub" if args.no_freetype else "FreeType compile/link/render") +
          (" with ASan/UBSan" if args.sanitize else ""))


if __name__ == "__main__":
    main()
