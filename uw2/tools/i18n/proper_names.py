#!/usr/bin/env python3
"""Phase 14: UW2's i18n/zh-TW/proper-names.tsv, the names a conversation or book shows with their
English the first time (the runtime's i18n/name_gloss.c), as UW1's tools/i18n/proper_names.py
makes UW1's. From three sources, each name kept only when the translations of UW2's
conversations or books use its Chinese:

  - UW1's authority (uw1/i18n/zh-TW/authority/PROPER-NOUNS.tsv): its people, places and
    orders, approved or project-selected (the two games share Britannia's);
  - UW2's own names (i18n/zh-TW/proper-nouns.tsv, from the translation's glossary);
  - with the game, the people the conversations name (STRINGS.PAK block 7, from 16) that start
    with a capital, by their translation's Chinese (the label's own （English） left out).

A Chinese name two English names share keeps the first found, in that order.

    python3 tools/i18n/proper_names.py --master MASTER.tsv [--pak DATA/STRINGS.PAK] [--check]

The master (the translation) is not in the repository; --check exits 1 when the committed
table differs from what the sources give."""
import argparse
import csv
from pathlib import Path
import sys

UW2 = Path(__file__).resolve().parents[2]
LOCALE = UW2 / 'i18n/zh-TW'
OUT = LOCALE / 'proper-names.tsv'
AUTHORITY = UW2.parent / 'uw1/i18n/zh-TW/authority/PROPER-NOUNS.tsv'
CATEGORIES = {'person', 'place', 'organization'}
GLOSSED = {'conversation', 'conversation_common', 'books'}
# words too common to gloss, as UW1's list
COMMON = {'Abyss', 'Academy', 'Seers', 'Level 7'}


def rows(path):
    with open(path, encoding='utf-8', newline='') as f:
        return list(csv.DictReader(f, delimiter='\t', quoting=csv.QUOTE_NONE))


def table(master, pak):
    targets = {(int(r['block']), int(r['index'])): r['target_zh_TW'] for r in rows(master)}
    # block 7 is the name labels, which carry their English already ("賈納（Janar）")
    text = '\n'.join(r['target_zh_TW'] for r in rows(master) if r['category'] in GLOSSED and r['block'] != '7')
    names = {}

    def add(zh, en):
        zh = zh.split('（')[0].strip()      # a label's Chinese, without its （English）
        if (zh and not zh.isascii() and en not in COMMON and not en.startswith('Level ')
                and zh in text and zh not in names):
            names[zh] = en

    for t in rows(AUTHORITY):
        if t['status'] in ('approved', 'project-selected') and t['category'] in CATEGORIES:
            add(t['canonical_zh_tw'], t['source_en'])
    for t in rows(LOCALE / 'proper-nouns.tsv'):
        add(t['zh_TW'], t['source_en'])
    if pak:
        sys.path.insert(0, str(UW2.parent / 'uw1/tools/i18n'))
        import l10n
        inv = l10n.Inventory.load(pak)
        for (block, index), en in sorted(inv.entries.items()):
            if block == 7 and index >= 16 and en[:1].isupper():
                add(targets.get((7, index), ''), en)
    lines = ['zh_TW\tsource_en'] + [f'{zh}\t{names[zh]}' for zh in sorted(names)]
    return ('\n'.join(lines) + '\n').encode('utf-8')


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--master', type=Path, required=True)
    ap.add_argument('--pak', type=Path)
    ap.add_argument('--check', action='store_true')
    a = ap.parse_args(argv)
    data = table(a.master, a.pak)
    if a.check:
        same = OUT.exists() and OUT.read_bytes() == data
        print('proper-names.tsv: ' + ('up to date' if same else 'differs; run tools/i18n/proper_names.py'))
        return 0 if same else 1
    OUT.write_bytes(data)
    print(f'{OUT.relative_to(UW2)}: {data.count(b"\n") - 1} names')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
