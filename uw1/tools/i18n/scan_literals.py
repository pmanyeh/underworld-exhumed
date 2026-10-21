#!/usr/bin/env python3
"""Conservative C literal inventory, including literals passed via named arrays.

This is lexical triage, not preprocessor/data-flow analysis. UNKNOWN is a
review queue, not permission to translate. Keep original C spelling as evidence.
"""
import argparse
from collections import Counter
import hashlib
from pathlib import Path
import re
import sys

from extract_strings import write_tsv

REPO = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(REPO / "exhume/tools"))
from cparse import strip_comments

TOKEN = re.compile(r'''(?:u8|[LuU])?"(?:\\[\s\S]|[^"\\])*"|'(?:\\[\s\S]|[^'\\])*'|[A-Za-z_]\w*|[^\s]''')
VISIBLE = {"string_to_screen", "scroll_print", "scroll_print_uw2", "draw_string",
           "grText", "grString", "print_string"}
DEBUG = {"uw_dprintf", "port_log", "DBG", "debug_printf"}


def decode_c(raw):
    """Decode narrow C escapes; the original spelling is also retained."""
    body = raw[raw.index('"') + 1:-1]
    result, i = [], 0
    simple = dict(zip("abfnrtv\\\"'?", "\a\b\f\n\r\t\v\\\"'?"))
    while i < len(body):
        c = body[i]
        i += 1
        if c != "\\":
            result.append(c)
            continue
        c = body[i]
        i += 1
        if c == "\n":
            continue
        if c in simple:
            result.append(simple[c])
        elif c in "01234567":
            end = i
            while end < min(i + 2, len(body)) and body[end] in "01234567":
                end += 1
            result.append(chr(int(c + body[i:end], 8) & 255))
            i = end
        elif c == "x":
            match = re.match(r"[0-9a-fA-F]+", body[i:])
            if not match:
                raise ValueError("empty C hex escape")
            result.append(chr(int(match[0], 16) & 255))
            i += len(match[0])
        else:
            # Unknown/wide escape: do not silently turn it into another value.
            result.append("\\" + c)
    return "".join(result)


def classify(path, call, text, directive=False):
    lower = path.lower()
    if "/tools/" in lower or "/tests/" in lower or "test_" in Path(lower).name:
        return "TEST_ONLY", "test/tool source; not linked into the game"
    if directive:
        return "UNKNOWN", "preprocessor operand; not display text"
    if call in DEBUG:
        return "DEBUG_ONLY", "explicit diagnostic sink"
    if call in VISIBLE:
        return "PLAYER_VISIBLE", "direct game text sink"
    if call == "plat_message":
        return "PORT_UI", "native message dialog"
    if lower.endswith("/ui/settings.c"):
        return "PORT_UI", "native settings source; review paths/format fragments separately"
    # Named strings in this file include originals for the game's scroll UI.
    if path == "uw1/src/ui/SCROLLIO.C" and text in ("[MORE]", "Yes", "No"):
        return "PLAYER_VISIBLE", "scroll UI label"
    return "UNKNOWN", "needs context/data-flow review (may be a path, format or UI text)"


def scan_source(path, source):
    clean = strip_comments(source)
    tokens = list(TOKEN.finditer(clean))
    stack, counts, result, i = [], Counter(), [], 0
    while i < len(tokens):
        token, pos = tokens[i][0], tokens[i].start()
        if token == "(":
            previous = tokens[i - 1][0] if i else ""
            stack.append(previous if re.fullmatch(r"[A-Za-z_]\w*", previous) else "")
        elif token == ")":
            if stack:
                stack.pop()
        elif re.match(r'^(?:u8|[LuU])?"', token):
            raw, pieces = [token], [decode_c(token)]
            while i + 1 < len(tokens) and re.match(r'^(?:u8|[LuU])?"', tokens[i + 1][0]):
                i += 1
                raw.append(tokens[i][0])
                pieces.append(decode_c(tokens[i][0]))
            text = "".join(pieces)
            call = next((name for name in reversed(stack) if name), "initializer")
            line_start = clean.rfind("\n", 0, pos) + 1
            kind, reason = classify(path, call, text, clean[line_start:pos].lstrip().startswith("#"))
            digest = hashlib.sha256(text.encode("utf-8")).hexdigest()[:16]
            base = f"literal:{path}:{call}:{digest}"
            counts[base] += 1
            result.append({"source_key": f"{base}:{counts[base]}", "path": path,
                           "line": clean.count("\n", 0, pos) + 1, "call": call,
                           "classification": kind, "reason": reason,
                           "source_en": text, "source_c": " ".join(raw)})
        i += 1
    return result


def inventory(repo=REPO):
    paths = []
    for directory in ("uw1/src", "exhume/runtime/port", "uw1/tools/i18n"):
        paths += [p for p in (repo / directory).rglob("*") if p.suffix.lower() in (".c", ".h")]
    rows = []
    for path in sorted(paths, key=lambda p: p.relative_to(repo).as_posix()):
        rows += scan_source(path.relative_to(repo).as_posix(), path.read_text(encoding="utf-8"))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    rows = inventory()
    write_tsv(args.output, rows, ["source_key", "path", "line", "call", "classification", "reason", "source_en", "source_c"])
    print(f"{len(rows)} literals: {dict(sorted(Counter(r['classification'] for r in rows).items()))}")


if __name__ == "__main__":
    main()
