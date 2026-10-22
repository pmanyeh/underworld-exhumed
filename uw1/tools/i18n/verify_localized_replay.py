#!/usr/bin/env python3
"""Exercise zh-TW lookup during the public native replays, against DOS goldens.

Uses upstream's Native verifier with extra documented CLI options, without
rebuilding a DOS reference. Original inputs,
assertions and goldens are unchanged. Work directories are separate from the
ordinary English verify run; every trace hit is checked against the master
dataset. Each session also writes the Phase 9 QA trace (port/i18n-trace.jsonl).
"""
import argparse
import json
import os
from pathlib import Path
import sys

from l10n import static_targets


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", required=True, type=Path)
    parser.add_argument("--catalog", required=True, type=Path)
    parser.add_argument("--master", required=True, type=Path)
    parser.add_argument("--report", required=True, type=Path)
    args = parser.parse_args()
    for name in ("config", "catalog", "master", "report"):
        setattr(args, name, getattr(args, name).resolve())
    sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "exhume/tools"))
    os.environ["EXHUME_CONFIG"] = str(args.config)
    import replay as R
    import golden as G

    G.VWORK = os.path.join(R.OUT, "verify-i18n")
    original = R.run_port

    def run_port(rec, out, extra=(), **kwargs):
        options = list(extra) + ["--locale", "zh-TW", "--i18n-file", str(args.catalog),
                                 "--i18n-log", os.path.join(out, "i18n.jsonl"),
                                 "--i18n-trace", os.path.join(out, "i18n-trace.jsonl")]
        return original(rec, out, options, **kwargs)

    R.run_port = run_port
    # Native-only PoC: cmd_verify otherwise tries a Borland/DOS rebuild just
    # to refresh a reference hash. Keep committed goldens and every Native
    # assertion, but do not invoke that out-of-scope build/toolchain.
    def no_dos_build(*_args, **_kwargs):
        print("Native-only PoC: using committed goldens; no DOS reference rebuild", flush=True)

    R.build = no_dos_build
    result = G.cmd_verify(G.SESSIONS, jobs=None)
    if result:
        raise SystemExit(result)
    expected = static_targets(args.master)
    sessions, all_keys = {}, set()
    for name in G.SESSIONS:
        trace = Path(G.VWORK) / name / "port/i18n.jsonl"
        if not trace.exists():
            raise AssertionError(f"locale was not initialized: {name}")
        rows = [json.loads(line) for line in trace.read_text(encoding="utf-8").splitlines()]
        keys = set()
        for row in rows:
            key = row["block"], row["index"]
            if key in keys or row["text"] != expected[key]:
                raise AssertionError(f"bad/duplicate translation hit in {name}: {key}")
            keys.add(key)
        sessions[name] = [list(k) for k in sorted(keys)]
        all_keys |= keys
    # Public talk selects option 1 three times: opening, exploration, advice.
    # It ends during the long advice/MORE prompt, before options 17/18 appear.
    # Those keys, and all other branches, are covered by the 52-key API test.
    required = {(3651, i) for i in (3, 4, 5, 6, 7, 10, 11, 12, 16)}
    talk_keys = {tuple(k) for k in sessions["talk"]}
    if not required <= talk_keys:
        raise AssertionError(f"Bragit path missing: {required - talk_keys}")
    report = {"status": "PASS", "locale": "zh-TW", "sessions": sessions,
              "session_count": len(G.SESSIONS), "distinct_translation_hits": len(all_keys),
              "translation_text_checked_against_master": True,
              "qa_traces": [str(Path(G.VWORK) / n / "port/i18n-trace.jsonl") for n in G.SESSIONS],
              "legacy_state_and_saves": "identical to committed DOS goldens",
              "dos_reference_rebuild": "NOT APPLICABLE - Native-only PoC",
              "display": "legacy English path; UTF-8 lookup verified via JSONL"}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
