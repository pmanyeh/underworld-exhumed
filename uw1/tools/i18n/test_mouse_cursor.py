#!/usr/bin/env python3
"""Compile the real MOUSE.C cursor paths against graphics-only doubles."""
import os
from pathlib import Path
import subprocess
import sys

uw1 = Path(__file__).resolve().parents[2]
root = uw1.parent
os.environ.setdefault('EXHUME_CONFIG', str(uw1 / 'exhume.toml'))
sys.path.insert(0, str(root / 'exhume/tools'))
import portcheck as P
out = uw1 / 'build/hd-cursor'
out.mkdir(parents=True, exist_ok=True)
obj = out / 'mouse-test.o'
cursor = out / 'cursor-core.o'
exe = out / ('mouse-test.exe' if os.name == 'nt' else 'mouse-test')
cc = P.host_cc()
san = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if '--sanitize' in sys.argv else []
subprocess.run([cc] + P.FLAGS + san + ['-g', '-O1', '-ffunction-sections', '-fdata-sections', '-c',
    str(Path(__file__).with_name('mouse_cursor_test.c')), '-o', str(obj)], check=True)
subprocess.run([cc] + san + ['-std=c11', '-g', '-O1', '-I'+str(uw1/'src/port'),
    '-I'+str(root/'exhume/runtime/port/platform'), '-c',
    str(uw1/'src/port/presentation/hd_cursor.c'), '-o', str(cursor)], check=True)
gc = '-Wl,-dead_strip' if sys.platform == 'darwin' else '-Wl,--gc-sections'
link = ['-Wl,--exclude-all-symbols'] if os.name == 'nt' else []
subprocess.run([cc, str(obj), str(cursor), gc, '-o', str(exe)] + link + san, check=True)
env = dict(os.environ, ASAN_OPTIONS='halt_on_error=1:detect_leaks='+('0' if os.name=='nt' else '1'),
           UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
subprocess.run([str(exe)], check=True, env=env)

# The actual wheel dispatcher and statistics scroll, including the original arrow handler.
parts = []
for path, signatures in [
    ('src/sys/UTIL.C', ['int far mvcheck(']),
    ('src/ui/STATS.C', ['void far port_stats_scroll(', 'void far mous_in_stat(']),
    ('src/port/ui/keys.c', ['void far port_key_wheel('])]:
    source = (uw1 / path).read_text(encoding='utf-8')
    parts += [sig + source.split(sig, 1)[1].split('\n}', 1)[0] + '\n}\n' for sig in signatures]
(out / 'stats_wheel_impl.h').write_text('\n'.join(parts), encoding='utf-8')
wheel = out / ('stats-wheel.exe' if os.name == 'nt' else 'stats-wheel')
subprocess.run([cc, '-std=c11', '-Wall', '-Wextra', '-Werror', '-g', '-O1', '-I' + str(out),
                *san, str(Path(__file__).with_name('stats_wheel_test.c')), '-o', str(wheel)], check=True)
subprocess.run([str(wheel)], check=True, env=env)
