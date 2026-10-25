"""Combat ownership, display state and draw clipping; no original assets or SDL required."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    uw1 = Path(__file__).resolve().parents[2]
    out = uw1 / 'build/combat-text'
    out.mkdir(parents=True, exist_ok=True)
    exe = out / ('combat' + ('-asan' if args.sanitize else '') + ('.exe' if os.name == 'nt' else ''))
    cc = os.environ.get('CC') or shutil.which('clang') or 'cc'
    command = [cc, '-std=c11', '-g', '-O1', '-Wall', '-Wextra', '-Werror',
               '-I' + str(uw1 / 'src/port'), '-I' + str(uw1.parent / 'exhume/runtime/port/platform'),
               str(Path(__file__).with_name('combat_text_test.c')),
               *[str(uw1 / 'src/port' / s) for s in ['presentation/combat_text.c', 'presentation/combat_text_draw.c', 'i18n/utf8.c']],
               '-o', str(exe)]
    if os.name != 'nt':
        command += ['-lm']
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    subprocess.run(command, check=True)
    env = dict(os.environ, ASAN_OPTIONS='halt_on_error=1:detect_leaks=' + ('0' if os.name == 'nt' else '1'),
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    subprocess.run([str(exe)], check=True, env=env)

if __name__ == '__main__':
    main()
