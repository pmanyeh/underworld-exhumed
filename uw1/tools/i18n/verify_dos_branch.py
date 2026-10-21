#!/usr/bin/env python3
"""Compare the DOS branch's C tokens with the Phase 1 baseline.

This is a structural guard, NOT a Turbo C compile or DOS exact-match check.
Headers are excluded on both sides; the same host preprocessor selects the
__TURBOC__ branch. The actual Borland check remains a separate reference gate.
"""
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", default="zh-tw-native-p1")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[3]
    path = "uw1/src/ui/GAMESTRN.C"
    original = subprocess.run(["git", "show", f"{args.baseline}:{path}"], cwd=repo,
                              capture_output=True, text=True, check=True).stdout
    current = (repo / path).read_text(encoding="utf-8")
    compiler = shutil.which("clang")
    if not compiler:
        raise SystemExit("Clang required for the structural preprocessor guard")
    outputs = []
    for text in (original, current):
        without_headers = re.sub(r"(?m)^\s*#\s*include[^\n]*", "", text)
        result = subprocess.run([compiler, "-E", "-P", "-x", "c", "-D__TURBOC__", "-"],
                                input=without_headers, text=True, capture_output=True, check=True)
        outputs.append(result.stdout.split())
    if outputs[0] != outputs[1]:
        raise SystemExit("FAIL: DOS branch token stream changed")
    digest = hashlib.sha256(" ".join(outputs[0]).encode()).hexdigest()
    print(f"PASS: DOS branch unchanged relative to {args.baseline}; token SHA256 {digest}")
    print("NOT a DOS exact-match result: headers excluded, Clang preprocessor only")


if __name__ == "__main__":
    main()
