#!/usr/bin/env python3
"""Check P8 player-flow evidence and save semantics; never execute DOS or change saves.

Run after recording and verifying the five Native cases. Screenshot appearance
still needs visual review; these checks cannot certify translation quality.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
import struct

from extract_strings import unescape


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rows(path):
    return [json.loads(line) for line in path.read_text(encoding='utf-8').splitlines()]


def player(folder):
    raw = (folder/'PLAYER.DAT').read_bytes()
    assert len(raw) >= 240
    decoded = bytes(value ^ ((raw[0]+3*(i % 80+1)) & 255) for i, value in enumerate(raw[1:211]))
    return raw, decoded, struct.unpack_from('<hhhhh', decoded, 0x54)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    root = Path(__file__).resolve().parents[2]
    for name in ('walk', 'dialogue', 'restore', 'english', 'resize'):
        parser.add_argument('--'+name, required=True, type=Path)
    parser.add_argument('--replays', required=True, type=Path, help='directory of NAME-replay.json reports')
    parser.add_argument('--poc', type=Path, default=root/'i18n/zh-TW/poc.tsv')
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    from PIL import Image

    with args.poc.open(encoding='utf-8', newline='') as stream:
        expected = {(int(row['block']), int(row['index'])): unescape(row['zh_tw'])
                    for row in csv.DictReader(stream, delimiter='\t') if row['kind'] == 'pak'}
    hits = rows(args.dialogue/'lookup.jsonl')
    for hit in hits:
        assert hit['text'] == expected[(hit['block'], hit['index'])], hit
    observed = {(hit['block'], hit['index']) for hit in hits}
    assert {(3651, i) for i in (3, 7, 10, 16, 19, 20, 23, 26, 30, 33)} <= observed
    flow = rows(args.dialogue/'scroll.jsonl')
    selections = [r['value'] for r in flow if r['event'] == 'selection']
    assert selections == [1, 1, 2, 2, 1, 1, 2], selections
    text = '\n'.join(r['text'] for r in flow if r['event'] == 'text')
    assert expected[(3651, 33)] in text and '@SS1' not in text
    assert 'Thou needst not be ashamed' in text and 'dextrous lad' in text
    assert sum(r['event'] == 'more' for r in flow) >= 3
    assert 'You see a pack.' in text and 'Save Game Succeeded.' in text
    walk_text = '\n'.join(r['text'] for r in rows(args.walk/'scroll.jsonl') if r['event'] == 'text')
    assert expected[(1, 5)] in walk_text, 'walk must use the key to unlock the door'
    _, _, arrival = player(args.walk/'SAVE1')
    assert arrival == (4736, 1664, 768, 0, 1), arrival
    for name in ('walk', 'dialogue', 'restore', 'english', 'resize'):
        home = getattr(args, name)
        recording = (home/'RECORD.OUT').read_bytes()
        assert recording[4] != 6, (name, 'cheat-capable recording')
        log = (home/'run.log').read_text(encoding='utf-8')
        assert not any(error in log for error in ('ERROR: AddressSanitizer', 'runtime error:', 'the game stops here', 'DESYNC'))

    # A restore followed by a save must preserve all player/inventory bytes except
    # the existing four-byte game clock. No other save fields are masked here.
    r2, p2, pos2 = player(args.restore/'SAVE2')
    r3, p3, pos3 = player(args.restore/'SAVE3')
    _, _, pos4 = player(args.restore/'SAVE4')
    assert pos3 == pos2 and pos4[:3] == (pos3[0]-128, pos3[1], pos3[2]), (pos2, pos3, pos4)
    assert r2[:207]+r2[211:] == r3[:207]+r3[211:], 'restore changed player/inventory beyond clock'
    assert p2[:0xCE] == p3[:0xCE]
    globals2 = (args.restore/'SAVE2/BGLOBALS.DAT').read_bytes()
    assert all((args.restore/f'SAVE{n}/BGLOBALS.DAT').read_bytes() == globals2 for n in (3, 4))
    restore_log = (args.restore/'run.log').read_text(encoding='utf-8')
    motion = re.search(r'motion: x (-?\d+) y (-?\d+) z (-?\d+) facing (\d+)', restore_log)
    assert motion and tuple(map(int, motion.groups())) == (4736, 1664, 768, 57344), motion
    restore_text = '\n'.join(r['text'] for r in rows(args.restore/'scroll.jsonl') if r['event'] == 'text')
    assert 'Restore Game Complete.' in restore_text

    # Independent English/Chinese playthroughs have different menu/clock timing
    # and can find Bragit in different directions. Compare persistent script state,
    # player data apart from facing/clock, and serialized items after the avatar.
    modern = args.dialogue/'SAVE2'
    english = args.english/'SAVE2'
    assert (modern/'BGLOBALS.DAT').read_bytes() == (english/'BGLOBALS.DAT').read_bytes()
    rm, pm, _ = player(modern)
    re_, pe, _ = player(english)
    assert pm[:0x5A]+pm[0x5C:0xCE] == pe[:0x5A]+pe[0x5C:0xCE]
    assert rm[240:] == re_[240:], 'serialized item slots/objects differ'

    geometry = {}
    for name in ('dialogue', 'resize'):
        home = getattr(args, name)
        with Image.open(home/'scanout.png') as image:
            assert image.size == (320, 200)
            for box in ((15, 169, 306, 199), (56, 53, 220, 132)):
                assert len(set(image.crop(box).get_flattened_data())) == 1, (name, box, 'legacy glyphs')
        with Image.open(home/'window.png') as image:
            geometry[name] = image.size
    assert geometry == {'dialogue': (960, 720), 'resize': (1280, 960)}, geometry
    resized = rows(args.resize/'scroll.jsonl')
    assert [r['value'] for r in resized if r['event'] == 'selection'] == [1]
    # The first response has the same prepared scalar count after the real resize.
    assert any(r['event'] == 'write' and r['box'] == 1 and r['cells'] == 246 for r in resized)
    assert 'scale=4' in (args.resize/'uw1port.cfg').read_text(encoding='utf-8')
    with Image.open(args.english/'scanout.png') as image:
        assert len(set(image.crop((56, 53, 220, 132)).get_flattened_data())) > 1

    replay = {}
    for name in ('walk', 'dialogue', 'restore', 'english', 'resize'):
        report = json.loads((args.replays/(name+'-replay.json')).read_text(encoding='utf-8'))
        assert report['status'] == 'PASS' and report['DOS_executed'] is False and report['cheats'] == []
        assert report['recording_sha256'] == sha(getattr(args, name)/'RECORD.OUT')
        assert [run['run'] for run in report['runs']] == ['replay1', 'replay2', 'sanitizer']
        assert all(run['status'] == 'PASS' for run in report['runs'])
        assert report['additional_section_masks'] == [] and report['goldens_modified'] is False
        replay[name] = report['runs'][0]['checkpoints']
    result = {'status': 'PASS', 'scope': 'P8 Native player-flow and save semantics; visual review separate',
        'walk_arrival_xyz_facing_level': arrival, 'mouse_choices': selections,
        'external_catalog_keys_observed': sorted(f'{b}:{i}' for b, i in observed),
        'dynamic_fallback_expanded_English': True, 'MORE_events': sum(r['event'] == 'more' for r in flow),
        'restore_player_and_inventory_exact_except_game_clock': True,
        'restore_position_before_movement': pos3, 'saved_position_after_movement': pos4,
        'continued_after_restore': True, 'English_and_modern_BGLOBALS_exact': True,
        'English_and_modern_player_except_facing_clock_equal': True,
        'English_and_modern_serialized_items_equal': True,
        'window_geometry': geometry, 'legacy_scroll_papers_uniform': True,
        'English_original_glyphs_present': True, 'cheats': False,
        'replay_checkpoints_per_run': replay, 'sanitizer_runs': 5,
        'recording_sha256': {name: sha(getattr(args, name)/'RECORD.OUT') for name in replay}}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
