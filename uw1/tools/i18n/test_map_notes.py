"""Chinese map-note persistence, independent overlays and input panel, without game assets."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    uw1 = Path(__file__).resolve().parents[2]
    out = uw1 / 'build/map-notes'
    out.mkdir(parents=True, exist_ok=True)
    # Exercise the actual game bridge without linking the game's archive/editor services.
    bridge = (uw1 / 'src/port/ui/map_notes_bridge.c').read_text(encoding='utf-8')
    signatures = ['static MapNoteKey key_of(', 'int port_map_note_draw(', 'int port_map_note_width(']
    (out / 'map_note_bridge_impl.h').write_text('\n'.join(
        sig + bridge.split(sig, 1)[1].split('\n}', 1)[0] + '\n}\n' for sig in signatures), encoding='utf-8')
    exe = out / ('map-notes' + ('-asan' if args.sanitize else '') + ('.exe' if os.name == 'nt' else ''))
    sources = ['i18n/map_notes.c', 'i18n/user_text.c', 'i18n/utf8.c', 'presentation/hd_label.c',
               'presentation/map_note_overlay.c', 'presentation/text_entry.c']
    cmd = [os.environ.get('CC') or shutil.which('clang') or 'cc', '-std=c11', '-g', '-O1', '-Wall', '-Wextra', '-Werror',
           '-I' + str(out), '-I' + str(uw1 / 'src/port'), '-I' + str(uw1.parent / 'exhume/runtime/port/platform'),
           str(Path(__file__).with_name('map_notes_test.c')), *[str(uw1 / 'src/port' / s) for s in sources], '-o', str(exe)]
    if args.sanitize:
        cmd += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    subprocess.run(cmd, check=True)
    env = dict(os.environ, ASAN_OPTIONS='halt_on_error=1:detect_leaks=' + ('1' if sys.platform.startswith('linux') else '0'),
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    with tempfile.TemporaryDirectory() as tmp:
        subprocess.run([str(exe), tmp], check=True, env=env)

if __name__ == '__main__':
    main()
