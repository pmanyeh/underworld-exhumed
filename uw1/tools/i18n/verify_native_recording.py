#!/usr/bin/env python3
"""Verify a fresh Native recording twice, optionally also in the sanitizer build.

The output directory must be new. Original recordings, original homes and stage
saves must be outside that output directory.
No DOS tools run, no golden changes and no additional section masks are used.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import sys


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', required=True, type=Path)
    parser.add_argument('--recording', required=True, type=Path)
    parser.add_argument('--original', required=True, type=Path)
    parser.add_argument('--stage', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--modern', action='store_true')
    parser.add_argument('--catalog', type=Path)
    parser.add_argument('--font', type=Path)
    parser.add_argument('--sanitize', action='store_true', help='also replay the port-debug build')
    args = parser.parse_args()
    if args.modern and (not args.catalog or not args.font):
        parser.error('--modern requires --catalog and --font')
    out = args.output.resolve()
    for source in (args.recording, args.original, args.stage, args.config, args.catalog, args.font):
        if source and source.resolve().is_relative_to(out):
            parser.error('recording, original, stage and dependencies must be outside --output')
    out.mkdir(parents=True, exist_ok=False)
    os.environ['EXHUME_CONFIG'] = str(args.config.resolve())
    sys.path.insert(0, str(Path(__file__).resolve().parents[3] / 'exhume/tools'))
    import replay as R
    import golden as G

    streams, stop = R.read_log(str(args.recording))
    assert not streams.get('CHEAT'), 'P8 gameplay must not contain cheats'
    original = R.read_dump(str(args.original))
    assert original and original[-1]['events'] <= stop
    extra = ['--sound', '0,0', '--locale', 'en']
    if args.modern:
        extra = ['--sound', '0,0', '--locale', 'zh-TW', '--hd-scroll',
                 '--i18n-file', str(args.catalog.resolve()), '--font', str(args.font.resolve())]
    results = []
    reference = None
    reference_saves = G.save_hashes(str(args.original))
    for label in ('replay1', 'replay2') + (('sanitizer',) if args.sanitize else ()):
        folder = out / label
        folder.mkdir()
        env = {'EXHUME_TEST_CHEATS': None, 'ASAN_OPTIONS': 'halt_on_error=1:detect_leaks=0',
               'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
        rc = R.run_port(str(args.recording.resolve()), str(folder),
                        extra + (['--debug'] if label == 'sanitizer' else []),
                        stage=str(args.stage.resolve()) if args.stage else None, quiet=True, env=env)
        assert rc == 0, (label, rc, folder/'port.log')
        log = (folder/'port.log').read_text(encoding='utf-8')
        assert not any(s in log for s in ('runtime error:', 'ERROR: AddressSanitizer', 'DESYNC')), log
        checkpoints = R.read_dump(str(folder))
        terminal_extra = int(original[-1]['kind'] != 4)
        assert len(checkpoints) == len(original) + terminal_extra, (label, len(checkpoints), len(original))
        assert checkpoints[-1]['kind'] == 4 and checkpoints[-1]['events'] == stop, (label, 'terminal')
        for i, (actual, expected) in enumerate(zip(checkpoints, original)):
            assert G.header(actual) == G.header(expected), (label, i, 'header')
            assert G.canon(actual) == G.canon(expected), (label, i, 'canonical sections')
        current = [(G.header(ck), G.canon(ck)) for ck in checkpoints]
        if reference is not None:
            assert current == reference, (label, 'including terminal')
        reference = current
        assert G.save_hashes(str(folder)) == reference_saves, (label, 'saved files')
        results.append({'run': label, 'status': 'PASS', 'checkpoints': len(checkpoints),
                        'all_canonical_sections_and_headers': True, 'saves_identical': True})
        print(label, 'PASS', len(checkpoints), 'checkpoints; all canonical sections, headers and saves', flush=True)
    report = {'status': 'PASS', 'DOS_executed': False, 'recording_sha256': digest(args.recording),
              'recording_version': args.recording.read_bytes()[4], 'cheats': [],
              'original_checkpoints': len(original), 'stop_event': stop,
              'expected_extra_terminal': terminal_extra, 'additional_section_masks': [],
              'goldens_modified': False, 'runs': results,
              'sanitizer_scope': 'port-debug; use the sanitizers specified when building it; third-party DLLs excluded'}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()
