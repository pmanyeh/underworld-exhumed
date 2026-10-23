#!/usr/bin/env python3
"""Build and run the HD text settings test (P10T); no game assets, SDL or FreeType."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    uw1 = Path(__file__).resolve().parents[2]
    out = uw1/'build/hd-scroll'
    out.mkdir(parents=True, exist_ok=True)
    exe = out/(('text-style-asan' if args.sanitize else 'text-style')+('.exe' if os.name == 'nt' else ''))
    command = [os.environ.get('CC') or shutil.which('clang') or 'cc', '-std=c11', '-g', '-O1',
               '-Wall', '-Wextra', '-Werror', '-I'+str(uw1/'src/port'),
               str(Path(__file__).with_name('text_style_test.c')), str(uw1/'src/port/presentation/text_style.c'),
               '-o', str(exe)]
    if args.sanitize:
        command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    subprocess.run(command, check=True)
    with tempfile.TemporaryDirectory() as scratch:
        run = subprocess.run([str(exe), scratch], capture_output=True, text=True)
    print(run.stdout, end='')
    if run.returncode:
        print(run.stderr, end='')
    run.check_returncode()


if __name__ == '__main__':
    main()
