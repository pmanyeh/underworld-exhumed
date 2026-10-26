#!/usr/bin/env python3
"""HD labels (hd_label.c): layout, alignment, signatures and bounds with a fake screen; no assets."""
import os
from pathlib import Path
import shutil
import subprocess
import unittest

UW1 = Path(__file__).resolve().parents[2]


class HdLabelTests(unittest.TestCase):
    def test_statistics_ascii_scope(self):
        cc = os.environ.get('CC') or shutil.which('clang') or shutil.which('cc')
        if not cc:
            self.skipTest('no C compiler')
        out = UW1 / 'build/hd-scroll'
        out.mkdir(parents=True, exist_ok=True)
        source = (UW1 / 'src/port/i18n/ui_text.c').read_text(encoding='utf-8')
        stats = (UW1 / 'src/ui/STATS.C').read_text(encoding='utf-8')
        signatures = ['static const char *ordinal(', 'const char *port_ui_text(']
        functions = [sig + source.split(sig, 1)[1].split('\n}', 1)[0] + '\n}\n'
                     for sig in signatures]
        state = source.split('static char next_english', 1)[1].split('\nvoid port_ui_next', 1)[0]
        wrapper = 'static void stats_string(' + stats.split('static void stats_string(', 1)[1].split('\n}', 1)[0] + '\n}\n'
        (out / 'stats_ascii_impl.h').write_text('static char next_english' + state + '\n' + '\n'.join(functions) + wrapper,
                                               encoding='utf-8')
        exe = out / ('stats_ascii' + ('.exe' if os.name == 'nt' else ''))
        subprocess.run([cc, '-std=c11', '-Wall', '-Wextra', '-Werror', '-I' + str(out),
                        '-I' + str(UW1 / 'src/port'), str(Path(__file__).with_name('stats_ascii_test.c')),
                        str(UW1 / 'src/port/presentation/hd_label.c'), str(UW1 / 'src/port/i18n/utf8.c'),
                        '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)

    def test_layout(self):
        cc = os.environ.get('CC') or shutil.which('clang') or shutil.which('cc')
        if not cc:
            self.skipTest('no C compiler')
        out = UW1 / 'build/hd-scroll'
        out.mkdir(parents=True, exist_ok=True)
        exe = out / ('hd_label' + ('.exe' if os.name == 'nt' else ''))
        subprocess.run([cc, '-std=c11', '-g', '-O1', '-Wall', '-Wextra', '-Werror', '-I' + str(UW1 / 'src/port'),
                        str(Path(__file__).with_name('hd_label_test.c')),
                        str(UW1 / 'src/port/presentation/hd_label.c'), str(UW1 / 'src/port/i18n/utf8.c'),
                        '-o', str(exe)], check=True)
        run = subprocess.run([str(exe)], capture_output=True, text=True)
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)


if __name__ == '__main__':
    unittest.main()
