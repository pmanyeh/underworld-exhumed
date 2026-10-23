#!/usr/bin/env python3
"""P11.4 tests: input_alias.c (built and run, no game assets), and the zh-TW table against
input-keys.tsv: one row per compared string, the same hash, every word the player can only know
in Chinese given at least one Chinese alias, and no alias claimed by two words of a conversation."""
import collections
import csv
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

UW1 = Path(__file__).resolve().parents[2]
TABLE = UW1 / 'i18n/zh-TW/input-aliases.tsv'
KEYS = UW1 / 'i18n/input-keys.tsv'
# typed as the game shows them: the Lizardman words the prisoner teaches, and Gurstang's code word
AS_SHOWN = {24, 187}


def rows(path):
    with open(path, encoding='utf-8', newline='') as f:
        return list(csv.DictReader(f, delimiter='\t'))


class TableTests(unittest.TestCase):
    def test_every_compared_string_has_a_row(self):
        keys = {(r['block'], r['index']): r['source_hash'] for r in rows(KEYS)}
        table = {(r['block'], r['index']): r['source_hash'] for r in rows(TABLE)}
        self.assertEqual(table, keys)

    def test_words_known_only_in_chinese_have_an_alias(self):
        missing = [f"{r['slot']}:{r['token']}" for r in rows(TABLE) if not r['aliases'] and int(r['slot']) not in AS_SHOWN]
        self.assertEqual(missing, [])

    def test_an_alias_means_one_word_per_conversation(self):
        owners = collections.defaultdict(set)
        for r in rows(TABLE):
            for alias in filter(None, r['aliases'].split('|')):
                owners[r['slot'], alias].add(r['token'].lower())
        self.assertEqual({k: v for k, v in owners.items() if len(v) > 1}, {})

    def test_table_is_lf_utf8(self):
        data = TABLE.read_bytes()
        self.assertNotIn(b'\r', data)
        data.decode('utf-8')


class MapTests(unittest.TestCase):
    def test_c_module(self):
        cc = os.environ.get('CC') or shutil.which('clang') or shutil.which('cc')
        if not cc:
            self.skipTest('no C compiler')
        out = UW1 / 'build/hd-scroll'
        out.mkdir(parents=True, exist_ok=True)
        exe = out / ('input-alias' + ('.exe' if os.name == 'nt' else ''))
        subprocess.run([cc, '-std=c11', '-g', '-O1', '-Wall', '-Wextra', '-Werror', '-I' + str(UW1 / 'src/port'),
                        str(Path(__file__).with_name('input_alias_test.c')), str(UW1 / 'src/port/i18n/input_alias.c'),
                        '-o', str(exe)], check=True)
        with tempfile.TemporaryDirectory() as scratch:
            run = subprocess.run([str(exe), scratch], capture_output=True, text=True, encoding='utf-8', errors='replace')
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)


if __name__ == '__main__':
    unittest.main()
