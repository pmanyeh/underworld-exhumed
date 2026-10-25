#!/usr/bin/env python3
"""P13 tests: name_gloss.c (built and run, no game assets), and the zh-TW proper-names table:
up to date with the authority and master, one English a name, LF UTF-8."""
import csv
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

UW1 = Path(__file__).resolve().parents[2]
TABLE = UW1 / 'i18n/zh-TW/proper-names.tsv'
sys.path.insert(0, str(Path(__file__).parent))
import proper_names


class TableTests(unittest.TestCase):
    def test_up_to_date(self):
        self.assertEqual(TABLE.read_bytes(), proper_names.table())

    def test_one_english_a_name(self):
        with open(TABLE, encoding='utf-8', newline='') as f:
            rows = list(csv.DictReader(f, delimiter='\t'))
        self.assertTrue(rows)
        self.assertEqual(len({r['zh_TW'] for r in rows}), len(rows))
        self.assertTrue(all(r['source_en'].isascii() and not r['zh_TW'].isascii() for r in rows))

    def test_table_is_lf_utf8(self):
        data = TABLE.read_bytes()
        self.assertNotIn(b'\r', data)
        data.decode('utf-8')


class GlossTests(unittest.TestCase):
    def test_c_module(self):
        cc = os.environ.get('CC') or shutil.which('clang') or shutil.which('cc')
        if not cc:
            self.skipTest('no C compiler')
        out = UW1 / 'build/hd-scroll'
        out.mkdir(parents=True, exist_ok=True)
        exe = out / ('name-gloss' + ('.exe' if os.name == 'nt' else ''))
        subprocess.run([cc, '-std=c11', '-g', '-O1', '-Wall', '-Wextra', '-Werror', '-I' + str(UW1 / 'src/port'),
                        str(Path(__file__).with_name('name_gloss_test.c')), str(UW1 / 'src/port/i18n/name_gloss.c'),
                        '-o', str(exe)], check=True)
        with tempfile.TemporaryDirectory() as scratch:
            run = subprocess.run([str(exe), scratch], capture_output=True, text=True, encoding='utf-8', errors='replace')
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)


if __name__ == '__main__':
    unittest.main()
