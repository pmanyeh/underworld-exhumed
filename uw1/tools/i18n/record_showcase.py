#!/usr/bin/env python3
"""Private Windows window-only showcase; requires a staged P8 arrival SAVE1.

Uses the existing Native input-script hook. No DOS, cheats, save editing, system
audio, desktop capture or downloads. Every output directory must be new. Run a
dry-run first, inspect its PNGs, then record twice. Videos and saves stay local.
"""
import argparse
import array
from fractions import Fraction
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import threading
import time
import wave

from record_vertical_slice import Script
from validate_vertical_slice import player


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def save_json(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def json_rows(path):
    return [json.loads(line) for line in path.read_text(encoding='utf-8').splitlines()]


def glide(script, ms, start, end):
    for i in range(1, 6):
        x, y = (round(a+(b-a)*i/5) for a, b in zip(start, end))
        script.add(ms+i*100, f'move {x} {y}')


def timeline(case, turns, prepare=False):
    script = Script()
    script.load()
    if prepare:
        for ms in (5500, 6200, 6900, 7600):
            script.chord(ms, 'shift+d')
        for ms in (8400, 9200):
            script.chord(ms, 'shift+w')
        for ms in (10000, 10700):
            script.chord(ms, 'shift+d')
        for ms in (11500, 12300):
            script.chord(ms, 'shift+w')
        for ms in (13100, 13800):
            script.chord(ms, 'shift+d')
        for ms in (14600, 15400):
            script.chord(ms, 'shift+w')
        script.save(17000, 1, 'showcase')
        return script, 34000, {'staged-world': 33000}
    for i in range(turns):
        script.chord(8000+700*i, 'shift+a')
    script.add(9500, 'key f2')
    previous = (100, 99)
    ms = 12000
    for direction in range(3):
        for y in (85, 105):
            for x in (100, 140, 180):
                glide(script, ms-600, previous, (x, y))
                script.click(ms, x, y, 'right', 100)
                previous = (x, y)
                ms += 800
        if direction < 2:
            script.chord(ms, 'shift+d')
            script.chord(ms+700, 'shift+d')
            ms += 1600
    if case == 'tech':
        for ms, key in ((32000, 'f11'), (32500, 'tab'), (33000, 'tab'),
                        (33500, 'down'), (36000, 'right'), (39000, 'esc')):
            script.add(ms, 'key '+key)
    choices = ((45000, 1), (63000, 2), (73000, 2)) if case == 'tech' else (
               (35000, 1), (55000, 2), (68000, 2))
    for ms, choice in choices:
        y = 173+(choice-1)*8
        glide(script, ms-1200, (148, 75) if choice == 1 else (30, 173), (30, y))
        script.click(ms, 30, y, held=100)
    script.add(51000 if case == 'tech' else 42000, 'key space')
    look = 78000 if case == 'tech' else 73000
    script.add(look, 'key f4')
    script.click(look+1000, 252, 85, 'right', 100)
    if case == 'main':
        script.add(76000, 'key f6')
        script.click(77000, 252, 85, 'right', 100)
        script.add(81000, 'key c')
    turn = 80000 if case == 'tech' else 83000
    for i in range(4):
        script.chord(turn+700*i, 'shift+d')
    for ms in ((84000, 86000) if case == 'tech' else (87000, 89000, 91000)):
        script.chord(ms, 'shift+w')
    end = 90000 if case == 'tech' else 97000
    shots = {'opening-world': 7000, 'first-chinese': 31000,
             'choice': 43000 if case == 'tech' else 33000,
             'more': 49000 if case == 'tech' else 40000,
             'post-conversation-world': 76000 if case == 'tech' else 71000,
             'continued-world': end-2500}
    if case == 'main':
        shots['backpack-open'] = 79000
    if case == 'tech':
        shots['scale-3'] = 31000
        shots['scale-4'] = 41000
    return script, end, shots


class TraceObserver(threading.Thread):
    """Timestamp flushed diagnostic rows, without reading gameplay memory.

    These are host observation times, not game clock values. The largest poll
    interval is saved so reading-time checks can reject unobserved intervals.
    """
    def __init__(self, out, start):
        super().__init__(daemon=True)
        self.out, self.start_time = out, start
        self.stop_event = threading.Event()
        self.error = None
        self.max_poll_ms = 0

    def run(self):
        try:
            position = 0
            previous = time.monotonic()
            with (self.out/'observed.jsonl').open('w', encoding='utf-8') as target:
                while True:
                    now = time.monotonic()
                    self.max_poll_ms = max(self.max_poll_ms, (now-previous)*1000)
                    previous = now
                    source = self.out/'scroll.jsonl'
                    if source.exists():
                        with source.open(encoding='utf-8') as stream:
                            stream.seek(position)
                            while True:
                                line = stream.readline()
                                if not line.endswith('\n'):
                                    break
                                row = json.loads(line)
                                row['observed_ms'] = round((now-self.start_time)*1000)
                                target.write(json.dumps(row, ensure_ascii=False)+'\n')
                                position = stream.tell()
                            target.flush()
                    if self.stop_event.wait(.02):
                        break
        except Exception as error:
            self.error = str(error)


def run_tool(command, log, **kwargs):
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            creationflags=subprocess.CREATE_NO_WINDOW, **kwargs)
    with log.open('ab') as stream:
        stream.write(result.stdout)
    require(result.returncode == 0, f'Tool failed; inspect {log.name}')
    return result.stdout


