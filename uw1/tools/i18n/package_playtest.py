#!/usr/bin/env python3
"""Build a fresh private Windows Chinese playtest folder and ZIP; never overwrite a folder."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import zipfile


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--catalog', type=Path, required=True)
    parser.add_argument('--toolchain', type=Path, required=True, help='MSYS2 clang64 directory')
    parser.add_argument('--version', default='0.5')
    args = parser.parse_args()
    uw1 = Path(__file__).resolve().parents[2]
    repo = uw1.parent
    out = args.output.resolve()
    archive = out.with_name(out.name + '.zip')
    if out.exists() or archive.exists():
        parser.error('output folder and ZIP must both be new; nothing was changed')
    if not re.fullmatch(r'[0-9]+(?:\.[0-9]+)*', args.version):
        parser.error('version must contain only numbers and dots')
    catalog = args.catalog.resolve()
    objdump = args.toolchain / 'bin/llvm-objdump.exe'
    binary = uw1 / 'build/port/uw1port.exe'
    for required in [catalog, objdump, binary]:
        if not required.is_file():
            parser.error('missing input: ' + str(required))
    # Resolve every import before creating the output. Windows DLLs stay in Windows.
    dependencies = {}
    todo = [binary]
    seen = set()
    while todo:
        path = todo.pop()
        output = subprocess.run([str(objdump), '-p', str(path)], check=True, capture_output=True,
                                text=True, encoding='utf-8', errors='replace').stdout
        for name in re.findall(r'DLL Name: (\S+)', output):
            key = name.lower()
            if key in seen:
                continue
            seen.add(key)
            source = next((p / name for p in [uw1 / 'tools/libs/bin', args.toolchain / 'bin']
                           if (p / name).is_file()), None)
            if source:
                dependencies[name] = source
                todo.append(source)
            elif not (key.startswith(('api-ms-', 'ext-ms-')) or
                      (Path(os.environ.get('SystemRoot', 'C:/Windows')) / 'System32' / name).is_file()):
                raise RuntimeError('unresolved DLL: ' + name)
    out.mkdir(parents=True)
    for source in [binary, *dependencies.values()]:
        shutil.copy2(source, out / source.name)
    copies = {
        uw1 / 'tools/fonts/jf-openhuninn-2.1.ttf': 'fonts/jf-openhuninn-2.1.ttf',
        catalog: 'i18n/zh-TW/catalog.uw1loc',
        repo / 'LICENSE': 'licenses/underworld-exhumed-MIT.txt',
        repo / 'exhume/LICENSE': 'licenses/Exhume-LICENSE.txt',
        uw1 / 'tools/nuked-opl3/LICENSE': 'licenses/Nuked-OPL3-LICENSE.txt',
        uw1 / 'tools/libs/share/doc/munt/libmt32emu/COPYING.txt': 'licenses/mt32emu/COPYING.txt',
        uw1 / 'tools/libs/share/doc/munt/libmt32emu/COPYING.LESSER.txt': 'licenses/mt32emu/COPYING.LESSER.txt',
    }
    for name in ['text-settings.ini', 'input-aliases.tsv', 'proper-names.tsv', 'SETTINGS.zh-TW.md']:
        copies[uw1 / 'i18n/zh-TW' / name] = 'i18n/zh-TW/' + name
    for path in (uw1 / 'docs/i18n/licenses').glob('*.txt'):
        copies[path] = 'licenses/' + path.name
    for source, target in copies.items():
        destination = out / target
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
    for name in ['SDL3', 'libc++', 'libiconv', 'libwinpthread']:
        shutil.copytree(args.toolchain / 'share/licenses' / name, out / 'licenses' / name)
    (out / 'VERSION.txt').write_text(args.version + '\n', encoding='utf-8')
    readme = (uw1 / 'docs/i18n/PLAYTEST_README.txt').read_text(encoding='utf-8')
    (out / '說明.txt').write_text(readme.replace('@VERSION@', args.version), encoding='utf-8', newline='\r\n')
    # Relative data only; the built-in GOG detector/folder picker handles all other installs.
    launcher = '@echo off\nchcp 65001 >nul\ncd /d "%~dp0"\n'
    launcher += 'if exist "%~dp0UW\\UW.EXE" (\n'
    launcher += '  uw1port.exe --data "%~dp0UW" --home "%~dp0home" %*\n'
    launcher += ') else (\n  uw1port.exe --home "%~dp0home" %*\n)\n'
    (out / '開始遊戲.bat').write_bytes(launcher.replace('\n', '\r\n').encode('utf-8'))
    git = shutil.which('git') or 'git'
    revision = lambda where: subprocess.check_output([git, '-C', str(where), 'rev-parse', 'HEAD'], text=True).strip()
    manifest = {'version': args.version, 'source_commit': revision(repo), 'exhume_commit': revision(repo / 'exhume'),
                'catalog_sha256': digest(catalog), 'files': {
                    p.relative_to(out).as_posix(): digest(p) for p in sorted(out.rglob('*')) if p.is_file()}}
    (out / 'MANIFEST.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for path in sorted(out.rglob('*')):
            if path.is_file():
                z.write(path, path.relative_to(out.parent).as_posix())
    print(json.dumps({'folder': str(out), 'zip': str(archive), 'zip_sha256': digest(archive),
                      'files': len(manifest['files']) + 1}, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
