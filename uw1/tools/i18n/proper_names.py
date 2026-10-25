#!/usr/bin/env python3
"""P13: i18n/zh-TW/proper-names.tsv, the names a conversation or book shows with their English
the first time (src/port/i18n/name_gloss.c). From the authority's people, places and orders
(authority/PROPER-NOUNS.tsv, approved or project-selected) whose Chinese the conversations' or
books' translations use. A Chinese name several English terms share takes the one the game uses
most (the most game keys). The English is the authority's own, already published there.

    python3 tools/i18n/proper_names.py [--check]

--check: exit 1 when the committed table differs from what the authority and master give."""
import argparse
import csv
from pathlib import Path
import sys

UW1 = Path(__file__).resolve().parents[2]
LOCALE = UW1 / 'i18n/zh-TW'
OUT = LOCALE / 'proper-names.tsv'
CATEGORIES = {'person', 'place', 'organization'}
# the master's categories whose text is glossed: the conversations and the books (block 3)
GLOSSED = {'conversation', 'conversation_common', 'books'}
# words too common to gloss: they name nothing a guide needs looking up, and are everywhere
COMMON = {
    'Abyss',            # 深淵, the game's word for the whole dungeon
    'Academy',          # 學院, any academy
    'Seers',            # 預言者, a common noun; Seers of the Moonstone is kept
    'Level 7',          # 第七層, a level's number
}


def rows(path):
    with open(path, encoding='utf-8', newline='') as f:
        return list(csv.DictReader(f, delimiter='\t'))


def table():
    text = '\n'.join(r['target_zh_TW'] for r in rows(LOCALE / 'master.tsv') if r['category'] in GLOSSED)
    best = {}
    for t in rows(LOCALE / 'authority/PROPER-NOUNS.tsv'):
        zh, en = t['canonical_zh_tw'], t['source_en']
        if (t['status'] not in ('approved', 'project-selected') or t['category'] not in CATEGORIES
                or en in COMMON or not zh or zh.isascii() or zh not in text):
            continue
        uses = len([k for k in t['game_keys'].split(';') if k])
        if zh not in best or uses > best[zh][1]:
            best[zh] = (en, uses)
    lines = ['zh_TW\tsource_en'] + [f'{zh}\t{best[zh][0]}' for zh in sorted(best)]
    return ('\n'.join(lines) + '\n').encode('utf-8')


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--check', action='store_true')
    a = ap.parse_args(argv)
    data = table()
    if a.check:
        same = OUT.exists() and OUT.read_bytes() == data
        print('proper-names.tsv: ' + ('up to date' if same else 'differs; run tools/i18n/proper_names.py'))
        return 0 if same else 1
    OUT.write_bytes(data)
    print(f'{OUT.relative_to(UW1)}: {data.count(b"\n") - 1} names')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
