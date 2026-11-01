"""Builds the release's front page (launcher.c) for the host: launcher/build/underworld, or
underworld.exe on Windows with UW1's icon. It needs SDL3 and FreeType through pkg-config, as the
ports do (uw1/tools/libs searched first), and the C compiler in CC (default clang).

    python3 launcher/build.py [--version TEXT] [--arch ARCH ...]
"""
import os, shlex, shutil, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
RT = os.path.join(REPO, 'exhume', 'runtime', 'port')
OUT = os.path.join(HERE, 'build')
SOURCES = [os.path.join(HERE, 'launcher.c'), os.path.join(RT, 'presentation', 'hd_font.c'),
           os.path.join(RT, 'i18n', 'layout.c'), os.path.join(RT, 'i18n', 'utf8.c')]


def pkg(*args):
    env = dict(os.environ)
    pc = os.path.join(REPO, 'uw1', 'tools', 'libs', 'lib', 'pkgconfig')
    if os.path.isdir(pc):
        env['PKG_CONFIG_PATH'] = pc + (os.pathsep + env['PKG_CONFIG_PATH'] if env.get('PKG_CONFIG_PATH') else '')
    r = subprocess.run(['pkg-config', *args, 'sdl3', 'freetype2'], capture_output=True, text=True, env=env)
    if r.returncode:
        sys.exit('launcher/build.py: pkg-config cannot find sdl3 and freetype2')
    return shlex.split(r.stdout)        # a path with a space comes escaped


def windows(cc):
    m = subprocess.run([cc, '-dumpmachine'], capture_output=True, text=True).stdout.strip()
    return 'mingw' in m or 'windows' in m, m


def resource(machine):
    """UW1's icon as a resource object, or [] without a resource compiler."""
    tool = shutil.which('llvm-windres') or shutil.which('windres')
    if not tool:
        print('launcher/build.py: no windres; the program gets no icon')
        return []
    rc, obj = os.path.join(OUT, 'underworld.rc'), os.path.join(OUT, 'underworld-res.o')
    ico = os.path.join(REPO, 'uw1', 'tools', 'dist', 'icon', 'uw1.ico').replace('\\', '/')
    with open(rc, 'w') as f:
        f.write(f'1 ICON "{ico}"\n')
    cmd = [tool, '-O', 'coff', '-i', rc, '-o', obj]
    if 'llvm' in os.path.basename(tool) and machine:
        cmd.append('--target=' + machine)
    subprocess.run(cmd, check=True)
    return [obj]


def main():
    version, archs = 'dev', []
    args = sys.argv[1:]
    while args:
        a = args.pop(0)
        if a == '--version': version = args.pop(0)
        elif a == '--arch': archs += ['-arch', args.pop(0)]     # macOS: --arch arm64 --arch x86_64, one universal program
        else: sys.exit(f'launcher/build.py: unknown argument {a}')
    cc = os.environ.get('CC', 'clang')
    os.makedirs(OUT, exist_ok=True)
    win, machine = windows(cc)
    out = os.path.join(OUT, 'underworld.exe' if win else 'underworld')
    cmd = [cc, *archs, '-O2', '-std=gnu11', '-Wall', '-Wextra', '-Wno-unused-parameter',
           f'-DLAUNCHER_VERSION="{version}"', '-DHD_HAVE_FREETYPE', '-I', HERE, '-I', RT,
           '-I', os.path.join(RT, 'platform'),
           *pkg('--cflags'), *SOURCES, '-o', out, *pkg('--libs'), '-lm']
    if win:
        cmd += resource(machine) + ['-mwindows']
    print(' '.join(cmd))
    subprocess.run(cmd, check=True)
    print(f'launcher/build.py: built {os.path.relpath(out, REPO)}')


if __name__ == '__main__':
    main()
