#!/usr/bin/env python3
"""Fetch the unmodified OFL 1.1 fonts and their licenses: jf open huninn 2.1, the HD
Chinese text's default (P10T; the Ultima VIII translation's font), and Noto Sans CJK TC,
the earlier phases' test font.

Pinned upstream release or commit and SHA256, no system font lookup or font modification.
Files remain under ignored tools/fonts/. Keep each license beside redistributed font files.
See docs/i18n/HD-TEXT.md for attribution and distribution terms.
"""
import argparse
import hashlib
from pathlib import Path
import urllib.request

REVISION = "f8d157532fbfaeda587e826d4cd5b21a49186f7c"
BASE = f"https://raw.githubusercontent.com/notofonts/noto-cjk/{REVISION}/Sans/"
# jf open huninn: the v2.1 release's font, and the license at the commit v2.1 tags
HUNINN_REVISION = "98d53b3dac1730889edf548359c326c53624fa80"
FILES = [
    ("https://github.com/justfont/open-huninn-font/releases/download/v2.1/jf-openhuninn-2.1.ttf",
     "jf-openhuninn-2.1.ttf", "9d5bf4932d31fe94c18cd8cfddc98bc1b14ce10f4e354c682179db290a99c825"),
    (f"https://raw.githubusercontent.com/justfont/open-huninn-font/{HUNINN_REVISION}/LICENSE",
     "jf-openhuninn-OFL.txt", "5df6de36ba2fe26119ac161edbed97c62a9da3c1493f6930b9d9af7f5c93004b"),
    (BASE + "OTF/TraditionalChinese/NotoSansCJKtc-Regular.otf", "NotoSansCJKtc-Regular.otf",
     "dce08bd4fd91aa8aa76ed8fea4b694c2dfb8550f67871e326843212ddbeb88b4"),
    (BASE + "LICENSE", "LICENSE.txt", "6a73f9541c2de74158c0e7cf6b0a58ef774f5a780bf191f2d7ec9cc53efe2bf2"),
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
            with urllib.request.urlopen(remote, timeout=90) as stream:
                data = stream.read(24 * 1024 * 1024 + 1)
            if hashlib.sha256(data).hexdigest() != digest:
                raise RuntimeError(f"SHA256 mismatch: {name}")
            target.write_bytes(data)
        print(f"Verified {name}: {digest}")


if __name__ == "__main__":
    main()
