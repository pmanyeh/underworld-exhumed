#!/usr/bin/env python3
"""Create the owner's private development transfer ZIP, with Git bundles and selected local data."""
import argparse
import hashlib
import importlib.metadata
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile


def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workspace', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    ws = args.workspace.resolve()
    output = args.output.resolve()
    if output.exists():
        parser.error('output must be new; existing archives are never overwritten')
    repo = ws / 'underworld-exhumed'
    uw1 = repo / 'uw1'
    git = shutil.which('git') or 'git'
    run = lambda directory, *cmd: subprocess.check_output([git, '-C', str(directory), *cmd], text=True).strip()
    for directory in [repo, repo / 'exhume']:
        if run(directory, 'status', '--porcelain'):
            parser.error('commit changes first: ' + str(directory))
    files = {}

    def add(path):
        if path.is_file():
            files['workspace/' + path.relative_to(ws).as_posix()] = path

    def tree(path):
        for base, dirs, names in os.walk(path, followlinks=False):
            dirs[:] = [n for n in dirs if n != '__pycache__' and not (Path(base) / n).is_symlink()
                       and not (hasattr(Path(base) / n, 'is_junction') and (Path(base) / n).is_junction())]
            for name in names:
                add(Path(base) / name)

    for relative in ['.poc-private', '.poc-evidence', '.poc-tools/handoff', 'translation', 'Game Resources',
                     'UW1-zhTW-playtest/home', 'underworld-exhumed/uw1/i18n/zh-TW/private-authority',
                     'underworld-exhumed/uw1/tools/fonts', 'underworld-exhumed/uw1/tools/libs',
                     'underworld-exhumed/uw1/tools/nuked-opl3']:
        tree(ws / relative)
    for path in (ws / '.poc-tools').glob('*.py'):
        add(path)
    for path in ws.glob('*.md'):
        add(path)
    add(ws / 'Start-UW1-PoC.ps1')
    for path in (uw1 / 'build/poc').glob('*.toml'):
        add(path)
    # Keep authored translations and small evidence, not the many GB of framebuffer dumps.
    reports = {'report.json', 'delivery.json', 'visual.json', 'visual-debug.json', 'record.json',
               'record-debug.json', 'save-equivalence.json', 'package-verification.json', 'restore-verification.json'}
    for base, dirs, names in os.walk(uw1 / 'build/i18n'):
        # Temporary clones and portable smoke copies are not another source of history.
        dirs[:] = [name for name in dirs if name != 'workspace' and not name.startswith(('restore-', 'portable-'))]
        for name in names:
            path = Path(base) / name
            if name == 'translations.py' or name in reports and path.stat().st_size < 4 * 1024 * 1024:
                add(path)
    for path in (uw1 / 'build/i18n/p13-goblin').iterdir():
        if path.is_file():
            add(path)
    for base, dirs, names in os.walk(uw1 / 'build/replay/verify-en-p6'):
        for name in list(dirs):
            if name.upper().startswith('SAVE'):
                tree(Path(base) / name)
                dirs.remove(name)
    for relative in ['build/port/uw1port.exe', 'build/port-debug/uw1port.exe']:
        add(uw1 / relative)
    requirements = '\n'.join(f'{name}=={importlib.metadata.version(name)}'
                             for name in ['fonttools', 'Pillow', 'iced-x86', 'unicorn']) + '\n'
    (ws / '.poc-tools/handoff/requirements.txt').write_text(requirements, encoding='utf-8')
    add(ws / '.poc-tools/handoff/requirements.txt')
    manifest = {'version': '0.5', 'private': True, 'source_commit': run(repo, 'rev-parse', 'HEAD'),
                'exhume_commit': run(repo / 'exhume', 'rev-parse', 'HEAD'), 'files': {}}
    with tempfile.TemporaryDirectory(prefix='uw1-handoff-') as temporary:
        for name, directory in [('underworld-exhumed', repo), ('exhume', repo / 'exhume')]:
            bundle = Path(temporary) / (name + '.bundle')
            subprocess.run([git, '-C', str(directory), 'bundle', 'create', str(bundle), '--all'], check=True)
            subprocess.run([git, '-C', str(directory), 'bundle', 'verify', str(bundle)], check=True,
                           stdout=subprocess.DEVNULL)
            files['bundles/' + bundle.name] = bundle
        files['移機說明.md'] = uw1 / 'docs/i18n/MIGRATION_0.5.md'
        with zipfile.ZipFile(output, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as z:
            for name, path in sorted(files.items()):
                manifest['files'][name] = sha(path)
                z.write(path, name)
            z.writestr('MANIFEST.json', json.dumps(manifest, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps({'archive': str(output), 'sha256': sha(output), 'files': len(files) + 1,
                      'size_bytes': output.stat().st_size}, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
