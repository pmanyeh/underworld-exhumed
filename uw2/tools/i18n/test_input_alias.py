"""Phase 14: UW2's i18n/zh-TW/input-aliases.tsv, the Chinese a player may type for a word a
conversation compares (CONVERSE.C's babl_ask, the runtime's i18n/input_alias.c).

    python tools/i18n/test_input_alias.py

Builds input_alias_test.c with the runtime's input_alias.c and runs it on the table. With the
game (UW2_DIR, else the directory of UW2_EXE), also checks each row's source_hash and token
against the player's DATA/STRINGS.PAK, through UW1's l10n.py (the same Huffman format).
"""
import os, shutil, subprocess, sys, tempfile
from pathlib import Path

UW2 = Path(__file__).resolve().parents[2]
RUNTIME = UW2.parent / 'exhume' / 'runtime' / 'port'
TABLE = UW2 / 'i18n' / 'zh-TW' / 'input-aliases.tsv'


def rows():
    lines = TABLE.read_bytes().decode('utf-8').split('\n')
    assert lines[0] == 'slot\tblock\tindex\tsource_hash\ttoken\taliases', lines[0]
    return [l.split('\t') for l in lines[1:] if l]


def check_data():
    data = os.environ.get('UW2_DIR') or (os.path.dirname(os.environ['UW2_EXE']) if os.environ.get('UW2_EXE') else '')
    pak = Path(data) / 'DATA' / 'STRINGS.PAK'
    if not pak.exists():
        print('no game data: source hashes not checked')
        return
    sys.path.insert(0, str(UW2.parent / 'uw1' / 'tools' / 'i18n'))
    import l10n
    inv = l10n.Inventory.load(pak)
    for slot, block, index, digest, token, aliases in rows():
        source = inv.source(int(block), int(index))
        assert source is not None and l10n.source_hash(source) == digest, (block, index)
        assert source.lower() == token, (block, index, source, token)
    print(f'{len(rows())} rows match STRINGS.PAK')


def main():
    seen = {}
    for slot, block, index, digest, token, aliases in rows():
        for alias in filter(None, aliases.split('|')):
            assert seen.setdefault((slot, alias), token) == token, ('an alias for two words', slot, alias)
    check_data()
    cc = os.environ.get('CC') or shutil.which('clang') or 'cc'
    with tempfile.TemporaryDirectory() as tmp:
        exe = Path(tmp) / 'input_alias_test'
        subprocess.run([cc, '-std=c11', '-Wall', '-Wextra', '-Werror', '-I', str(RUNTIME),
                        str(Path(__file__).with_name('input_alias_test.c')), str(RUNTIME / 'i18n' / 'input_alias.c'),
                        '-o', str(exe)], check=True)
        return subprocess.run([str(exe), str(TABLE)]).returncode


if __name__ == '__main__':
    sys.exit(main())
