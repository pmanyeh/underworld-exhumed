#!/usr/bin/env python3
"""Builds the macOS release of both games with their front page, on a Mac (the release
workflow's macOS job): one app, Underworld.app, holding the three programs (Contents/MacOS:
underworld, uw1port, uw2port; universal when they are), the libraries they load in
Contents/Frameworks with @rpath install names, and UW1's icon; signed ad hoc from the inside
out, so that macOS asks the player once, for the one app. Beside it, as in the Windows release:
開始遊戲.command (it starts the front page with the release's folder, which a downloaded app,
moved by macOS to a read-only place to run, would not know), the three 說明 files, licenses/,
and the games' home folders UW1/ and UW2/ with their Chinese files but the catalogs.

The catalogs (built from the player's game data, so never in CI) and the fonts (not in git) are
added afterwards by finish_release.py, which keeps every byte of this ZIP.

    python3 launcher/package_macos.py --version 1.0 --uw1 PROG --uw2 PROG --launcher PROG \
        --libs DIR [--out DIR]
"""
import argparse, os, plistlib, shutil, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)


def run(cmd):
    return subprocess.run(cmd, capture_output=True, text=True, check=True).stdout


def deps(path):
    lines = run(['otool', '-L', path]).splitlines()
    return list(dict.fromkeys(l.strip().split(' (')[0] for l in lines if l.strip() and not l.rstrip().endswith(':')))


def rpaths(path):
    out, lines = [], run(['otool', '-l', path]).splitlines()
    for i, l in enumerate(lines):
        if 'cmd LC_RPATH' in l:
            out.append(lines[i + 2].strip().split()[1])
    return list(dict.fromkeys(out))


def resolve(dep, search):
    if dep.startswith('@rpath/'):
        name = dep[len('@rpath/'):]
        return next((os.path.join(d, name) for d in search if os.path.exists(os.path.join(d, name))), None)
    return dep if os.path.exists(dep) else None


