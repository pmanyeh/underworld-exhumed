#!/usr/bin/env python3
"""Compile reviewed UTF-8 TSV to a deterministic, bounded native lookup catalog.

The catalog is a generated build artifact; do not edit or commit it. Literal
keys are reserved for the later hard-coded UI phase and are not packed here.
"""
import argparse
import csv
from pathlib import Path
import struct

from extract_strings import unescape
from validate_poc import validate_tokens

MAGIC = b"UW1LOC1\0"
MAX_ENTRIES, MAX_TARGET, MAX_CATALOG = 4096, 16384, 4 * 1024 * 1024


def pack(path):
    entries, seen, literals = [], set(), 0
    with path.open(encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames != ["kind", "block", "index", "source_key", "role", "source_en", "zh_tw", "note"]:
            raise ValueError("unexpected TSV schema")
        for raw in reader:
            if None in raw or any(v is None for v in raw.values()):
                raise ValueError("wrong column count")
            row = {k: unescape(v) for k, v in raw.items()}
            if row["source_key"] in seen:
                raise ValueError("duplicate source key")
            seen.add(row["source_key"])
            if row["kind"] == "literal":
                if row["block"] or row["index"] or not row["source_key"].startswith("literal:"):
                    raise ValueError("invalid literal identity")
                literals += 1
                continue
            if row["kind"] != "pak":
                raise ValueError("unknown key kind")
            block, index = int(row["block"]), int(row["index"])
            if not 0 < block <= 0x7FFF or block in (0x7C, 0x7D) or not 0 <= index < 512:
                raise ValueError("unresolved, dynamic or unaddressable key")
            if row["source_key"] != f"pak:{block}:{index}":
                raise ValueError("inconsistent PAK identity")
            validate_tokens(row["source_en"], row["zh_tw"])
            source, target = row["source_en"].encode("cp437"), row["zh_tw"].encode("utf-8")
            if not 0 < len(source) <= 511 or not 0 < len(target) <= MAX_TARGET:
                raise ValueError("empty or over-limit text")
            if b"\0" in source or b"\0" in target:
                raise ValueError("embedded NUL")
            entries.append((block, index, source, target))
    entries.sort()
    if not 0 < len(entries) <= MAX_ENTRIES:
        raise ValueError("catalog entry limit")
    result = bytearray(MAGIC + struct.pack("<I", len(entries)))
    for block, index, source, target in entries:
        result += struct.pack("<HHII", block, index, len(source), len(target))
        result += source + b"\0" + target + b"\0"
    if len(result) > MAX_CATALOG:
        raise ValueError("catalog byte limit")
    return bytes(result), len(entries), literals


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tsv", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    data, count, literals = pack(args.tsv)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"{count} static entries, {len(data)} bytes; {literals} literal reserved -> {args.output}")


if __name__ == "__main__":
    main()