def snapshot(ffmpeg, path, log):
    run_tool([ffmpeg, '-hide_banner', '-loglevel', 'error', '-f', 'gdigrab',
              '-framerate', '10', '-draw_mouse', '0', '-i', 'title=Ultima Underworld',
              '-frames:v', '1', '-update', '1', '-y', str(path)], log, timeout=8)
    from PIL import Image
    with Image.open(path) as image:
        require(image.size in ((960, 720), (1280, 960)), 'Unexpected client dimensions')
        require(any(high-low > 20 for low, high in image.convert('RGB').getextrema()),
                'Blank window capture')
        return image.size


class Capture:
    def __init__(self, ffmpeg, out, name, fps, crf, epoch):
        self.path = out/(name+'.mp4')
        self.log = (out/(name+'.log')).open('wb')
        self.start = time.monotonic()-epoch
        self.command = [ffmpeg, '-hide_banner', '-loglevel', 'warning', '-f', 'gdigrab',
            '-framerate', str(fps), '-draw_mouse', '0', '-i', 'title=Ultima Underworld',
            '-an', '-vf', 'pad=1280:960:(ow-iw)/2:(oh-ih)/2:color=black',
            '-c:v', 'libx264', '-preset', 'veryfast', '-crf', str(crf),
            '-pix_fmt', 'yuv420p', '-r', str(fps), '-movflags', '+faststart',
            '-y', str(self.path)]
        self.process = subprocess.Popen(self.command, stdin=subprocess.PIPE,
            stdout=self.log, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)

    def stop(self):
        try:
            require(self.process.poll() is None, 'Recorder exited before requested stop')
            self.process.communicate(b'q\n', timeout=30)
            require(self.process.returncode == 0, 'Recorder failed; inspect capture log')
        finally:
            if self.process.poll() is None:
                self.process.kill()
                self.process.wait()
            self.log.close()


