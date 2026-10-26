"""Combat ownership, display state and draw clipping; no original assets or SDL required."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    uw1 = Path(__file__).resolve().parents[2]
    out = uw1 / 'build/combat-text'
    out.mkdir(parents=True, exist_ok=True)
    exe = out / ('combat' + ('-asan' if args.sanitize else '') + ('.exe' if os.name == 'nt' else ''))
    cc = os.environ.get('CC') or shutil.which('clang') or 'cc'
    command = [cc, '-std=c11', '-g', '-O1', '-Wall', '-Wextra', '-Werror',
               '-I' + str(uw1 / 'src/port'), '-I' + str(uw1.parent / 'exhume/runtime/port/platform'),
               str(Path(__file__).with_name('combat_text_test.c')),
               *[str(uw1 / 'src/port' / s) for s in ['presentation/combat_text.c', 'presentation/combat_text_draw.c', 'i18n/utf8.c']],
               '-o', str(exe)]
    if os.name != 'nt':
        command += ['-lm']
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    subprocess.run(command, check=True)
    # LeakSanitizer is unavailable with the Windows and Apple Clang ASan runtimes.
    env = dict(os.environ, ASAN_OPTIONS='halt_on_error=1:detect_leaks=' + ('1' if sys.platform.startswith('linux') else '0'),
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    subprocess.run([str(exe)], check=True, env=env)

    # Use the exact game functions, including their native hooks, with deterministic AI inputs.
    os.environ.setdefault('EXHUME_CONFIG', str(uw1 / 'exhume.toml'))
    sys.path.insert(0, str(uw1.parent / 'exhume/tools'))
    import portcheck as P
    ai = (uw1 / 'src/critter/AI.C').read_text(encoding='utf-8')
    names = ['unsigned char far crit_attack(', 'char far maybe_cast_defensive_spell(',
             'unsigned char far crit_magik_attack(', 'unsigned char far crit_missile_attack(']
    functions = [start + ai.split(start, 1)[1].split('\n}\n', 1)[0] + '\n}\n' for start in names]
    (out / 'combat_attack_impl.h').write_text('\n'.join(functions), encoding='utf-8')
    obj = out / ('attack-asan.o' if args.sanitize else 'attack.o')
    san = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if args.sanitize else []
    subprocess.run([P.host_cc(), *P.FLAGS, *P.layout_flags(P.host_cc()),
                    '-I' + str(uw1.parent / 'exhume/runtime/port'), '-I' + str(out), *san, '-g', '-O1',
                    '-c', str(Path(__file__).with_name('combat_attack_test.c')), '-o', str(obj)], check=True)
    attack = out / ('attack' + ('-asan' if args.sanitize else '') + ('.exe' if os.name == 'nt' else ''))
    subprocess.run([P.host_cc(), '-std=c11', '-I' + str(uw1 / 'src/port/presentation'), *san,
                    str(obj), str(uw1 / 'src/port/presentation/combat_text.c'), '-o', str(attack)], check=True)
    subprocess.run([str(attack)], check=True, env=env)

if __name__ == '__main__':
    main()
