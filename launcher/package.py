#!/usr/bin/env python3
"""Builds the Windows release of both games with their front page: a new folder and its ZIP,
never over an old one. The folder holds Underworld.exe (launcher/), uw1port.exe and uw2port.exe
with the DLLs they import, fonts/, licenses/, 說明.txt, MANIFEST.json, and the games' home
folders UW1/ and UW2/ with only their Chinese files (i18n/zh-TW): no settings, so every
enhancement starts off, and no saves.

    python3 launcher/package.py --output DIR --uw1-catalog FILE --uw2-catalog FILE \
        --toolchain MSYS2_CLANG64 --version X.Y

The three programs must be built first (uw1: exhume/tools/portbuild.py; uw2: tools/portbuild.py;
launcher/build.py --version X.Y)."""
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


def imports(objdump, binary, search):
    """Every DLL binary needs, transitively, found in search (Windows's own left out)."""
    found, todo, seen = {}, [binary], set()
    while todo:
        out = subprocess.run([str(objdump), '-p', str(todo.pop())], check=True, capture_output=True,
                             text=True, encoding='utf-8', errors='replace').stdout
        for name in re.findall(r'DLL Name: (\S+)', out):
            if name.lower() in seen:
                continue
            seen.add(name.lower())
            source = next((p / name for p in search if (p / name).is_file()), None)
            if source:
                found[name] = source
                todo.append(source)
            elif not (name.lower().startswith(('api-ms-', 'ext-ms-')) or
                      (Path(os.environ.get('SystemRoot', 'C:/Windows')) / 'System32' / name).is_file()):
                raise RuntimeError('unresolved DLL: ' + name)
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--uw1-catalog', type=Path, required=True)
    parser.add_argument('--uw2-catalog', type=Path, required=True)
    parser.add_argument('--toolchain', type=Path, required=True, help='MSYS2 clang64 directory')
    parser.add_argument('--version', required=True)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    uw1, uw2 = repo / 'uw1', repo / 'uw2'
    out = args.output.resolve()
    archive = out.with_name(out.name + '.zip')
    if out.exists() or archive.exists():
        parser.error('the output folder and its ZIP must both be new; nothing was changed')
    if not re.fullmatch(r'[0-9]+(?:\.[0-9]+)*', args.version):
        parser.error('the version is numbers and dots')
    objdump = args.toolchain / 'bin/llvm-objdump.exe'
    programs = [repo / 'launcher/build/underworld.exe', uw1 / 'build/port/uw1port.exe', uw2 / 'build/port/uw2port.exe']
    catalogs = {'UW1': (args.uw1_catalog.resolve(), 'catalog.uw1loc'), 'UW2': (args.uw2_catalog.resolve(), 'catalog.uw2loc')}
    for required in [objdump, *programs, *(c for c, _ in catalogs.values())]:
        if not required.is_file():
            parser.error('missing input: ' + str(required))
    search = [uw1 / 'tools/libs/bin', args.toolchain / 'bin']
    dlls = {}
    for program in programs:
        dlls.update(imports(objdump, program, search))

    out.mkdir(parents=True)
    shutil.copy2(programs[0], out / 'Underworld.exe')
    for source in [*programs[1:], *dlls.values()]:
        shutil.copy2(source, out / source.name)
    copies = {
        uw1 / 'tools/fonts/jf-openhuninn-2.1.ttf': 'fonts/jf-openhuninn-2.1.ttf',
        repo / 'LICENSE': 'licenses/underworld-exhumed-MIT.txt',
        repo / 'exhume/LICENSE': 'licenses/Exhume-LICENSE.txt',
        uw1 / 'tools/nuked-opl3/LICENSE': 'licenses/Nuked-OPL3-LICENSE.txt',
        uw1 / 'tools/libs/share/doc/munt/libmt32emu/COPYING.txt': 'licenses/mt32emu/COPYING.txt',
        uw1 / 'tools/libs/share/doc/munt/libmt32emu/COPYING.LESSER.txt': 'licenses/mt32emu/COPYING.LESSER.txt',
    }
    for path in (uw1 / 'docs/i18n/licenses').glob('*.txt'):
        copies[path] = 'licenses/' + path.name
    for game, (catalog, name) in catalogs.items():
        copies[catalog] = f'{game}/i18n/zh-TW/{name}'
        src = (uw1 if game == 'UW1' else uw2) / 'i18n/zh-TW'
        for extra in ['input-aliases.tsv', 'proper-names.tsv', 'text-settings.ini']:
            if (src / extra).is_file():
                copies[src / extra] = f'{game}/i18n/zh-TW/{extra}'
    for source, target in copies.items():
        destination = out / target
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
    for name in ['SDL3', 'libc++', 'libiconv', 'libwinpthread', 'freetype']:
        if (args.toolchain / 'share/licenses' / name).is_dir():
            shutil.copytree(args.toolchain / 'share/licenses' / name, out / 'licenses' / name)
    readme = (repo / 'launcher/README.zh-TW.txt').read_text(encoding='utf-8')
    (out / '說明.txt').write_text(readme.replace('@VERSION@', args.version), encoding='utf-8', newline='\r\n')
    (out / 'VERSION.txt').write_text(args.version + '\n', encoding='utf-8')

    git = shutil.which('git') or 'git'
    revision = lambda where: subprocess.check_output([git, '-C', str(where), 'rev-parse', 'HEAD'], text=True).strip()
    manifest = {'version': args.version, 'source_commit': revision(repo), 'exhume_commit': revision(repo / 'exhume'),
                'catalogs': {g: digest(c) for g, (c, _) in catalogs.items()},
                'files': {p.relative_to(out).as_posix(): digest(p) for p in sorted(out.rglob('*')) if p.is_file()}}
    (out / 'MANIFEST.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for path in sorted(out.rglob('*')):
            if path.is_file():
                z.write(path, path.relative_to(out.parent).as_posix())
    print(json.dumps({'folder': str(out), 'zip': str(archive), 'zip_sha256': digest(archive),
                      'files': len(manifest['files']) + 1, 'dlls': sorted(dlls)}, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
