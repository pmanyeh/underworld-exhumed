#!/usr/bin/env python3
"""BABL template/control token rules, and the PoC subset check over the master dataset."""
import argparse
from collections import Counter
from pathlib import Path
import re

from extract_strings import StringsPak, conversation_headers
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


def validate(master, literal_rows, pak, cnv, literals):
    """The 53 PoC translations survive in the Phase 9 master/literal datasets."""
    from l10n import check_literals, check_master
    headers = {r["slot"]: r for r in conversation_headers(cnv)}
    if headers[67]["resolved_block"] != 3651:
        raise ValueError("this PoC expects Bragit slot 67 / string block 3651")
    problems = check_master(master)[0] + check_literals(literal_rows, literals)
    for row in master:
        b, i = int(row["block"]), int(row["index"])
        if pak.read(b, i).decode("cp437") != row["source"]:
            problems.append(f"source drift {b}:{i}")
    if problems:
        raise ValueError("\n".join(problems))
    translated = {(int(r["block"]), int(r["index"])) for r in master if r["target_zh_TW"]}
    bragit = {(3651, i) for i in range(pak.blocks[3651][1]) if pak.read(3651, i)}
    poc = bragit | {(1, 2), (1, 5), (7, 83)}
    if not poc <= translated:
        raise ValueError(f"PoC keys missing: {sorted(poc - translated)}")
    more = [r for r in literal_rows if r["source"] == "[MORE]" and r["target_zh_TW"]]
    if len(more) != 1:
        raise ValueError("PoC [MORE] literal sample missing")
    return {"status": "PASS", "poc_translations": len(poc) + 1,
            "bragit_nonempty_static_strings": len(bragit), "master_rows": len(master),
            "literal_rows": len(literal_rows)}


def main():
    from l10n import LITERALS, MASTER, load_literals, load_master
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pak", type=Path)
    parser.add_argument("cnv", type=Path)
    parser.add_argument("--master", type=Path, default=MASTER)
    parser.add_argument("--literals", type=Path, default=LITERALS)
    args = parser.parse_args()
    print(validate(load_master(args.master), load_literals(args.literals),
                   StringsPak(args.pak.read_bytes()), args.cnv.read_bytes(), inventory()))


if __name__ == "__main__":
    main()