def validate_flow(out, observer, case):
    require(observer.error is None, 'Trace observer failed: '+str(observer.error))
    require(observer.max_poll_ms < 500, 'Trace observation gap exceeds 500 ms')
    flow = json_rows(out/'observed.jsonl')
    raw = json_rows(out/'scroll.jsonl')
    require(len(flow) == len(raw), 'Incomplete trace observations')
    selections = [row['value'] for row in flow if row['event'] == 'selection']
    require(selections == [1, 2, 2], f'Unexpected conversation choices: {selections}')
    text = '\n'.join(row['text'] for row in flow if row['event'] == 'text')
    require(all(t in text for t in ('你好，陌生人', '探索，是嗎', '也許吧', '那麼，告辭了',
                                     'Thou needst not be ashamed', 'You see a pack.')),
            'Missing real Bragit dialogue, expanded fallback or post-dialogue item look')
    require('@SS1' not in text, 'Unexpanded conversation template')
    hits = json_rows(out/'lookup.jsonl')
    keys = {(r['block'], r['index']) for r in hits}
    require({(3651, n) for n in (3, 4, 7, 12, 13, 15)} <= keys, 'Missing zh-TW catalog hits')
    menus, mores, menu_start = [], [], None
    for i, row in enumerate(flow):
        if row['event'] == 'write' and row['box'] == 0 and row['value'] > 0:
            menu_start = row['observed_ms']
        if row['event'] == 'selection':
            require(menu_start is not None, 'Selection has no completed menu')
            menus.append(row['observed_ms']-menu_start)
            menu_start = None
        if row['event'] == 'more':
            following = next((r for r in flow[i+1:] if r['event'] == 'write'), None)
            require(following is not None, 'MORE never acknowledged')
            mores.append(following['observed_ms']-row['observed_ms'])
    require(menus and min(menus) >= 3000, f'Menu reading time too short: {menus}')
    require(mores and min(mores) >= 2000, f'MORE reading time too short: {mores}')
    log = (out/'run.log').read_text(encoding='utf-8', errors='replace')
    require(not any(s in log for s in ('ERROR: AddressSanitizer', 'runtime error:',
                'the game stops here', 'DESYNC', 'halted', 'Segmentation fault')), 'Runtime error')
    require('renderer software' in log and 'HD text cache:' in log, 'Missing presentation diagnostics')
    motion = re.search(r'motion: x (-?\d+) y (-?\d+) z (-?\d+) facing (\d+)', log)
    initial = player(out/'SAVE1')[2]
    require(motion and tuple(map(int, motion.groups()[:3])) != initial[:3],
            'Missing continued movement after conversation')
    require((out/'RECORD.OUT').read_bytes()[4] != 6, 'Cheat-capable recording')
    cursor = json_rows(out/'cursor.jsonl')
    require(any(row['visible'] and row['path'] == 1 for row in cursor), 'No native cursor evidence')
    require(any(row['mode'] == 4 for row in cursor) and cursor[-1]['mode'] == 1,
            'Conversation screen did not return to the game screen')
    if case == 'tech':
        require('scale=4' in (out/'uw1port.cfg').read_text(), 'Scale 4 was not persisted')
    return {'choices': selections, 'catalog_keys': sorted(f'{b}:{i}' for b, i in keys),
        'menu_hold_ms': menus, 'more_hold_ms': mores, 'observer_max_poll_ms': observer.max_poll_ms,
        'bragit': True, 'return_to_3d_and_item_look': True, 'continued_motion': list(map(int, motion.groups())),
        'cheats': False, 'game_recording_sha256': digest(out/'RECORD.OUT')}


def probe(ffprobe, path, log):
    return json.loads(run_tool([ffprobe, '-v', 'error', '-show_streams', '-show_format',
                               '-of', 'json', str(path)], log, timeout=30))


