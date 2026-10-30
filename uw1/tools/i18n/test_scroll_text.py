#!/usr/bin/env python3
"""Build/run real display flow and provenance checks; no game assets."""
from portsrc import RUNTIME, port_src
import argparse
import os
from pathlib import Path
import shlex
import shutil
import struct
import subprocess
from l10n import MAGIC


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    display = parser.add_mutually_exclusive_group()
    display.add_argument('--window', action='store_true')
    display.add_argument('--render', action='store_true', help='real FreeType metrics/masks without a window')
    args = parser.parse_args()
    uw1 = Path(__file__).resolve().parents[2]
    root = uw1.parent
    out = uw1/'build/hd-scroll'
    out.mkdir(parents=True, exist_ok=True)
    rendering = args.window or args.render
    name = ('window' if args.window else 'render' if args.render else 'flow') + ('-asan' if args.sanitize else '')
    exe = out/(name+('.exe' if os.name=='nt' else ''))
    env = dict(os.environ)
    env['PKG_CONFIG_PATH'] = str(uw1/'tools/libs/lib/pkgconfig')+os.pathsep+env.get('PKG_CONFIG_PATH','')
    packages = ['sdl3']+(['freetype2'] if rendering else [])
    flags = shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs']+packages,text=True,env=env))
    sources = ['presentation/scroll_text.c','i18n/layout.c','i18n/utf8.c','i18n/input_alias.c','i18n/phrases.c']
    if rendering:
        sources += ['presentation/scroll_overlay.c','presentation/hd_font.c']
    else:
        sources += ['presentation/scroll_bridge.c','i18n/localization.c','i18n/cp437.c','i18n/user_text.c','i18n/name_gloss.c']
    command = [os.environ.get('CC') or shutil.which('clang') or 'cc','-std=c11','-g','-O1',
        '-Wall','-Wextra','-Werror','-Wno-unused-function','-I'+str(uw1/'src/port'), "-I" + str(RUNTIME),
        '-I'+str(root/'exhume/runtime/port/platform'),'-I'+str(root/'exhume/runtime/port'),
        str(Path(__file__).with_name('scroll_window_test.c' if args.window else 'scroll_render_test.c' if args.render else 'scroll_text_test.c'))]
    command += [str(port_src(s)) for s in sources]
    if rendering:
        command += ['-DHD_HAVE_FREETYPE',str(root/'exhume/runtime/port/platform/png.c')]
        if os.name!='nt':command+=['-lm']
    if args.sanitize:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command+['-o',str(exe)]+flags,check=True)
    env['PATH']=str(uw1/'tools/libs/bin')+os.pathsep+env['PATH']
    env['ASAN_OPTIONS']='halt_on_error=1:detect_leaks='+('0' if os.name=='nt' else '1')
    env['UBSAN_OPTIONS']='halt_on_error=1:print_stacktrace=1'
    data=MAGIC+struct.pack('<I',2)
    for index,source,target in [(3,'Hail','你好'),(4,'Hello @SS1','你好@SS1')]:
        a,b=source.encode(),target.encode()
        data+=struct.pack('<HHII',3651,index,len(a),len(b))+a+b'\0'+b+b'\0'
    catalog=out/'flow.uwloc';catalog.write_bytes(data)
    run=[str(exe),str(uw1/'tools/fonts/jf-openhuninn-2.1.ttf'),str(out/name)] if rendering else [str(exe),str(catalog)]
    if args.render: run.append(str(uw1/'tools/fonts/NotoSansCJKtc-Regular.otf'))  # a fallback for what huninn lacks
    result=subprocess.run(run,capture_output=True,text=True,env=env)
    print(result.stdout,end='',flush=True);print(result.stderr,end='',flush=True)
    result.check_returncode()


if __name__=='__main__':main()
