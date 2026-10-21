#!/usr/bin/env python3
"""Fetch the unmodified OFL 1.1 Noto Sans CJK TC test font and its license.

Pinned upstream commit and SHA256, no system font lookup or font modification.
Files remain under ignored tools/fonts/. Keep LICENSE.txt beside redistributed
font files. See docs/i18n/HD-TEXT.md for attribution and distribution terms.
"""
import argparse
import hashlib
from pathlib import Path
import urllib.request

REVISION = "f8d157532fbfaeda587e826d4cd5b21a49186f7c"
BASE = f"https://raw.githubusercontent.com/notofonts/noto-cjk/{REVISION}/Sans/"
FILES = [
    ("OTF/TraditionalChinese/NotoSansCJKtc-Regular.otf", "NotoSansCJKtc-Regular.otf",
     "dce08bd4fd91aa8aa76ed8fea4b694c2dfb8550f67871e326843212ddbeb88b4"),
    ("LICENSE", "LICENSE.txt", "6a73f9541c2de74158c0e7cf6b0a58ef774f5a780bf191f2d7ec9cc53efe2bf2"),
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path(__file__).resolve().parents[2] / "tools/fonts")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    for remote, name, digest in FILES:
        target = args.output / name
        data = target.read_bytes() if target.exists() else b""
        if hashlib.sha256(data).hexdigest() != digest:
            with urllib.request.urlopen(BASE + remote, timeout=90) as stream:
                data = stream.read(24 * 1024 * 1024 + 1)
            if hashlib.sha256(data).hexdigest() != digest:
                raise RuntimeError(f"SHA256 mismatch: {name}")
            target.write_bytes(data)
        print(f"Verified {name}: {digest}")


if __name__ == "__main__":
    main()