def finish_video(args, out, captures, wall_duration):
    audio = None
    if args.audio:
        with wave.open(str(out/'game.wav'), 'rb') as source:
            require(source.getnchannels() == 2 and source.getsampwidth() == 2, 'Unexpected Native WAV format')
            audio = {'rate': source.getframerate(), 'channels': source.getnchannels(),
                     'duration': source.getnframes()/source.getframerate()}
            samples = array.array('h', source.readframes(source.getnframes()))
        require(samples and max(samples)-min(samples) > 512, 'Native WAV is silent')
        audio['range'] = max(samples)-min(samples)
        audio['estimated_origin_seconds'] = wall_duration-audio['duration']
        require(abs(audio['estimated_origin_seconds']) < 3, 'Native audio clock does not track host time')
    parts = []
    for i, capture in enumerate(captures):
        path = out/f'part-{i}.mp4'
        command = [args.ffmpeg, '-hide_banner', '-loglevel', 'error', '-i', str(capture.path)]
        if audio:
            offset = max(0, capture.start-audio['estimated_origin_seconds'])
            command += ['-ss', str(offset), '-i', str(out/'game.wav'), '-map', '0:v:0',
                        '-map', '1:a:0', '-c:v', 'copy', '-c:a', 'aac', '-b:a', '192k', '-shortest']
        else:
            command += ['-c', 'copy']
        run_tool(command+['-y', str(path)], out/'mux.log', timeout=90)
        parts.append(path)
    manifest = out/'parts.txt'
    manifest.write_text(''.join(f"file '{p.name}'\n" for p in parts), encoding='utf-8')
    video = out/f'showcase-{args.case}.mp4'
    run_tool([args.ffmpeg, '-hide_banner', '-loglevel', 'error', '-f', 'concat', '-safe', '1',
              '-i', str(manifest), '-c', 'copy', '-movflags', '+faststart', '-y', str(video)],
             out/'mux.log', timeout=90)
    info = probe(args.ffprobe, video, out/'ffprobe.log')
    save_json(out/'ffprobe.json', info)
    streams = info['streams']
    v = next(s for s in streams if s['codec_type'] == 'video')
    require(video.stat().st_size > 100000, 'Empty or implausibly small MP4')
    duration = float(info['format']['duration'])
    lo, hi = (83, 93) if args.case == 'tech' else (90, 101)
    require(lo <= duration <= hi, f'Unexpected duration: {duration}')
    require((v['width'], v['height'], v['codec_name'], v['pix_fmt']) ==
            (1280, 960, 'h264', 'yuv420p'), 'Unexpected video stream')
    require(Fraction(v['avg_frame_rate']) == args.fps, 'Unexpected frame rate')
    require(any(s['codec_type'] == 'audio' for s in streams) == args.audio, 'Unexpected audio streams')
    run_tool([args.ffmpeg, '-v', 'error', '-xerror', '-i', str(video), '-f', 'null', '-'],
             out/'decode.log', timeout=120)
    return {'path': video.name, 'bytes': video.stat().st_size, 'sha256': digest(video),
            'duration': duration, 'width': v['width'], 'height': v['height'], 'fps': args.fps,
            'codec': v['codec_name'], 'pixel_format': v['pix_fmt'], 'audio': audio,
            'full_decode': 'PASS', 'segments': [{'start_seconds': c.start, 'path': c.path.name} for c in captures]}


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', required=True, choices=('main', 'tech'))
    for name in ('data', 'stage', 'output'):
        parser.add_argument('--'+name, required=True, type=Path)
    parser.add_argument('--exe', type=Path, default=root/'build/port/uw1port.exe')
    parser.add_argument('--catalog', type=Path, default=root/'build/i18n/zh-TW.uwloc')
    parser.add_argument('--font', type=Path, default=root/'tools/fonts/NotoSansCJKtc-Regular.otf')
    parser.add_argument('--ffmpeg', default='ffmpeg')
    parser.add_argument('--ffprobe', default='ffprobe')
    parser.add_argument('--fps', type=int, default=60, choices=(30, 60))
    parser.add_argument('--crf', type=int, default=17, choices=range(16, 19))
    parser.add_argument('--dry-run', action='store_true')
    parser.add_argument('--prepare-stage', action='store_true',
                        help='normal walk/save from P8 arrival; produces a private stage, no MP4')
    parser.add_argument('--validated-dry-run', type=Path,
                        help='reuse a matching PASS dry-run directory; otherwise run one before recording')
    parser.add_argument('--turns', type=int, default=0, choices=range(8),
                        help='45-degree left turns from the staged view; P8 arrival needs 2')
    audio = parser.add_mutually_exclusive_group()
    audio.add_argument('--with-audio', dest='audio', action='store_true')
    audio.add_argument('--no-audio', dest='audio', action='store_false')
    parser.set_defaults(audio=True)
    args = parser.parse_args()
    if args.prepare_stage:
        require(args.case == 'main', 'Stage preparation uses --case main')
        args.dry_run = True
    require(os.name == 'nt', 'This capture backend requires Windows')
    for name in ('ffmpeg', 'ffprobe'):
        value = shutil.which(getattr(args, name))
        require(value is not None, name.upper()+'_NOT_FOUND')
        setattr(args, name, value)
    for path in (args.exe, args.catalog, args.font, args.data/'UW.EXE'):
        require(path.is_file(), f'Required file is absent: {path}')
    for name in ('PLAYER.DAT', 'LEV.ARK', 'BGLOBALS.DAT', 'DESC'):
        require((args.stage/'SAVE1'/name).is_file(), f'Stage SAVE1 lacks {name}')
    require(not args.output.exists(), 'Output must be a new directory')
    out = args.output.resolve()
    out.mkdir(parents=True)
    captures, active, game, observer = [], None, None, None
    try:
        versions = {n: run_tool([getattr(args, n), '-version'], out/'tools.log', timeout=15)
                    .decode('utf-8', errors='replace').splitlines()[0] for n in ('ffmpeg', 'ffprobe')}
        shutil.copytree(args.stage/'SAVE1', out/'SAVE1')
        stage_hashes = {p.name: digest(p) for p in (out/'SAVE1').iterdir() if p.is_file()}
        if args.prepare_stage:
            require(player(args.stage/'SAVE1')[2] == (4736, 1664, 768, 0, 1),
                    'Stage preparation requires the verified P8 arrival pose')
        script, end, shots = timeline(args.case, args.turns, args.prepare_stage)
        script.write(out/'input.script')
        env = dict(os.environ)
        env.pop('EXHUME_TEST_CHEATS', None)
        env.pop('EXHUME_TEST_FAULT', None)
        env['SDL_RENDER_DRIVER'] = 'software'
        env['UW1PORT_POS_LOG'] = '1'
        env['PATH'] = str(root/'tools/libs/bin')+os.pathsep+env.get('PATH', '')
        command = [str(args.exe.resolve()), '--data', str(args.data.resolve()), '--home', str(out),
            '--record', '--enhance', 'skip-intro', '--input-script', str(out/'input.script'),
            '--locale', 'zh-TW', '--hd-scroll', '--hd-cursor', '--font', str(args.font.resolve()),
            '--i18n-file', str(args.catalog.resolve()), '--scroll-log', str(out/'scroll.jsonl'),
            '--i18n-log', str(out/'lookup.jsonl'), '--cursor-log', str(out/'cursor.jsonl'),
            '--sound', '3,1' if args.audio else '0,0', '--scale', '3' if args.case == 'tech' else '4',
            '--exit-after', str(end), '--exit-on-halt', '--screenshot-after', str(end-3000),
            '--screenshot', str(out/'scanout.png'), '--window-shot', str(out/'window.png')]
        command += ['--audio-wav', str(out/'game.wav')] if args.audio else ['--no-audio']
        scenario = {'case': args.case, 'dry_run': args.dry_run, 'cheats': False,
            'capture': 'gdigrab game client only; no desktop/system audio', 'renderer': 'software',
            'command': command, 'tools': versions, 'stage_sha256': stage_hashes,
            'exe_sha256': digest(args.exe), 'catalog_sha256': digest(args.catalog),
            'font_sha256': digest(args.font), 'shots_ms': shots, 'script_end_ms': end,
            'prepare_stage': args.prepare_stage}
        save_json(out/'scenario.json', scenario)
        if not args.dry_run:
            dry = args.validated_dry_run
            if dry is None:
                dry = out/'dry-run'
                child = list(sys.argv[1:])
                child[child.index('--output')+1] = str(dry)
                result = subprocess.run([sys.executable, str(Path(__file__).resolve()),
                                         *child, '--dry-run'], env=env)
                require(result.returncode == 0, 'Automatic dry-run failed; recording was not started')
            checked = json.loads((dry/'validation.json').read_text(encoding='utf-8'))
            previous = json.loads((dry/'scenario.json').read_text(encoding='utf-8'))
            require(checked['status'] == 'PASS' and checked['dry_run'] is True and
                    checked['case'] == args.case, 'A matching PASS dry-run is required')
            for key in ('exe_sha256', 'catalog_sha256', 'font_sha256', 'stage_sha256', 'renderer'):
                require(previous[key] == scenario[key], 'Dry-run dependency changed: '+key)
            require(digest(dry/'input.script') == digest(out/'input.script'), 'Dry-run input script changed')
            require(previous['command'][previous['command'].index('--sound')+1] ==
                    command[command.index('--sound')+1], 'Dry-run audio mode changed')
            scenario['validated_dry_run'] = str(dry.resolve())
            save_json(out/'scenario.json', scenario)
        with (out/'run.log').open('wb') as log:
            epoch = time.monotonic()
            game = subprocess.Popen(command, env=env, stdout=log, stderr=subprocess.STDOUT,
                                    creationflags=subprocess.CREATE_NO_WINDOW)
            observer = TraceObserver(out, epoch)
            observer.start()
            ready = False
            while time.monotonic()-epoch < 2.5 and game.poll() is None:
                try:
                    snapshot(args.ffmpeg, out/'capture-ready.png', out/'capture.log')
                    ready = True
                    break
                except RuntimeError:
                    time.sleep(.1)
            require(ready, 'Game window capture not ready before load; inspect capture.log')
            if not args.dry_run:
                active = Capture(args.ffmpeg, out, 'capture-0', args.fps, args.crf, epoch)
                captures.append(active)
            pending = dict(shots)
            resized = False
            while game.poll() is None:
                elapsed = (time.monotonic()-epoch)*1000
                # gdigrab fixes its input rectangle at open. Reopen around the
                # real settings resize; pad 3x without enlarging its text pixels.
                if args.case == 'tech' and elapsed >= 35200 and not resized:
                    if active:
                        active.stop()
                        active = None
                    deadline = time.monotonic()+3
                    while True:
                        size = snapshot(args.ffmpeg, out/'resize-ready.png', out/'capture.log')
                        if size == (1280, 960):
                            break
                        require(time.monotonic() < deadline, 'Real 3x -> 4x resize did not occur')
                        time.sleep(.05)
                    if not args.dry_run:
                        active = Capture(args.ffmpeg, out, 'capture-1', args.fps, args.crf, epoch)
                        captures.append(active)
                    resized = True
                for name, ms in list(pending.items()):
                    if elapsed >= ms:
                        size = snapshot(args.ffmpeg, out/(name+'.png'), out/'capture.log')
                        if name == 'scale-3':
                            require(size == (960, 720), '3x screenshot is not native 960x720')
                        if name == 'scale-4':
                            require(size == (1280, 960), '4x screenshot is not native 1280x960')
                        del pending[name]
                if active and elapsed >= end-1200:
                    active.stop()
                    active = None
                require(elapsed < end+30000, 'Game did not finish on time')
                time.sleep(.02)
            wall_duration = time.monotonic()-epoch
            require(game.returncode == 0, f'Game failed: {game.returncode}')
            require(not pending, 'Game exited before all screenshot milestones')
        time.sleep(.05)
        observer.stop_event.set()
        observer.join(timeout=3)
        require(not observer.is_alive(), 'Trace observer did not stop')
        if args.prepare_stage:
            rows = json_rows(out/'scroll.jsonl')
            require(any(r['event'] == 'text' and 'Save Game Succeeded' in r['text'] for r in rows),
                    'Normal in-game stage save did not succeed')
            require(not any(r['event'] == 'selection' or
                    (r['event'] == 'text' and '你好' in r['text']) for r in rows),
                    'Stage must precede the conversation')
            pose = player(out/'SAVE1')[2]
            require(pose[:2] == (4480, 1664), 'Stage walk did not reach the expected room')
            flow = {'stage': True, 'normal_save': True, 'pose': pose, 'cheats': False,
                    'stage_sha256': {p.name: digest(p) for p in (out/'SAVE1').iterdir() if p.is_file()}}
        else:
            flow = validate_flow(out, observer, args.case)
        require(all(digest(args.stage/'SAVE1'/name) == value for name, value in stage_hashes.items()),
                'Source stage changed')
        video = None if args.dry_run else finish_video(args, out, captures, wall_duration)
        save_json(out/'validation.json', {'status': 'PASS', 'case': args.case,
            'dry_run': args.dry_run, 'flow': flow, 'video': video,
            'audio_capture': 'NOT_REQUESTED' if not args.audio else ('WAV_ONLY' if args.dry_run else 'PASS'),
            'visual_review_required': True, 'DOS_executed': False, 'goldens_modified': False})
        print('PASS', args.case, 'dry-run' if args.dry_run else 'recording', out)
    except Exception as error:
        save_json(out/'validation.json', {'status': 'FAIL', 'case': args.case, 'error': str(error)})
        raise
    finally:
        if active and active.process.poll() is None:
            active.stop()
        if game and game.poll() is None:
            game.terminate()
            game.wait(timeout=10)
        if observer and observer.is_alive():
            observer.stop_event.set()
            observer.join(timeout=3)


if __name__ == '__main__':
    main()