def text(name, version):
    with open(os.path.join(HERE, name), encoding='utf-8') as f:
        return f.read().replace('@VERSION@', version)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--version', required=True)
    p.add_argument('--uw1', required=True)
    p.add_argument('--uw2', required=True)
    p.add_argument('--launcher', required=True)
    p.add_argument('--libs', required=True, help='where the libraries were built (exhume/tools/setup-libs.sh)')
    p.add_argument('--out', default=os.path.join(HERE, 'build', 'dist'))
    a = p.parse_args()
    name = f'Underworld-zhTW-{a.version}-macOS'
    stage = os.path.join(a.out, name)
    shutil.rmtree(stage, ignore_errors=True)
    app = os.path.join(stage, 'Underworld.app')
    contents = os.path.join(app, 'Contents')
    macos, fw, res = (os.path.join(contents, d) for d in ('MacOS', 'Frameworks', 'Resources'))
    for d in (macos, fw, res):
        os.makedirs(d)
    exes = []
    for src, exe in ((a.launcher, 'underworld'), (a.uw1, 'uw1port'), (a.uw2, 'uw2port')):
        shutil.copy(src, os.path.join(macos, exe))
        os.chmod(os.path.join(macos, exe), 0o755)
        exes.append(os.path.join(macos, exe))
    with open(os.path.join(contents, 'Info.plist'), 'wb') as f:
        f.write(plistlib.dumps({
            'CFBundleDevelopmentRegion': 'zh_TW', 'CFBundleDisplayName': '地下創世紀', 'CFBundleExecutable': 'underworld',
            'CFBundleIconFile': 'uw1', 'CFBundleIdentifier': 'io.github.pmanyeh.underworld-zhtw',
            'CFBundleInfoDictionaryVersion': '6.0', 'CFBundleName': 'Underworld', 'CFBundlePackageType': 'APPL',
            'CFBundleShortVersionString': a.version, 'CFBundleVersion': a.version,
            'LSApplicationCategoryType': 'public.app-category.role-playing-games', 'LSMinimumSystemVersion': '11.0',
            'NSHighResolutionCapable': True}))
    with open(os.path.join(contents, 'PkgInfo'), 'w') as f:
        f.write('APPL????')
    shutil.copy(os.path.join(REPO, 'uw1', 'tools', 'dist', 'icon', 'uw1.icns'), os.path.join(res, 'uw1.icns'))

    # every library outside the system's that any of the three loads, followed through the
    # libraries' own, copied once into Frameworks
    search = [os.path.join(a.libs, 'lib')] + [os.path.dirname(e) for e in (a.uw1, a.uw2, a.launcher)]
    for e in exes:
        search += [r for r in rpaths(e) if not r.startswith('@')]
    todo, copied = list(exes), {}
    while todo:
        f = todo.pop()
        for dep in deps(f):
            base = os.path.basename(dep)
            if dep.startswith(('/usr/lib/', '/System/')) or base in copied or (f not in exes and base == os.path.basename(f)):
                continue
            src = resolve(dep, search)
            if not src:
                sys.exit(f'package_macos.py: cannot find {dep}, needed by {f}')
            dst = os.path.join(fw, base)
            shutil.copy(os.path.realpath(src), dst)
            os.chmod(dst, 0o755)
            copied[base] = dep
            todo.append(dst)
    libs = [os.path.join(fw, n) for n in copied]
    for f in exes + libs:
        for dep in deps(f):
            base = os.path.basename(dep)
            if base in copied and dep != '@rpath/' + base and not (f in libs and base == os.path.basename(f)):
                run(['install_name_tool', '-change', dep, '@rpath/' + base, f])
        if f in libs:
            run(['install_name_tool', '-id', '@rpath/' + os.path.basename(f), f])
    for e in exes:
        for rp in rpaths(e):
            if not rp.startswith('@'):
                run(['install_name_tool', '-delete_rpath', rp, e])
        if '@executable_path/../Frameworks' not in rpaths(e):
            run(['install_name_tool', '-add_rpath', '@executable_path/../Frameworks', e])
    for f in libs + exes + [app]:                   # inside out; the app seals the rest
        run(['codesign', '--force', '--sign', '-', f])
    for f in exes + libs:
        print(f'{os.path.relpath(f, stage)}: {run(["lipo", "-archs", f]).strip()}')

    command = os.path.join(stage, '開始遊戲.command')
    with open(command, 'w', encoding='utf-8', newline='\n') as f:
        f.write('#!/bin/sh\n'
                '# the front page, given this folder: the saves, settings, fonts and Chinese files are here\n'
                'cd "$(dirname "$0")" || exit 1\n'
                'exec ./Underworld.app/Contents/MacOS/underworld --base "$PWD"\n')
    os.chmod(command, 0o755)
    for src, dst in (('README.zh-TW.txt', '說明.txt'), ('SETTINGS.zh-TW.txt', '說明_設定畫面.txt'),
                     ('MT32.zh-TW.txt', '說明_MT-32音樂.txt')):
        with open(os.path.join(stage, dst), 'w', encoding='utf-8') as f:
            f.write(text(src, a.version))
    for game in ('UW1', 'UW2'):
        src = os.path.join(REPO, game.lower(), 'i18n', 'zh-TW')
        dst = os.path.join(stage, game, 'i18n', 'zh-TW')
        os.makedirs(dst)
        for extra in ('input-aliases.tsv', 'proper-names.tsv', 'text-settings.ini'):
            if os.path.exists(os.path.join(src, extra)):
                shutil.copy(os.path.join(src, extra), dst)
    lic = os.path.join(stage, 'licenses')
    os.makedirs(lic)
    for src, dst in ((os.path.join(REPO, 'LICENSE'), 'underworld-exhumed-MIT.txt'),
                     (os.path.join(REPO, 'exhume', 'LICENSE'), 'Exhume-LICENSE.txt'),
                     (os.path.join(a.libs, 'share', 'licenses', 'SDL3', 'LICENSE.txt'), 'SDL3-LICENSE.txt'),
                     (os.path.join(a.libs, 'share', 'doc', 'munt', 'libmt32emu', 'COPYING.LESSER.txt'), 'mt32emu-COPYING.LESSER.txt'),
                     (os.path.join(a.libs, 'share', 'doc', 'munt', 'libmt32emu', 'COPYING.txt'), 'mt32emu-COPYING.txt')):
        if os.path.exists(src):
            shutil.copy(src, os.path.join(lic, dst))
    for path in os.listdir(os.path.join(REPO, 'uw1', 'docs', 'i18n', 'licenses')):
        shutil.copy(os.path.join(REPO, 'uw1', 'docs', 'i18n', 'licenses', path), lic)
    nuked = os.path.join(REPO, 'uw1', 'tools', 'nuked-opl3', 'LICENSE')
    if os.path.exists(nuked):
        shutil.copy(nuked, os.path.join(lic, 'Nuked-OPL3-LICENSE.txt'))

    zpath = os.path.join(a.out, name + '.zip')
    if os.path.exists(zpath):
        os.remove(zpath)
    run(['ditto', '-c', '-k', '--norsrc', '--noextattr', '--keepParent', stage, zpath])
    print(zpath)


if __name__ == '__main__':
    main()
