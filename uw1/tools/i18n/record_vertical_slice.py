#!/usr/bin/env python3
"""Record the Native P8 player route using real key/mouse input, never cheats.

Requires the user's own legal UW1 data, built Native executable and (for modern
display) font/catalog. Each --output must be new; stage copies only SAVE1..4.
The walk follows the repository's documented public talk route up to talk mode.
Inspect screenshots and use verify_native_recording.py to check each recording.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tomllib


class Script:
    def __init__(self):
        self.events = []

    def add(self, ms, event):
        self.events.append((ms, event))

    def chord(self, ms, chord):
        parts = chord.lower().replace('control', 'ctrl').replace('escape', 'esc').split('+')
        for mod in parts[:-1]:
            self.add(ms, 'down '+mod)
            ms += 60
        self.add(ms, 'key '+parts[-1])
        ms += 150
        for mod in reversed(parts[:-1]):
            self.add(ms, 'up '+mod)
            ms += 60
        return ms + 400

    def click(self, ms, x, y, button='left', held=220):
        self.add(ms, f'mdown {x} {y} {button}')
        self.add(ms+held, f'mup {x} {y} {button}')

    def load(self, slot=1):
        self.add(3000, 'key enter')
        for i in range(slot-1):
            self.add(4300+i*200, 'key down')
        self.add(5000 if slot > 1 else 4500, 'key enter')

    def save(self, ms, slot, name):
        self.chord(ms, 'ctrl+s')
        for i in range(slot-1):
            self.add(ms+1800+i*200, 'key down')
        self.add(ms+3000, 'key enter')
        # Clear any prefilled description explicitly, keeping ASCII input unchanged.
        for i in range(30):
            self.add(ms+4500+i*80, 'key backspace')
        for i, char in enumerate(name):
            self.add(ms+7500+i*200, 'key '+char)
        self.add(ms+10000, 'key enter')

    def write(self, path):
        path.write_text(''.join(f'{ms} {event}\n' for ms, event in sorted(self.events)), encoding='utf-8')


def walk(script, root):
    script.add(2000, 'key enter')
    for i in range(8):
        script.add(3500+1500*i, 'key enter')
    for ms, key in ((16000, 'a'), (17000, 'enter'), (18500, 'enter')):
        script.add(ms, 'key '+key)
    steps = tomllib.loads((root/'exhume.toml').read_text(encoding='utf-8'))['replay']['steps']['talk']
    steps = steps[2:steps.index('k:F2')]
    ms, x, y = 24000, 160, 100
    for step in steps:
        op, _, arg = step.partition(':')
        if op == 'w':
            ms += int(arg)
        elif op == 'k':
            ms = script.chord(ms, arg)
        elif op == 'M':
            x, y = 0, 0
            script.add(ms, 'move 0 0')
            ms += 200
        elif op == 'm':
            x, y = (int(v)//2 for v in arg.split(','))
            script.add(ms, f'move {x} {y}')
            ms += 200
        elif op == 'c':
            button, held = arg.split(',')
            script.click(ms, x, y, button, int(held))
            ms += int(held)+200
        elif op != 's':
            raise ValueError(step)
    arrival = ms
    # Same input sequence as the initial accepted P8 walk fixture.
    ms = script.chord(ms, 'ctrl+s')+1800
    ms = script.chord(ms, 'enter')+1800
    for char in 'p8arrival':
        ms = script.chord(ms, char)+50
    ms = script.chord(ms, 'enter')+4000
    return ms, ms+1000, {'arrival_ms': arrival, 'save_slot': 1}


def dialogue(script, resize=False, keyboard=False):
    script.load()
    if resize:
        script.chord(8000, 'shift+a')
        script.chord(8700, 'shift+a')
        script.add(9400, 'key f2')
        ms = 10000
        for y in range(35, 116, 8):
            for x in range(60, 216, 8):
                script.click(ms, x, y, 'right', 100)
                ms += 180
        script.click(54000, 30, 173)
        for ms in range(55500, 60500, 750):
            script.add(ms, 'key space')
        # Do not send MORE acknowledgements while settings owns keyboard focus.
        for ms, key in ((61000, 'f11'), (61150, 'tab'), (61300, 'tab'),
                        (61500, 'down'), (61700, 'right'), (62000, 'esc')):
            script.add(ms, 'key '+key)
        return 65000, 66000, {'choices': [1], 'settings_resize': '3x -> 4x'}
    script.chord(8000, 'shift+a')
    script.add(9000, 'key f2')
    ms = 9500
    # Bragit wanders: probe the view in two directions using ordinary talk clicks.
    for direction in range(2):
        for y in (35, 55, 75, 95, 115):
            for x in (60, 82, 105, 127, 150, 172, 195, 215):
                script.click(ms, x, y, 'right', 180)
                ms += 450
        if direction == 0:
            script.chord(ms, 'shift+a')
            ms += 1000
    choices = [(50000, 1), (60000, 1), (70000, 2), (80000, 2),
               (90000, 1), (100000, 1), (114000, 2)]
    for start, choice in choices:
        if keyboard:
            script.add(start, 'key '+str(choice))
        else:
            script.click(start, 30, 173+(choice-1)*8)
        for ms in range(start+1500, start+(12500 if start == 100000 else 8000), 750):
            script.add(ms, 'key space')
    script.add(124000, 'key f4')
    script.click(125000, 252, 85, 'right', 250)
    script.chord(127000, 'ctrl+s')
    script.add(129000, 'key down')
    script.add(130000, 'key enter')
    for i, char in enumerate('p8done'):
        script.add(132000+i*200, 'key '+char)
    script.add(134000, 'key enter')
    return 58000, 140000, {'choices': [choice for _, choice in choices], 'save_slot': 2}


def restore(script):
    script.load(2)
    script.save(11000, 3, 'p8loaded')
    script.chord(24000, 'shift+w')
    script.save(27000, 4, 'p8moved')
    script.chord(41000, 'ctrl+r')
    script.add(42800, 'key down')
    script.add(43000, 'key down')
    script.add(44000, 'key enter')
    script.add(48000, 'key f6')
    script.click(49000, 252, 85, 'right', 250)
    script.chord(51000, 'shift+d')
    return 53000, 54000, {'load_slot': 2, 'save_slots': [3, 4], 'restore_slot': 3,
                          'post_restore': 'use pack, turn right'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    root = Path(__file__).resolve().parents[2]
    parser.add_argument('--case', required=True, choices=('walk', 'dialogue', 'restore', 'resize'))
    parser.add_argument('--data', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--stage', type=Path, help='prior case output containing SAVE1..4')
    parser.add_argument('--exe', type=Path, default=root/'build/port'/('uw1port.exe' if os.name == 'nt' else 'uw1port'))
    parser.add_argument('--catalog', type=Path, default=root/'build/i18n/zh-TW.uwloc')
    parser.add_argument('--font', type=Path, default=root/'tools/fonts/NotoSansCJKtc-Regular.otf')
    parser.add_argument('--english', action='store_true', help='original English text and cursor renderer')
    args = parser.parse_args()
    if args.case != 'walk' and not args.stage:
        parser.error('--stage is required after the walk')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    if args.stage:
        for slot in range(1, 5):
            source = args.stage/f'SAVE{slot}'
            if source.is_dir():
                shutil.copytree(source, out/source.name)
    script = Script()
    shot, end, expected = walk(script, root) if args.case == 'walk' else (
        restore(script) if args.case == 'restore' else dialogue(script, args.case == 'resize', args.english))
    script.write(out/'input.script')
    command = [str(args.exe.resolve()), '--data', str(args.data.resolve()), '--home', str(out),
        '--record', '--sound', '0,0', '--no-audio', '--enhance', 'skip-intro',
        '--input-script', str(out/'input.script'), '--locale', 'en' if args.english else 'zh-TW',
        '--screenshot-after', str(shot), '--screenshot', str(out/'scanout.png'),
        '--window-shot', str(out/'window.png'), '--exit-after', str(end), '--scale', '3', '--exit-on-halt']
    if not args.english:
        command += ['--hd-scroll', '--i18n-file', str(args.catalog.resolve()), '--font', str(args.font.resolve()),
                    '--scroll-log', str(out/'scroll.jsonl'), '--i18n-log', str(out/'lookup.jsonl'),
                    '--cursor-log', str(out/'cursor.jsonl')]
    (out/'scenario.json').write_text(json.dumps({'case': args.case, 'english': args.english,
        'cheats': False, 'expected': expected, 'command': command}, indent=2)+'\n', encoding='utf-8')
    env = dict(os.environ)
    env.pop('EXHUME_TEST_CHEATS', None)
    env['UW1PORT_POS_LOG'] = '1'
    env['PATH'] = str(root/'tools/libs/bin')+os.pathsep+env.get('PATH', '')
    print(args.case, 'English' if args.english else 'HD zh-TW', end, 'ms;', out, flush=True)
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            env=env, timeout=end/1000+60)
    (out/'run.log').write_bytes(result.stdout)
    print(result.stdout.decode('utf-8', errors='replace'), flush=True)
    result.check_returncode()


if __name__ == '__main__':
    main()
