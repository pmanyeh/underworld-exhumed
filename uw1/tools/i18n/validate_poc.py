#!/usr/bin/env python3
"""Validate Phase 1 translation identities and required control/template tokens."""
import argparse
from collections import Counter
import csv
from pathlib import Path
import re

from extract_strings import StringsPak, conversation_headers, unescape
from scan_literals import inventory


def template_tokens(text):
    # Preserve the complete recursive BABL index syntax. Fail closed on an
    # unfamiliar marker instead of treating it as ordinary translatable text.
    tokens, i = [], 0

    def variable(pos, integer=False):
        if pos >= len(text) or text[pos] not in "GSPC":
            raise ValueError("unknown BABL variable kind")
        kind = text[pos]
        pos += 1
        if kind != "C":
            if pos >= len(text) or text[pos] not in ("I" if integer else "IS"):
                raise ValueError("unknown BABL variable type")
            pos += 1
        match = re.match(r"-?\d+", text[pos:])
        if not match:
            raise ValueError("missing BABL variable number")
        pos += len(match[0])
        if pos < len(text) and text[pos] in "GSPC":
            pos = variable(pos, True)
        return pos

    while i < len(text):
        if text[i] != "@":
            i += 1
            continue
        start = i
        i += 1
        i = i + 1 if text[i:i + 1] == "@" else variable(i)
        tokens.append(text[start:i])
    return Counter(tokens)


def controls(text):
    return re.findall(r"\\[pPm0-6]", text)


def validate_tokens(source, target):
    if template_tokens(source) != template_tokens(target):
        raise ValueError("BABL templates differ")
    if controls(source) != controls(target):
        raise ValueError("game controls differ or changed order")
    if source.count("\n") != target.count("\n"):
        raise ValueError("explicit newline count differs")


def validate(poc, pak, cnv, literals):
    headers = {r["slot"]: r for r in conversation_headers(cnv)}
    if headers[67]["resolved_block"] != 3651:
        raise ValueError("this PoC expects Bragit slot 67 / string block 3651")
    literal_map = {r["source_key"]: r for r in literals}
    seen, translated, counts = set(), set(), Counter()
    with poc.open(encoding="utf-8", newline="") as stream:
        for raw in csv.DictReader(stream, delimiter="\t"):
            row = {k: unescape(v) for k, v in raw.items()}
            key = row["source_key"]
            if key in seen:
                raise ValueError(f"duplicate key {key}")
            seen.add(key)
            if not row["zh_tw"].strip():
                raise ValueError(f"empty translation {key}")
            if row["kind"] == "pak":
                b, i = int(row["block"]), int(row["index"])
                if b in (0, 0x7C, 0x7D) or not 0 <= i <= 511:
                    raise ValueError(f"unresolved/dynamic/unaddressable key {key}")
                if b not in pak.blocks or i >= pak.blocks[b][1]:
                    raise ValueError(f"nonexistent key {key}")
                if key != f"pak:{b}:{i}":
                    raise ValueError(f"inconsistent key {key}")
                source = pak.read(b, i).decode("cp437")
                if b == 3651:
                    translated.add(i)
                elif (b, i) not in {(1, 2), (1, 5), (7, 83)}:
                    raise ValueError("outside Phase 1 translation scope")
            elif row["kind"] == "literal":
                if row["block"] or row["index"]:
                    raise ValueError("literal must use a separate namespace")
                literal = literal_map[key]
                if literal["classification"] != "PLAYER_VISIBLE" or literal["source_en"] != "[MORE]":
                    raise ValueError("unexpected hard-coded sample")
                source = literal["source_en"]
            else:
                raise ValueError("unknown key kind")
            if source != row["source_en"]:
                raise ValueError(f"source drift {key}")
            try:
                validate_tokens(source, row["zh_tw"])
            except ValueError as error:
                raise ValueError(f"{key}: {error}") from error
            counts[row["kind"]] += 1
    expected = {i for i in range(pak.blocks[3651][1]) if pak.read(3651, i)}
    if translated != expected:
        raise ValueError(f"Bragit key coverage differs: {translated ^ expected}")
    if counts != {"pak": len(expected) + 3, "literal": 1}:
        raise ValueError(f"PoC scope differs: {counts}")
    return {"status": "PASS", "translations": sum(counts.values()),
            "bragit_nonempty_static_strings": len(expected), "counts": dict(counts)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pak", type=Path)
    parser.add_argument("cnv", type=Path)
    parser.add_argument("poc", type=Path)
    args = parser.parse_args()
    print(validate(args.poc, StringsPak(args.pak.read_bytes()), args.cnv.read_bytes(), inventory()))


if __name__ == "__main__":
    main()
