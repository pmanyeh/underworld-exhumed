#!/usr/bin/env python3
"""Cross-check every extracted string against the original native C decoder.

Run after `make port`, with the same compiler/library environment. Imports the
pinned upstream build configuration; does not change it or any game source.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from extract_strings import StringsPak


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe", type=Path)
    parser.add_argument("pak", type=Path)
    parser.add_argument("--report", required=True, type=Path)
    parser.add_argument("--catalog", type=Path, help="also test the Phase 2 hook and ring lifetime")
    args = parser.parse_args()
    args.report = args.report.resolve()
    if args.catalog:
        args.catalog = args.catalog.resolve()
    exe, pak_path = args.exe.resolve(), args.pak.resolve()
    sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "exhume/tools"))
    import portbuild
    import portcfg
    import portcheck
    import sources

    cfg, port = portbuild.CFG, portbuild.P
    os.chdir(cfg.root)
    out = Path(cfg.build) / "i18n"
    out.mkdir(parents=True, exist_ok=True)
    probe = out / ("native-probe.exe" if os.name == "nt" else "native-probe")
    obj = out / "native-probe.o"
    cc = portcheck.host_cc()
    subprocess.run([cc] + portbuild.PORT_FLAGS + portcheck.layout_flags(cc) +
                   ["-c", str(Path(__file__).with_name("native_probe.c")), "-o", str(obj)], check=True)
    objects = [os.path.join(port.out, sources.stem(p) + ".o") for p in portcfg.game_sources(cfg)]
    for path in portbuild.port_sources():
        if path.endswith(os.sep + portcfg.fuzz(cfg).main.replace("/", os.sep)):
            continue
        objects.append(portbuild.port_object(port.out, path))
    _, libraries, extra = portbuild.sound_deps()
    objects += [os.path.join(port.out, "deps", os.path.basename(p)[:-2] + ".o") for p in extra]
    threads = [] if sys.platform == "darwin" else ["-pthread"]
    subprocess.run([cc, "-o", str(probe), str(obj)] + objects +
                   portbuild.pkg_config("--libs") + libraries + threads, check=True)
    pak = StringsPak(pak_path.read_bytes())
    keys = [(r["block"], r["index"]) for r in pak.rows()]
    keys += [(0xBEEF, 0), (1, 65535)]  # Both missing-key paths in the original.
    command = [str(probe), str(exe), str(pak_path)]
    trace = out / "native-hook.jsonl"
    if args.catalog:
        command += [str(args.catalog), str(trace)]
    result = subprocess.run(command,
                            input="".join(f"{b} {i}\n" for b, i in keys),
                            text=True, capture_output=True, check=True)
    actual = {}
    for line in result.stdout.splitlines():
        b, i, value = line.split("\t")
        actual[int(b), int(i)] = bytes.fromhex(value)
    if set(actual) != set(keys):
        raise AssertionError("native probe returned a different key set")
    failures = [(b, i) for b, i in keys if actual[b, i] != pak.read(b, i)]
    if failures:
        raise AssertionError(f"native decoder disagrees: {failures[:20]}")
    report = {"status": "PASS", "strings_compared": len(keys) - 2,
              "missing_keys_compared": 2, "blocks": len(pak.blocks),
              "pak_sha256": hashlib.sha256(pak.data).hexdigest(),
              "decoder": "unchanged native src/ui/GAMESTRN.C read_string",
              "block0_resolution": "PASS: get_string(3), context 3651, compared to read_string(3651,3)",
              "dynamic_blocks": "PASS: make_string/get_string pointer identity for 0x7C and 0x7D",
              "categories": sorted(set(r["category"] for r in pak.rows()))}
    if args.catalog:
        hits = [json.loads(line) for line in trace.read_text(encoding="utf-8").splitlines()]
        if {(r["block"], r["index"]) for r in hits} != {(3651, 3), (1, 2), (1, 5)}:
            raise AssertionError(f"unexpected hook trace keys: {hits}")
        report["runtime_hook"] = {"status": "PASS", "trace": hits,
                                  "checks": ["resolved Bragit block", "untranslated cutscene fallback",
                                             "both dynamic pointers bypass lookup", "eight-slot ring lifetime",
                                             "legacy strings unchanged", "UTF-8 pointer survives ring reuse"]}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
