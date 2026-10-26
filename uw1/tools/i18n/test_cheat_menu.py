"""Real cheats/recovery and drawing-only map/brightness code; no game assets."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import struct
uw1=Path(__file__).resolve().parents[2]
root=uw1.parent
os.environ.setdefault('EXHUME_CONFIG',str(uw1/'exhume.toml'))
sys.path.insert(0,str(root/'exhume/tools'))
import portcheck as P
out=uw1/'build/cheat-menu';out.mkdir(parents=True,exist_ok=True)
exe=out/('check.exe' if os.name=='nt' else 'check')
san=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if '--sanitize' in sys.argv else []
obj=out/'menu.o';runtime=out/'runtime.o'
subprocess.run([P.host_cc(),*P.FLAGS,'-I'+str(root/'exhume/runtime/port'),*P.layout_flags(P.host_cc()),*san,'-g','-O1','-c',str(Path(__file__).with_name('cheat_menu_test.c')),'-o',str(obj)],check=True)
subprocess.run([P.host_cc(),'-std=c11','-I'+str(uw1/'src/port'),'-I'+str(root/'exhume/runtime/port/sys'),*san,'-g','-O1','-c',str(root/'exhume/runtime/port/sys/cheat.c'),'-o',str(runtime)],check=True)
subprocess.run([P.host_cc(),str(obj),str(runtime),*san,'-o',str(exe)],check=True)
env=dict(os.environ,ASAN_OPTIONS='halt_on_error=1:detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
env.pop('EXHUME_TEST_CHEATS',None)
with tempfile.TemporaryDirectory() as temp:
    subprocess.run([str(exe)],cwd=temp,env=env,check=True)
    for name,commands in [('public',['menu-god 1','menu-rescue']),('private',['set hp 1']),
                          ('mixed',['menu-god 1','give 1']),('invalid',['menu-god 9'])]:
        data=b'UWRP\x06'+bytes(7)+b'\x09\0\0'
        for i,cmd in enumerate(commands,1):
            text=cmd.encode('ascii');data+=struct.pack('<BHI',10,len(text)+4,i)+text
        path=Path(temp)/(name+'.rec');path.write_bytes(data)
        subprocess.run([str(exe),name,str(path)],cwd=temp,env=env,check=True)
ui=out/('settings.exe' if os.name=='nt' else 'settings')
subprocess.run([P.host_cc(),'-std=c11','-D_DEFAULT_SOURCE','-I'+str(root/'exhume/runtime/port'),
    '-I'+str(root/'exhume/runtime/tests'),*san,str(Path(__file__).with_name('settings_cheats_test.c')),
    *[str(root/'exhume/runtime'/f) for f in ['port/ui/settings.c','port/ui/font.c','port/sys/enhance.c','tests/config_stub.c']],
    '-o',str(ui)],check=True)
subprocess.run([str(ui)],env=env,check=True)
record_source=(root/'exhume/runtime/replay/replay.c').read_text(encoding='utf-8')
fn=record_source.split('void rp_cheat_put(uint32_t poll, const char *text)',1)[1].split('\n#endif',1)[0]
(out/'menu_record_impl.h').write_text('void rp_cheat_put(uint32_t poll, const char *text)'+fn,encoding='utf-8')
rec=out/('record.exe' if os.name=='nt' else 'record')
subprocess.run([P.host_cc(),'-std=c11','-D_POSIX_C_SOURCE=200809L','-I'+str(out),*san,
    str(Path(__file__).with_name('replay_menu_test.c')),'-o',str(rec)],check=True)
subprocess.run([str(rec)],env=env,check=True)
motion=(uw1/'src/motion/PLAYMOVE.C').read_text(encoding='utf-8')
fn=motion.split('void far finish_player(void)',1)[1].split('\n/* One step',1)[0]
(out/'god_wait_impl.h').write_text('void finish_player(void)'+fn,encoding='utf-8')
wait=out/('wait.exe' if os.name=='nt' else 'wait')
subprocess.run([P.host_cc(),'-std=c11','-I'+str(out),*san,str(Path(__file__).with_name('cheat_wait_test.c')),'-o',str(wait)],check=True)
subprocess.run([str(wait)],env=env,check=True)
ai=(uw1/'src/critter/AI.C').read_text(encoding='utf-8')
fn=ai.split('char far set_up_target(void)',1)[1].split('\n/* Whether a hurt',1)[0]
(out/'god_target_impl.h').write_text('char far set_up_target(void)'+fn,encoding='utf-8')
target=out/('target.exe' if os.name=='nt' else 'target')
subprocess.run([P.host_cc(),'-std=c11','-I'+str(out),*san,str(Path(__file__).with_name('cheat_target_test.c')),'-o',str(target)],check=True)
subprocess.run([str(target)],env=env,check=True)
