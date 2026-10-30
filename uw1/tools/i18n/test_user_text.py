#!/usr/bin/env python3
"""The player's words in Chinese (user_text.c): stand-ins, side files and what is shown; no assets."""
from portsrc import RUNTIME, port_src
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

UW1 = Path(__file__).resolve().parents[2]


class UserTextTests(unittest.TestCase):
    def test_user_text(self):
        cc = os.environ.get('CC') or shutil.which('clang') or shutil.which('cc')
        if not cc:
            self.skipTest('no C compiler')
        out = UW1 / 'build/hd-scroll'
        out.mkdir(parents=True, exist_ok=True)
        exe = out / ('user_text' + ('.exe' if os.name == 'nt' else ''))
        subprocess.run([cc, '-std=c11', '-g', '-O1', '-Wall', '-Wextra', '-Werror', '-I' + str(UW1 / 'src/port'), "-I" + str(RUNTIME),
                        str(Path(__file__).with_name('user_text_test.c')),
                        str(port_src('i18n/user_text.c')), str(port_src('i18n/utf8.c')),
                        '-o', str(exe)], check=True)
        with tempfile.TemporaryDirectory() as d:
            run = subprocess.run([str(exe), d], capture_output=True, text=True)
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)


if __name__ == '__main__':
    unittest.main()
