#!/usr/bin/env python3
"""Finishes the macOS release ZIP that package_macos.py made in CI: adds what CI cannot have, the
two catalogs (built from the player's game data by l10n.py build) and the font (not in git), into
the release folder in the ZIP, and copies every entry already there byte for byte, with its
modes, so that the app's signature and the start script's execute bit stay as CI made them.

    python3 launcher/finish_release.py --zip CI.zip --out FINAL.zip \
        --uw1-catalog FILE --uw2-catalog FILE
"""
import argparse, hashlib, os, zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--zip', required=True)
    p.add_argument('--out', required=True)
    p.add_argument('--uw1-catalog', required=True)
    p.add_argument('--uw2-catalog', required=True)
    a = p.parse_args()
    if os.path.exists(a.out):
        p.error('the output must be new')
    fonts = os.path.join(REPO, 'uw1', 'tools', 'fonts')
    with zipfile.ZipFile(a.zip) as src:
        names = src.namelist()
        top = names[0].split('/')[0]
        if not all(n.startswith(top + '/') for n in names):
            p.error('the ZIP holds more than one top folder')
        add = {f'{top}/UW1/i18n/zh-TW/catalog.uw1loc': a.uw1_catalog,
               f'{top}/UW2/i18n/zh-TW/catalog.uw2loc': a.uw2_catalog,
               f'{top}/fonts/jf-openhuninn-2.1.ttf': os.path.join(fonts, 'jf-openhuninn-2.1.ttf'),
               f'{top}/licenses/jf-openhuninn-OFL.txt': os.path.join(fonts, 'jf-openhuninn-OFL.txt')}
        clash = [n for n in add if n in names]
        if clash:
            p.error('already in the ZIP: ' + ', '.join(clash))
        with zipfile.ZipFile(a.out, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as out:
            for info in src.infolist():
                out.writestr(info, src.read(info), compress_type=zipfile.ZIP_DEFLATED, compresslevel=6)
            if f'{top}/fonts/' not in names:
                d = zipfile.ZipInfo(f'{top}/fonts/', date_time=src.infolist()[0].date_time)
                d.external_attr = (0o40755 << 16) | 0x10
                out.writestr(d, b'')
            for arc, path in add.items():
                info = zipfile.ZipInfo(arc, date_time=src.infolist()[0].date_time)
                info.external_attr = 0o100644 << 16
                info.create_system = 3                  # Unix, so the modes mean something
                with open(path, 'rb') as f:
                    out.writestr(info, f.read(), compress_type=zipfile.ZIP_DEFLATED, compresslevel=6)
    with open(a.out, 'rb') as f:
        print(a.out, hashlib.sha256(f.read()).hexdigest())


if __name__ == '__main__':
    main()
