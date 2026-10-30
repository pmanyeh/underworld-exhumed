#!/usr/bin/env python3
"""Cutscene subtitles (caption.c): layout built and run with a fixed-width measure; no assets."""
from portsrc import RUNTIME, port_src
import os
from pathlib import Path
import shutil
import subprocess
import unittest

UW1 = Path(__file__).resolve().parents[2]


class CaptionTests(unittest.TestCase):
    def test_layout(self):
        cc = os.environ.get('CC') or shutil.which('clang') or shutil.which('cc')
        if not cc:
            self.skipTest('no C compiler')
        out = UW1 / 'build/hd-scroll'
        out.mkdir(parents=True, exist_ok=True)
        exe = out / ('caption' + ('.exe' if os.name == 'nt' else ''))
        subprocess.run([cc, '-std=c11', '-g', '-O1', '-Wall', '-Wextra', '-Werror', '-I' + str(UW1 / 'src/port'), "-I" + str(RUNTIME),
                        str(Path(__file__).with_name('caption_test.c')),
                        str(port_src('presentation/caption.c')), str(port_src('i18n/utf8.c')),
                        '-o', str(exe)], check=True)
        run = subprocess.run([str(exe)], capture_output=True, text=True)
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)


if __name__ == '__main__':
    unittest.main()
