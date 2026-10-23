#!/usr/bin/env python3
"""How each conversation string is used by its BABL script (P10B's consumer audit).

A linear sweep of each script in the player's CNV.ARK with a small symbolic stack:
immediates, frame and global addresses and the values stored in them, so that a string id
is followed from PUSHI through STO and arrays (OFFSET) to the instruction or built-in that
consumes it. Each string of the conversation's block gets one use:

  compare  compare, contains, find or STRCMP match it (usually against the player's typed
           answer from babl_ask): translating it would break the match
  compose  append, copy, length or val build or measure a string from it
  display  SAY, babl_menu, babl_fmenu, print, say, respond or sex show it
  none     no consumer found (reached some other way, or not used)

The built-ins read their arguments as args[-1], args[-2]... (BABL.C), so each has its own
arity here; the count a script pushes before CALLI is not reliable (babl_menu's is 0).
A compare whose argument is a function parameter (a negative frame slot) would hide the
string from this sweep; such calls are reported, and the shipped scripts have none.

--input-keys writes the compared strings as the list l10n.py reads (i18n/input-keys.tsv):
keys and source hashes, no text. --report writes the per-conversation counts.
"""
import argparse
import collections
import json
from pathlib import Path
import struct

import extract_strings
import l10n

(NOP, ADD, MUL, SUB, DIV, MOD, OR, AND, NOT, TSTGT, TSTGE, TSTLT, TSTLE, TSTEQ, TSTNE, JMP,
 BEQ, BNE, BRA, CALL, CALLI, RET, PUSHI, PUSHI_EFF, POP, SWAP, PUSHBP, POPBP, SPTOBP, BPTOSP,
 ADDSP, FETCHM, STO, OFFSET, START, SAVE_REG, PUSH_REG, STRCMP, EXIT, SAY, RESPOND, NEG) = range(42)
OPERAND = {JMP, BEQ, BNE, BRA, CALL, CALLI, PUSHI, PUSHI_EFF}
BINARY = {ADD, MUL, SUB, DIV, MOD, OR, AND, TSTGT, TSTGE, TSTLT, TSTLE, TSTEQ, TSTNE}
DISPLAY = {"babl_menu", "babl_fmenu", "print", "say", "respond", "sex"}
COMPARE = {"compare", "contains", "find"}
COMPOSE = {"append", "copy", "length", "val"}
ARITY = {"babl_menu": 1, "babl_fmenu": 2, "compare": 2, "contains": 2, "find": 2, "print": 1, "say": 1,
         "respond": 1, "sex": 2, "append": 2, "copy": 2, "length": 1, "val": 1}
UNKNOWN = ("?",)


def scripts(cnv):
    """(header, code words, {function index: name}) for each conversation in CNV.ARK."""
    count, = struct.unpack_from("<H", cnv, 0)
    offsets = struct.unpack_from(f"<{count}I", cnv, 2)
    for header in extract_strings.conversation_headers(cnv):
        chunk = cnv[offsets[header["slot"]]:]
        pos = 16
        for _ in header["imports"]:
            size, = struct.unpack_from("<H", chunk, pos)
            pos += 2 + size + 8
        code = struct.unpack_from(f"<{header['code_words']}H", chunk, pos)
        funcs = {i["index"]: i["name"] for i in header["imports"] if i["kind"] == 0x111}
        yield header, code, funcs


