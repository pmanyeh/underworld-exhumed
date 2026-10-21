#!/usr/bin/env python3
"""Compile/run the standalone core stress driver with ASan and UBSan."""
import os
from pathlib import Path
import shutil
import subprocess


def main():
    uw1 = Path(__file__).resolve().parents[2]
    out = uw1 / "build/i18n"
    out.mkdir(parents=True, exist_ok=True)
    target = out / ("layout-stress.exe" if os.name == "nt" else "layout-stress")
    compiler = os.environ.get("CC") or shutil.which("clang")
    if not compiler:
        raise SystemExit("Clang with AddressSanitizer/UndefinedBehaviorSanitizer required")
    subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-g", "-O1",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    "-I", str(uw1 / "src/port"), str(uw1 / "tools/i18n/layout_stress.c"),
                    str(uw1 / "src/port/i18n/utf8.c"), str(uw1 / "src/port/i18n/layout.c"),
                    "-o", str(target)], check=True)
    env = dict(os.environ)
    env["ASAN_OPTIONS"] = "halt_on_error=1:detect_leaks=" + ("0" if os.name == "nt" else "1")
    env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    subprocess.run([str(target)], env=env, check=True)
    print("ASan + UBSan PASS (Windows: leak detection unavailable)" if os.name == "nt" else "ASan + UBSan PASS")


if __name__ == "__main__":
    main()
