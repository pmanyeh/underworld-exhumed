#!/usr/bin/env python3
"""Native HD cursor: replay public sessions against committed gameplay goldens.

Video memory/framebuffer and graphics scratch intentionally change when cursor
rasterization moves to RGBA. Every other normalized section, checkpoint header,
and saved file must match. No DOS build or golden regeneration is performed.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    os.environ['EXHUME_CONFIG'] = str(args.config.resolve())
    sys.path.insert(0, str(Path(__file__).resolve().parents[3]/'exhume/tools'))
    import replay as R
    import golden as G
    work = Path(R.OUT)/'verify-hd-cursor'
    work.mkdir(parents=True, exist_ok=True)
    ignored = {'VGA', 'FBUF', 'GFX'}
    results = {}

    def run(name):
        if name in results:
            return
        stage = None
        if name in G.STAGE_FROM:
            source = G.STAGE_FROM[name]
            run(source)
            stage = work/(name+'-stage')
            if stage.exists():
                shutil.rmtree(stage)
            stage.mkdir(parents=True)
            for save in G.stage_dirs(str(work/source)):
                shutil.copytree(work/source/save, stage/save)
        out = work/name
        out.mkdir(parents=True, exist_ok=True)
        trace = out/'cursor.jsonl'
        rc = R.run_port(G.rec_of(name), str(out),
                        ['--hd-cursor', '--cursor-log', str(trace.resolve())],
                        stage=str(stage) if stage else None, quiet=True)
        assert rc == 0, (name, rc, out/'port.log')
        gold = G.load_golden(name)
        checkpoints = R.read_dump(str(out))
        if gold:
            assert len(checkpoints)==len(gold['checkpoints']), name
            for i, (ck, expected) in enumerate(zip(checkpoints, gold['checkpoints'])):
                assert G.header(ck)==expected['ck'], (name,i,'header')
                actual=G.canon(ck)
                keys=(set(actual)|set(expected['secs']))-ignored
                bad=[k for k in sorted(keys) if actual.get(k)!=expected['secs'].get(k)]
                assert not bad, (name,i,bad)
            assert G.save_hashes(str(out))==gold['saves'], (name,'saves')
        rows=[json.loads(s) for s in trace.read_text().splitlines()]
        visible=[r for r in rows if r['visible']]
        assert visible, (name,'no cursor published')
        results[name]={'checkpoints':len(checkpoints), 'golden_compared':bool(gold),
            'pictures':sorted({r['picture'] for r in visible}),
            'paths':sorted({r['path'] for r in visible}),
            'modes':sorted({r['mode'] for r in visible})}
        print(name, results[name], flush=True)

    for name in G.SESSIONS:
        run(name)
    report={'status':'PASS', 'sessions':results, 'public_sessions':len(G.SESSIONS),
        'ignored_presentation_sections':sorted(ignored),
        'compared':'all other canonical sections, headers and saved file hashes',
        'dos_execution':False}
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')


if __name__=='__main__':
    main()
