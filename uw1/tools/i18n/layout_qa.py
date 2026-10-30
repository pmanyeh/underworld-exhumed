#!/usr/bin/env python3
"""Layout QA for translated conversation rows (P10B): lay each target out with the HD
scroll's own code and the default font, in the box the game shows it in, and report lines,
[MORE] pages and failures. A player's menu option, and any string outside a conversation
(game messages, books, writings, trap text), goes to the message scroll (box 0, three lines);
an NPC's line to the conversation's box (1, nine lines); the role comes from babl_audit. Needs the player's STRINGS.PAK/CNV.ARK for the roles; reports keys and counts."""
from portsrc import RUNTIME, port_src
import argparse
import collections
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess

import babl_audit
import l10n

# conversation strings live in blocks 0xE00 + slot; everything below (game messages, books,
# writings, trap text) prints in the message scroll
CONVERSATION_BASE = 0xE00


def build(uw1):
    out = uw1 / 'build/hd-scroll'
    out.mkdir(parents=True, exist_ok=True)
    exe = out / ('layout-qa' + ('.exe' if os.name == 'nt' else ''))
    env = dict(os.environ)
    env['PKG_CONFIG_PATH'] = str(uw1 / 'tools/libs/lib/pkgconfig') + os.pathsep + env.get('PKG_CONFIG_PATH', '')
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'freetype2'], text=True, env=env))
    root = uw1.parent
    subprocess.run([os.environ.get('CC') or shutil.which('clang') or 'cc', '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror',
                    '-DHD_HAVE_FREETYPE', '-I' + str(uw1 / 'src/port'), "-I" + str(RUNTIME), '-I' + str(root / 'exhume/runtime/port/platform'),
                    '-I' + str(root / 'exhume/runtime/port'), str(Path(__file__).with_name('layout_qa.c')),
                    *(str(port_src(s)) for s in ('presentation/scroll_text.c', 'presentation/hd_font.c',
                                                          'i18n/layout.c', 'i18n/utf8.c')),
                    '-o', str(exe)] + flags, check=True)
    return exe


def main():
    uw1 = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pak', type=Path, required=True)
    parser.add_argument('--cnv', type=Path, required=True)
    parser.add_argument('--master', type=Path, default=uw1 / 'i18n/zh-TW/master.tsv')
    parser.add_argument('--font', type=Path, default=uw1 / 'tools/fonts/jf-openhuninn-2.1.ttf')
    parser.add_argument('--block', type=int, action='append', required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    inventory = l10n.Inventory.load(args.pak, args.cnv)
    roles = {}
    for header, code, funcs in babl_audit.scripts(args.cnv.read_bytes()):
        if header['resolved_block'] in args.block:
            n = 1 + max(i for b, i in inventory.entries if b == header['resolved_block'])
            uses, _ = babl_audit.sweep(code, funcs, n)
            for index, how in uses.items():
                roles[header['resolved_block'], index] = {c for _, c in how}
    rows = [r for r in l10n.load_master(args.master) if int(r['block']) in args.block and r['target_zh_TW']]
    work = args.report.with_suffix('.work')
    work.mkdir(parents=True, exist_ok=True)
    feed = work / 'input.tsv'
    with open(feed, 'w', encoding='utf-8', newline='\n') as f:
        for r in rows:
            menu = roles.get((int(r['block']), int(r['index'])), set()) & {'babl_menu', 'babl_fmenu'}
            scroll = menu or int(r['block']) < CONVERSATION_BASE
            f.write(f"{r['block']}:{r['index']}\t{0 if scroll else 1}\t{l10n.escape(r['target_zh_TW'])}\n")
    exe = build(uw1)
    env = dict(os.environ)
    env['PATH'] = str(uw1 / 'tools/libs/bin') + os.pathsep + env['PATH']
    subprocess.run([str(exe), str(args.font), str(feed), str(work / 'output.tsv')], check=True, env=env)
    results = [line.split('\t') for line in (work / 'output.tsv').read_text(encoding='utf-8').splitlines()]
    pages = collections.Counter()
    report = {'font': args.font.name, 'rows': len(results), 'errors': [], 'by_box': {}, 'longest': []}
    for key, box, status, lines, more in results:
        if status != 'ok':
            report['errors'].append(key)
        b = report['by_box'].setdefault('message' if box == '0' else 'talk', collections.Counter())
        b['rows'] += 1
        b['multi_page'] += int(more) > 0
        pages[key] = (int(lines), int(more), box)
    report['by_box'] = {k: dict(v) for k, v in report['by_box'].items()}
    report['longest'] = [{'key': k, 'lines': v[0], 'more': v[1], 'box': 'message' if v[2] == '0' else 'talk'}
                         for k, v in sorted(pages.items(), key=lambda kv: (-kv[1][1], -kv[1][0]))[:10]]
    args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f"{len(results)} rows: {len(report['errors'])} errors; " +
          '; '.join(f"{k} {v['rows']} rows, {v.get('multi_page', 0)} over a page" for k, v in report['by_box'].items()))
    return 1 if report['errors'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