def sweep(code, funcs, nstrings):
    """{string id: {(use, consumer)}} and the compares made on function parameters."""
    uses = collections.defaultdict(set)
    on_parameters = []
    stack, frame, glob = [], {}, {}

    def pop():
        return stack.pop() if stack else UNKNOWN

    def address(entry):
        return ("glob", entry[1]) if entry[0] == "imm" else entry

    def stored(entry, many=False):
        store = {"frame": frame, "glob": glob}.get(entry[0])
        if store is None:
            return []
        if not many:
            return [store[entry[1]]] if store.get(entry[1]) is not None else []
        out, slot = [], entry[1]
        while store.get(slot) and len(out) < 64:      # a 0-terminated array
            out.append(store[slot])
            slot += 1
        return out

    def use(sid, how):
        if isinstance(sid, int) and 0 < sid < nstrings:
            uses[sid].add(how)

    pc = 0
    while pc < len(code):
        op = code[pc]
        arg = code[pc + 1] if op in OPERAND and pc + 1 < len(code) else None
        if op == PUSHI:
            stack.append(("imm", arg))
        elif op == PUSHI_EFF:
            stack.append(("frame", arg - 0x10000 if arg >= 0x8000 else arg))
        elif op == FETCHM:
            got = stored(address(pop()))
            stack.append(("val", got[0]) if got else UNKNOWN)
        elif op == STO:
            value, where = pop(), address(pop())
            store = {"frame": frame, "glob": glob}.get(where[0])
            if store is not None:
                store[where[1]] = value[1] if value[0] in ("imm", "val") else None
        elif op == OFFSET:
            where, index = address(pop()), pop()      # the address on top, the index below
            stack.append((where[0], where[1] + index[1] - 1)
                         if where[0] in ("frame", "glob") and index[0] == "imm" else UNKNOWN)
        elif op == CALLI:
            name = funcs.get(arg, f"#{arg}")
            n = ARITY.get(name, 0)
            args = [address(a) for a in stack[-1 - n:-1]] if n and len(stack) > n else []
            how = ("display" if name in DISPLAY else "compare" if name in COMPARE
                   else "compose" if name in COMPOSE else None)
            if how == "compare" and any(a[0] == "frame" and a[1] < 0 for a in args):
                on_parameters.append(pc)
            if how:
                for a in args:
                    for sid in stored(a, many=name in ("babl_menu", "babl_fmenu")):
                        use(sid, (how, name))
            if stack:
                stack[-1] = UNKNOWN                   # the result replaces the count
        elif op == SAY:
            value = pop()
            use(value[1] if value[0] in ("imm", "val") else None, ("display", "SAY"))
        elif op == STRCMP:
            for value in (pop(), pop()):
                use(value[1] if value[0] in ("imm", "val") else None, ("compare", "STRCMP"))
            stack.append(UNKNOWN)
        elif op in BINARY:
            pop(); pop(); stack.append(UNKNOWN)
        elif op in (NOT, NEG):
            pop(); stack.append(UNKNOWN)
        elif op in (BEQ, BNE, POP, POPBP):
            pop()
        elif op == SWAP and len(stack) > 1:
            stack[-1], stack[-2] = stack[-2], stack[-1]
        elif op in (PUSHBP, PUSH_REG):
            stack.append(UNKNOWN)
        elif op == SPTOBP:
            frame = {}
        elif op == ADDSP:
            n = pop()
            stack.extend([UNKNOWN] * (n[1] if n[0] == "imm" and 0 <= n[1] < 512 else 0))
        elif op in (RET, EXIT):
            stack = []
        pc += 2 if op in OPERAND else 1
    return uses, on_parameters


def audit(inventory, cnv):
    report = {}
    for header, code, funcs in scripts(cnv):
        block = header["resolved_block"]
        nstrings = 1 + max((i for b, i in inventory.entries if b == block), default=-1)
        uses, on_parameters = sweep(code, funcs, nstrings)
        rows = {}
        for index in range(nstrings):
            kinds = {k for k, _ in uses.get(index, ())}
            rows[index] = ("compare" if "compare" in kinds else "compose" if "compose" in kinds
                           else "display" if "display" in kinds else "none")
        report[block] = {"slot": header["slot"], "rows": rows, "compare_on_parameters": on_parameters,
                         "compared_by": {i: sorted(c for k, c in uses[i] if k == "compare")
                                         for i in uses if any(k == "compare" for k, _ in uses[i])}}
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--pak", type=Path, required=True)
    parser.add_argument("--cnv", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    parser.add_argument("--input-keys", type=Path)
    args = parser.parse_args()
    inventory = l10n.Inventory.load(args.pak, args.cnv)
    report = audit(inventory, args.cnv.read_bytes())
    totals = collections.Counter(use for r in report.values() for use in r["rows"].values())
    hidden = {block: r["compare_on_parameters"] for block, r in report.items() if r["compare_on_parameters"]}
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        summary = {"totals": dict(totals), "compare_on_parameters": hidden,
                   "conversations": {str(b): {"slot": r["slot"], **collections.Counter(r["rows"].values())}
                                     for b, r in sorted(report.items())}}
        args.report.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8", newline="\n")
    if args.input_keys:
        lines = ["block\tindex\tsource_hash\treason"]
        for block, r in sorted(report.items()):
            for index, by in sorted(r["compared_by"].items()):
                source = inventory.entries.get((block, index), "")
                if source:
                    lines.append(f"{block}\t{index}\t{l10n.source_hash(source)}\t"
                                 f"BABL {'/'.join(by)} in conversation slot {r['slot']}")
        args.input_keys.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    print(f"{sum(totals.values())} conversation strings: " + ", ".join(f"{k} {v}" for k, v in sorted(totals.items())))
    if hidden:
        print(f"WARNING: compares on function parameters, strings not traced: {hidden}")
    return 1 if hidden else 0


if __name__ == "__main__":
    raise SystemExit(main())
