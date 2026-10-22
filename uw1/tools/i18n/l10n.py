#!/usr/bin/env python3
"""Phase 9 localization production pipeline for UW1 zh-TW.

Identity is the resolved (block, index); source text is only a drift check,
context and translator reference. Legal game data is read locally. Full
English inventories and translator exports stay in ignored build/; only the
master dataset, literal namespace, count-only reports and fingerprints are
committed. Standard library only.

  inventory  refresh STRINGS.PAK/CNV.ARK inventory, compare the baseline
  validate   master + literals against the current inventory
  normalize  rewrite master/literals in canonical form
  build      validate, then pack a deterministic runtime catalog
  coverage   coverage matrix, optionally with runtime QA traces
  export     translator/QA workbook (key, source, target, context, hits)
  import     merge an edited workbook back into the master dataset
"""
import argparse
from collections import Counter, defaultdict
import csv
import hashlib
import io
import json
from pathlib import Path
import struct
import sys

from extract_strings import StringsPak, conversation_headers, escape, unescape, CATEGORIES
from validate_poc import controls, template_tokens, validate_tokens

UW1 = Path(__file__).resolve().parents[2]
MASTER = UW1 / "i18n/zh-TW/master.tsv"
LITERALS = UW1 / "i18n/zh-TW/literals.tsv"
BASELINE = UW1 / "i18n/inventory-baseline.json"

MASTER_FIELDS = ["block", "index", "category", "source", "target_zh_TW", "status", "context", "notes", "flags"]
LITERAL_FIELDS = ["key", "source", "target_zh_TW", "status", "context", "notes", "flags"]
STATUSES = ("untranslated", "draft", "reviewed", "approved", "blocked")
PACKED = {"draft": 0, "reviewed": 1, "approved": 2}
FLAGS = ("dynamic-template", "grammar-sensitive", "object-name", "control-code",
         "hardcoded-related", "showcase", "needs-context")
# Derived from the source text; normalize adds them, validate requires them.
DERIVED = ("dynamic-template", "object-name", "control-code")
DYNAMIC_BLOCKS = (0x7C, 0x7D)
# Same bounds as src/port/i18n/localization.c.
MAGIC = b"UW1LOC1\0"
MAX_ENTRIES, MAX_TARGET, MAX_CATALOG = 32768, 16384, 16 * 1024 * 1024
HANDLING = ("direct", "template", "object-name", "grammar", "bitmap-ui", "hardcoded",
            "debug-report", "empty", "unaddressable")
SPECIAL = set(HANDLING) - {"direct", "empty", "unaddressable"}
EVENTS = ("hit", "template", "miss", "drift", "dynamic", "invalid")


class DatasetError(ValueError):
    """All problems found in one dataset, reported together."""

    def __init__(self, problems):
        self.problems = problems
        super().__init__("\n".join(problems))


def category(block):
    if block in CATEGORIES:
        return CATEGORIES[block]
    if block == 24:
        return "dungeon_report"
    if 0xC00 <= block < 0xE00:
        return "cutscene"
    if block >= 0xE00:
        return "conversation"
    return "unknown_static"


def object_name(block, index, source):
    """fix_name_string article syntax: article_name&plural (blocks 4, 7 >= 256)."""
    return bool(source) and (block == 4 or (block == 7 and index >= 256))


def derived_flags(block, index, source):
    flags = set()
    if template_tokens(source):
        flags.add("dynamic-template")
    if object_name(block, index, source):
        flags.add("object-name")
    if controls(source):
        flags.add("control-code")
    return flags


def classify(block, index, source, flags=()):
    """First-pass handling class; manual flags can only make an entry special."""
    flags = set(flags) | derived_flags(block, index, source)
    if index > 511:
        return "unaddressable"
    if not source:
        return "empty"
    if "dynamic-template" in flags:
        return "template"
    if "object-name" in flags:
        return "object-name"
    # Adjective/texture fragments are composed with hard-coded "You see ...";
    # edge whitespace marks a fragment concatenated at runtime.
    if ("grammar-sensitive" in flags or block in (5, 10) or source != source.strip(" ")):
        return "grammar"
    # CHARGEN.C, STATS.C and CUTS.C draw these with string_to_screen.
    if block == 2 or category(block) == "cutscene":
        return "bitmap-ui"
    if "hardcoded-related" in flags:
        return "hardcoded"
    if block == 24:
        return "debug-report"
    return "direct"


# ---------------------------------------------------------------- inventory

class Inventory:
    def __init__(self, pak_bytes, cnv_bytes=None):
        self.pak = StringsPak(pak_bytes)
        self.pak_sha256 = hashlib.sha256(pak_bytes).hexdigest()
        self.cnv_sha256 = hashlib.sha256(cnv_bytes).hexdigest() if cnv_bytes else None
        self.entries = {}
        for block, (_, count) in sorted(self.pak.blocks.items()):
            for index in range(count):
                self.entries[block, index] = self.pak.read(block, index).decode("cp437")
        self.conversations = conversation_headers(cnv_bytes) if cnv_bytes else []
        self.slot_of_block = {c["resolved_block"]: c["slot"] for c in self.conversations}

    @classmethod
    def load(cls, pak, cnv=None):
        return cls(Path(pak).read_bytes(), Path(cnv).read_bytes() if cnv else None)

    def source(self, block, index):
        return self.entries.get((block, index))

    def npc(self, slot):
        # UW1 keeps conversation partner names in block 7 at slot + 16.
        return self.entries.get((7, slot + 16), "")

    def context(self, block, index):
        if block in self.slot_of_block:
            slot = self.slot_of_block[block]
            return f"conversation slot {slot} ({self.npc(slot) or 'unnamed'})"
        if 0xC00 <= block < 0xE00:
            return f"cutscene {block - 0xC00}"
        return category(block)

    def block_digest(self, block):
        h = hashlib.sha256()
        for index in range(self.pak.blocks[block][1]):
            h.update(escape(self.entries[block, index]).encode("utf-8") + b"\n")
        return h.hexdigest()

    def summary(self):
        cats, handling = Counter(), defaultdict(Counter)
        for (block, index), source in self.entries.items():
            cats[category(block)] += 1
            handling[category(block)][classify(block, index, source)] += 1
        conversation_blocks = sorted(b for b in self.pak.blocks if b >= 0xE00)
        referenced = sorted(set(self.slot_of_block) & set(conversation_blocks))
        return {
            "pak_sha256": self.pak_sha256, "cnv_sha256": self.cnv_sha256, "encoding": "cp437",
            "block_count": len(self.pak.blocks), "entry_count": len(self.entries),
            "category_count": len(cats), "categories": dict(sorted(cats.items())),
            "handling": {c: dict(sorted(h.items())) for c, h in sorted(handling.items())},
            "empty_entries": sum(not s for s in self.entries.values()),
            "unaddressable_entries": sorted([b, i] for b, i in self.entries if i > 511),
            "unaddressable_nonempty": sorted([b, i] for (b, i), s in self.entries.items() if i > 511 and s),
            "dynamic_blocks_on_disk": [b for b in DYNAMIC_BLOCKS if b in self.pak.blocks],
            "block_zero_on_disk": 0 in self.pak.blocks,
            "block_zero_rule": "raw block 0 resolves to CutsceneOrConversationStringBlock (CNV header or 0xC00+n)",
            "cutscene_blocks": sorted(b for b in self.pak.blocks if 0xC00 <= b < 0xE00),
            "conversation_headers": len(self.conversations),
            "conversation_blocks": len(conversation_blocks),
            "conversation_blocks_referenced": len(referenced),
            "conversation_blocks_unreferenced": sorted(set(conversation_blocks) - set(self.slot_of_block)),
            "conversation_headers_without_block": sorted(set(self.slot_of_block) - set(conversation_blocks)),
            "conversation_slots": {str(c["slot"]): c["resolved_block"] for c in self.conversations},
            "blocks": {str(b): {"entries": n, "category": category(b), "sha256": self.block_digest(b)}
                       for b, (_, n) in sorted(self.pak.blocks.items())},
        }


def compare_baseline(summary, baseline):
    """Per-block drift between two inventory fingerprints (no text involved)."""
    old, new = baseline["blocks"], summary["blocks"]
    changed = sorted((int(b) for b in set(old) & set(new) if old[b] != new[b]))
    return {"pak_sha256_changed": summary["pak_sha256"] != baseline["pak_sha256"],
            "cnv_sha256_changed": summary["cnv_sha256"] != baseline["cnv_sha256"],
            "blocks_added": sorted(int(b) for b in set(new) - set(old)),
            "blocks_removed": sorted(int(b) for b in set(old) - set(new)),
            "blocks_changed": changed,
            "status": "SAME" if old == new and summary["conversation_slots"] == baseline["conversation_slots"]
                      else "DRIFT"}


def baseline_view(summary):
    keep = ("pak_sha256", "cnv_sha256", "encoding", "block_count", "entry_count", "category_count",
            "categories", "conversation_slots", "blocks")
    return {k: summary[k] for k in keep}


# ---------------------------------------------------------------- datasets

def read_tsv(path, fields):
    """Strict UTF-8 TSV: no BOM, LF only, exact header, reversible escapes."""
    raw = Path(path).read_bytes()
    problems = []
    if raw.startswith(b"\xef\xbb\xbf"):
        raise DatasetError([f"{path}: UTF-8 BOM is not allowed"])
    if b"\r" in raw:
        raise DatasetError([f"{path}: CR line endings are not allowed"])
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError as error:
        line = raw[:error.start].count(b"\n") + 1
        raise DatasetError([f"{path}:{line}: invalid UTF-8 at byte {error.start}"]) from None
    reader = csv.reader(io.StringIO(text, newline=""), delimiter="\t")
    header = next(reader, None)
    if header != fields:
        raise DatasetError([f"{path}: header must be {' '.join(fields)}"])
    rows = []
    for record in reader:
        line = reader.line_num
        if len(record) != len(fields):
            problems.append(f"{path}:{line}: expected {len(fields)} columns, found {len(record)}")
            continue
        try:
            row = {k: unescape(v) for k, v in zip(fields, record)}
        except ValueError as error:
            problems.append(f"{path}:{line}: {error}")
            continue
        row["_line"] = line
        rows.append(row)
    if problems:
        raise DatasetError(problems)
    return rows


def render_tsv(rows, fields):
    stream = io.StringIO(newline="")
    writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
    writer.writerow(fields)
    for row in rows:
        writer.writerow([escape(str(row[k])) for k in fields])
    return stream.getvalue().encode("utf-8")


def split_flags(text):
    return [f for f in text.split(",") if f] if text else []


def join_flags(flags):
    return ",".join(f for f in FLAGS if f in flags)


def text_problems(text, what):
    problems = []
    if "\0" in text:
        problems.append(f"{what} contains NUL")
    if any(0xD800 <= ord(c) <= 0xDFFF for c in text):
        problems.append(f"{what} contains a surrogate (not a Unicode scalar value)")
    if "﻿" in text:
        problems.append(f"{what} contains U+FEFF")
    if any(ord(c) < 32 and c != "\n" for c in text):
        problems.append(f"{what} contains a control character other than LF")
    return problems


def check_target(source, target):
    problems = text_problems(target, "target")
    try:
        validate_tokens(source, target)
    except ValueError as error:
        problems.append(str(error))
    if len(target.encode("utf-8", "surrogatepass")) > MAX_TARGET:
        problems.append(f"target exceeds {MAX_TARGET} UTF-8 bytes")
    # \m and \p pause for MORE; a trailing pause must stay trailing.
    for code in ("\\m", "\\p"):
        if source.endswith(code) != target.endswith(code):
            problems.append(f"trailing {code} pause moved")
    if "[MORE]" in target and "[MORE]" not in source:
        problems.append("target spells the [MORE] prompt; use the game's pause codes")
    return problems


def is_default(row, derived):
    return (row["status"] == "untranslated" and not row["target_zh_TW"] and not row["context"]
            and not row["notes"] and set(split_flags(row["flags"])) <= derived)


def check_master(rows, inventory=None):
    """Return (problems, drift). Without an inventory only row-level rules run."""
    problems, drift, seen = [], [], set()
    previous = None
    for row in rows:
        where = f"master:{row.get('_line', '?')}"
        try:
            block, index = int(row["block"]), int(row["index"])
            if row["block"] != str(block) or row["index"] != str(index):
                raise ValueError
        except ValueError:
            problems.append(f"{where}: block/index must be canonical decimal integers")
            continue
        key = (block, index)
        where = f"master:{row.get('_line', '?')} ({block}:{index})"
        if key in seen:
            problems.append(f"{where}: duplicate (block,index)")
        seen.add(key)
        if previous is not None and key < previous:
            problems.append(f"{where}: rows are not sorted by (block,index); run normalize")
        previous = key
        if block == 0 or block > 0x7FFF:
            problems.append(f"{where}: invalid block (block 0 must be stored as its resolved block)")
        if block in DYNAMIC_BLOCKS:
            problems.append(f"{where}: dynamic block 0x{block:X} cannot hold a static translation")
        if index > 511:
            problems.append(f"{where}: index > 511 is not addressable by get_string")
        if row["category"] != category(block):
            problems.append(f"{where}: category must be {category(block)}")
        if row["status"] not in STATUSES:
            problems.append(f"{where}: unknown status {row['status']!r}")
        flags = split_flags(row["flags"])
        if any(f not in FLAGS for f in flags):
            problems.append(f"{where}: unknown flag in {row['flags']!r}")
        elif row["flags"] != join_flags(set(flags)):
            problems.append(f"{where}: flags are not canonical; run normalize")
        source, target = row["source"], row["target_zh_TW"]
        problems += [f"{where}: {p}" for p in text_problems(source, "source")]
        if not source:
            problems.append(f"{where}: empty source entries cannot be translated")
        try:
            source.encode("cp437")
        except UnicodeEncodeError:
            problems.append(f"{where}: source is not representable in CP437")
        if len(source.encode("utf-8")) > 511:
            problems.append(f"{where}: source exceeds the game's 511-byte string")
        missing = derived_flags(block, index, source) - set(flags)
        if missing:
            problems.append(f"{where}: derived flags missing {sorted(missing)}; run normalize")
        if target:
            if row["status"] == "untranslated":
                problems.append(f"{where}: untranslated row has a target; set draft or clear it")
            problems += [f"{where}: {p}" for p in check_target(source, target)]
        elif row["status"] in PACKED:
            problems.append(f"{where}: status {row['status']} needs a target")
        if is_default(row, derived_flags(block, index, source)):
            problems.append(f"{where}: row carries no translator work; remove it or run normalize")
        if inventory is not None:
            current = inventory.source(block, index)
            if current is None:
                problems.append(f"{where}: key does not exist in the current STRINGS.PAK")
            elif current != source:
                drift.append({"block": block, "index": index, "line": row.get("_line")})
                problems.append(f"{where}: source drift (master source differs from current data)")
    return problems, drift


def check_literals(rows, literals=None):
    problems, seen = [], set()
    by_key = {r["source_key"]: r for r in literals} if literals is not None else None
    for row in rows:
        where = f"literals:{row.get('_line', '?')} ({row['key']})"
        if not row["key"].startswith("literal:"):
            problems.append(f"{where}: literal keys use the literal: namespace")
        if row["key"] in seen:
            problems.append(f"{where}: duplicate key")
        seen.add(row["key"])
        if row["status"] not in STATUSES:
            problems.append(f"{where}: unknown status {row['status']!r}")
        flags = split_flags(row["flags"])
        if any(f not in FLAGS for f in flags) or row["flags"] != join_flags(set(flags)):
            problems.append(f"{where}: flags unknown or not canonical")
        if row["target_zh_TW"]:
            problems += [f"{where}: {p}" for p in check_target(row["source"], row["target_zh_TW"])]
            if row["status"] == "untranslated":
                problems.append(f"{where}: untranslated row has a target")
        if by_key is not None:
            literal = by_key.get(row["key"])
            if literal is None:
                problems.append(f"{where}: key no longer exists in the literal scan")
            elif literal["source_en"] != row["source"]:
                problems.append(f"{where}: source drift")
            elif literal["classification"] != "PLAYER_VISIBLE":
                problems.append(f"{where}: literal is {literal['classification']}, not PLAYER_VISIBLE")
    return problems


def normalize_master(rows, inventory=None):
    """Canonical order, derived flags, category; drop rows carrying no work."""
    out = []
    for row in rows:
        block, index = int(row["block"]), int(row["index"])
        derived = derived_flags(block, index, row["source"])
        flags = set(split_flags(row["flags"])) | derived
        new = {k: row[k] for k in MASTER_FIELDS}
        new.update(block=str(block), index=str(index), category=category(block), flags=join_flags(flags))
        if inventory is not None and not new["source"]:
            new["source"] = inventory.source(block, index) or ""
        if not is_default(new, derived):
            out.append(new)
    out.sort(key=lambda r: (int(r["block"]), int(r["index"])))
    return out


def load_master(path=MASTER):
    return read_tsv(path, MASTER_FIELDS)


def load_literals(path=LITERALS):
    return read_tsv(path, LITERAL_FIELDS)


def static_targets(path=MASTER, min_status="draft"):
    """{(block,index): target} exactly as the catalog packs them."""
    return {(int(r["block"]), int(r["index"])): r["target_zh_TW"] for r in packed_rows(load_master(path), min_status)}


def packed_rows(rows, min_status="draft"):
    floor = PACKED[min_status]
    return [r for r in rows if r["target_zh_TW"] and PACKED.get(r["status"], -1) >= floor]


# ---------------------------------------------------------------- catalog

def pack(rows, min_status="draft"):
    """Deterministic UW1LOC1 catalog; same rows produce the same bytes."""
    entries = []
    for row in packed_rows(rows, min_status):
        block, index = int(row["block"]), int(row["index"])
        source, target = row["source"].encode("cp437"), row["target_zh_TW"].encode("utf-8")
        entries.append((block, index, source, target))
    entries.sort()
    if len({(b, i) for b, i, _, _ in entries}) != len(entries):
        raise DatasetError(["duplicate (block,index) in packed rows"])
    if not 0 < len(entries) <= MAX_ENTRIES:
        raise DatasetError([f"catalog must hold 1..{MAX_ENTRIES} entries, got {len(entries)}"])
    result = bytearray(MAGIC + struct.pack("<I", len(entries)))
    for block, index, source, target in entries:
        result += struct.pack("<HHII", block, index, len(source), len(target))
        result += source + b"\0" + target + b"\0"
    if len(result) > MAX_CATALOG:
        raise DatasetError([f"catalog exceeds {MAX_CATALOG} bytes"])
    return bytes(result), len(entries)


# ---------------------------------------------------------------- runtime trace

def read_traces(paths):
    """Aggregate --i18n-trace JSONL: {(block,index): Counter(event)}."""
    counts = defaultdict(Counter)
    for path in paths:
        for n, line in enumerate(Path(path).read_text(encoding="utf-8").splitlines(), 1):
            if not line.strip():
                continue
            try:
                record = json.loads(line)
                event, key = record["event"], (int(record["block"]), int(record["index"]))
            except (ValueError, KeyError, TypeError) as error:
                raise DatasetError([f"{path}:{n}: malformed trace record ({error})"]) from None
            if event not in EVENTS:
                raise DatasetError([f"{path}:{n}: unknown trace event {event!r}"])
            counts[key][event] += 1
    return counts


# ---------------------------------------------------------------- coverage

def coverage(inventory, master, literal_rows=(), literal_scan=None, traces=None, problems=()):
    by_key = {(int(r["block"]), int(r["index"])): r for r in master}
    traces = traces or {}
    cols = ("total", "empty", "unaddressable", "direct", "special", "untranslated", "draft",
            "reviewed", "approved", "blocked", "runtime_seen", "runtime_hit", "runtime_fallback")
    table = defaultdict(Counter)
    lists = Counter()
    runtime_untranslated, runtime_fallback, dynamic = set(), set(), set()
    for (block, index), source in inventory.entries.items():
        row = by_key.get((block, index))
        flags = set(split_flags(row["flags"])) if row else set()
        handling = classify(block, index, source, flags)
        all_flags = flags | derived_flags(block, index, source)
        t = table[category(block)]
        t["total"] += 1
        if handling in ("empty", "unaddressable"):
            t[handling] += 1
        else:
            t["special" if handling in SPECIAL else "direct"] += 1
            status = row["status"] if row and row["target_zh_TW"] else "untranslated"
            if row and row["status"] == "blocked":
                status = "blocked"
            t[status] += 1
            lists["untranslated"] += status == "untranslated"
            lists["template"] += handling == "template"
            lists["grammar-sensitive"] += handling in ("grammar", "object-name") or "grammar-sensitive" in all_flags
            lists["hardcoded-related"] += "hardcoded-related" in all_flags
            lists["bitmap-ui"] += handling == "bitmap-ui"
        events = traces.get((block, index))
        if events:
            t["runtime_seen"] += 1
            t["runtime_hit"] += events["hit"] > 0
            fallback = events["miss"] + events["drift"] + events["template"] > 0
            t["runtime_fallback"] += fallback
            if fallback:
                runtime_fallback.add((block, index))
            if events["miss"]:
                runtime_untranslated.add((block, index))
    for key, events in traces.items():
        if events["dynamic"] or events["invalid"]:
            dynamic.add(key)
    totals = Counter()
    for t in table.values():
        totals.update(t)
    scan = Counter(r["classification"] for r in literal_scan) if literal_scan is not None else None
    return {
        "columns": list(cols),
        "categories": {c: {k: table[c][k] for k in cols} for c in sorted(table)},
        "total": {k: totals[k] for k in cols},
        "special_strings": {
            "untranslated": lists["untranslated"], "template": lists["template"],
            "grammar_sensitive": lists["grammar-sensitive"], "bitmap_ui": lists["bitmap-ui"],
            "hardcoded_related": lists["hardcoded-related"],
            "runtime_untranslated": len(runtime_untranslated), "runtime_fallback": len(runtime_fallback),
            "runtime_dynamic_or_invalid_keys": len(dynamic),
            "source_drift": sum("source drift" in p for p in problems),
            "invalid_target": sum("source drift" not in p for p in problems),
            "hardcoded_literal_candidates": (scan["PLAYER_VISIBLE"] + scan["UNKNOWN"]) if scan else None,
            "hardcoded_literals_translated": sum(bool(r["target_zh_TW"]) for r in literal_rows),
        },
        "literal_scan": dict(sorted(scan.items())) if scan else None,
        "runtime": {"traced_keys": len(traces),
                    "events": dict(sum((Counter(e) for e in traces.values()), Counter())),
                    "untranslated_keys": sorted([b, i] for b, i in runtime_untranslated),
                    "fallback_keys": sorted([b, i] for b, i in runtime_fallback),
                    "dynamic_or_invalid_keys": sorted([b, i] for b, i in dynamic)} if traces else None,
        "pak_sha256": inventory.pak_sha256,
    }


def coverage_markdown(report):
    head = ("Category", "Total", "Empty", "Unaddr.", "Direct", "Special", "Untransl.", "Draft",
            "Reviewed", "Approved", "Blocked", "Runtime seen", "Runtime hit", "Runtime fallback")
    lines = ["# UW1 zh-TW localization coverage", "",
             f"PAK `{report['pak_sha256'][:16]}…`. Direct + Special = translatable non-empty, addressable entries.", "",
             "| " + " | ".join(head) + " |", "| --- |" + " ---: |" * (len(head) - 1)]
    for name, row in list(report["categories"].items()) + [("**total**", report["total"])]:
        lines.append("| " + " | ".join([name] + [str(row[c]) for c in report["columns"]]) + " |")
    lines += ["", "| Special list | Count |", "| --- | ---: |"]
    lines += [f"| {k} | {v} |" for k, v in report["special_strings"].items()]
    translatable = report["total"]["direct"] + report["total"]["special"]
    done = sum(report["total"][s] for s in ("draft", "reviewed", "approved"))
    lines += ["", f"Translated (draft or better): {done} / {translatable} "
                  f"({100 * done / translatable:.2f}%)." if translatable else ""]
    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------- export / import

EXPORT_FIELDS = ["key", "block", "index", "category", "handling", "flags", "status", "source",
                 "target_zh_TW", "context", "notes", "auto_context", "runtime_hits", "runtime_events",
                 "previous_source", "next_source"]
EDITABLE = ("target_zh_TW", "status", "context", "notes", "flags")


def export_rows(inventory, master, traces=None, select=None):
    by_key = {(int(r["block"]), int(r["index"])): r for r in master}
    traces = traces or {}
    for (block, index), source in sorted(inventory.entries.items()):
        if not source or index > 511:
            continue
        row = by_key.get((block, index))
        derived = derived_flags(block, index, source)
        flags = set(split_flags(row["flags"])) if row else set()
        handling = classify(block, index, source, flags)
        events = traces.get((block, index), Counter())
        out = {"key": f"{block}:{index}", "block": block, "index": index, "category": category(block),
               "handling": handling, "flags": join_flags(flags | derived),
               "status": row["status"] if row else "untranslated", "source": source,
               "target_zh_TW": row["target_zh_TW"] if row else "",
               "context": row["context"] if row else "", "notes": row["notes"] if row else "",
               "auto_context": inventory.context(block, index),
               "runtime_hits": sum(events.values()),
               "runtime_events": ",".join(f"{e}={events[e]}" for e in EVENTS if events[e]),
               "previous_source": inventory.source(block, index - 1) or "",
               "next_source": inventory.source(block, index + 1) or ""}
        if select is None or select(out):
            yield out


def merge(master, edits, inventory):
    """Apply workbook edits to master rows; source drift in an edit is rejected."""
    by_key = {(int(r["block"]), int(r["index"])): dict(r) for r in master}
    problems, changed = [], 0
    for edit in edits:
        key = (int(edit["block"]), int(edit["index"]))
        where = f"workbook:{edit.get('_line', '?')} ({key[0]}:{key[1]})"
        current = inventory.source(*key)
        if current is None:
            problems.append(f"{where}: key does not exist in the current STRINGS.PAK")
            continue
        if edit["source"] != current:
            problems.append(f"{where}: workbook source differs from current data (stale export)")
            continue
        derived = derived_flags(*key, current)
        base = by_key.get(key) or {"block": str(key[0]), "index": str(key[1]), "category": category(key[0]),
                                   "source": current, "target_zh_TW": "", "status": "untranslated",
                                   "context": "", "notes": "", "flags": join_flags(derived)}
        new = dict(base, **{k: edit[k] for k in EDITABLE}, source=current)
        if all(f in FLAGS for f in split_flags(new["flags"])):
            new["flags"] = join_flags(set(split_flags(new["flags"])) | derived)
        if any(new[k] != base[k] for k in EDITABLE):
            changed += 1
        by_key[key] = new
    if problems:
        raise DatasetError(problems)
    return normalize_master(by_key.values()), changed


# ---------------------------------------------------------------- CLI

def write_bytes(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def write_json(path, value):
    write_bytes(path, (json.dumps(value, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))


def literal_scan():
    from scan_literals import inventory as scan
    return scan()


def validate_all(args, inventory):
    problems = []
    try:
        master = load_master(args.master)
    except DatasetError as error:
        return None, None, error.problems, []
    found, drift = check_master(master, inventory)
    problems += found
    canonical = render_tsv(normalize_master(master), MASTER_FIELDS)
    if canonical != Path(args.master).read_bytes() and not any("normalize" in p for p in problems):
        problems.append("master: file is not in canonical form; run normalize")
    literal_rows = []
    if args.literals and Path(args.literals).exists():
        try:
            literal_rows = load_literals(args.literals)
            problems += check_literals(literal_rows, literal_scan())
        except DatasetError as error:
            problems += error.problems
    return master, literal_rows, problems, drift


def add_data(parser, required=True):
    parser.add_argument("--pak", type=Path, required=required, help="the player's DATA/STRINGS.PAK")
    parser.add_argument("--cnv", type=Path, help="the player's DATA/CNV.ARK (conversation context)")


def add_dataset(parser):
    parser.add_argument("--master", type=Path, default=MASTER)
    parser.add_argument("--literals", type=Path, default=LITERALS)


def cmd_inventory(args):
    inventory = Inventory.load(args.pak, args.cnv)
    summary = inventory.summary()
    rows = [{"block": b, "index": i, "category": category(b), "handling": classify(b, i, s),
             "flags": join_flags(derived_flags(b, i, s)), "source_en": s}
            for (b, i), s in sorted(inventory.entries.items())]
    write_bytes(args.out / "inventory.tsv", render_tsv(rows, list(rows[0])))
    report = {"summary": summary}
    if args.baseline.exists():
        report["baseline"] = compare_baseline(summary, json.loads(args.baseline.read_text(encoding="utf-8")))
    if args.write_baseline:
        write_json(args.baseline, baseline_view(summary))
    write_json(args.out / "inventory.json", report)
    status = report.get("baseline", {}).get("status", "NO BASELINE")
    print(f"{summary['entry_count']} entries, {summary['block_count']} blocks, "
          f"{summary['category_count']} categories; baseline {status} -> {args.out}")
    return 1 if status == "DRIFT" and not args.write_baseline else 0


def cmd_validate(args):
    inventory = Inventory.load(args.pak, args.cnv) if args.pak else None
    master, literal_rows, problems, drift = validate_all(args, inventory)
    report = {"status": "FAIL" if problems else "PASS", "inventory_checked": inventory is not None,
              "master_rows": len(master or ()), "literal_rows": len(literal_rows or ()),
              "packed_rows": len(packed_rows(master or ())), "source_drift": drift, "problems": problems}
    if args.report:
        write_json(args.report, report)
    for p in problems:
        print(p, file=sys.stderr)
    print(f"{report['status']}: {report['master_rows']} master rows, {report['literal_rows']} literal rows, "
          f"{len(problems)} problems, {len(drift)} drift")
    return 1 if problems else 0


def cmd_normalize(args):
    inventory = Inventory.load(args.pak, args.cnv) if args.pak else None
    rows = normalize_master(load_master(args.master), inventory)
    write_bytes(args.master, render_tsv(rows, MASTER_FIELDS))
    if args.literals.exists():
        literal_rows = [dict(r, flags=join_flags(set(split_flags(r["flags"]))))
                        for r in sorted(load_literals(args.literals), key=lambda r: r["key"])]
        write_bytes(args.literals, render_tsv(literal_rows, LITERAL_FIELDS))
    print(f"{len(rows)} master rows written -> {args.master}")
    return 0


def cmd_build(args):
    inventory = Inventory.load(args.pak, args.cnv)
    master, _, problems, drift = validate_all(args, inventory)
    if problems:
        for p in problems:
            print(p, file=sys.stderr)
        print(f"FAIL: catalog not built ({len(problems)} problems, {len(drift)} drift)")
        return 1
    data, count = pack(master, args.min_status)
    write_bytes(args.output, data)
    manifest = {"catalog_sha256": hashlib.sha256(data).hexdigest(), "bytes": len(data), "entries": count,
                "min_status": args.min_status,
                "master_sha256": hashlib.sha256(Path(args.master).read_bytes()).hexdigest(),
                "pak_sha256": inventory.pak_sha256, "format": MAGIC.rstrip(b"\0").decode()}
    if args.manifest:
        write_json(args.manifest, manifest)
    print(f"{count} entries, {len(data)} bytes, sha256 {manifest['catalog_sha256']} -> {args.output}")
    return 0


def cmd_coverage(args):
    inventory = Inventory.load(args.pak, args.cnv)
    master, literal_rows, problems, _ = validate_all(args, inventory)
    if master is None:
        print("\n".join(problems), file=sys.stderr)
        return 1
    report = coverage(inventory, master, literal_rows, literal_scan(), read_traces(args.trace), problems)
    report["validation"] = "FAIL" if problems else "PASS"
    write_json(args.out / "coverage.json", report)
    write_bytes(args.out / "coverage.md", coverage_markdown(report).encode("utf-8"))
    print(coverage_markdown(report))
    return 0


def cmd_export(args):
    inventory = Inventory.load(args.pak, args.cnv)
    master = load_master(args.master)
    traces = read_traces(args.trace)

    def select(row):
        return ((not args.category or row["category"] in args.category)
                and (not args.block or row["block"] in args.block)
                and (not args.handling or row["handling"] in args.handling)
                and (not args.untranslated or not row["target_zh_TW"])
                and (not args.runtime or row["runtime_hits"] > 0))
    rows = list(export_rows(inventory, master, traces, select))
    write_bytes(args.output, render_tsv(rows, EXPORT_FIELDS))
    print(f"{len(rows)} rows -> {args.output}")
    return 0


def cmd_import(args):
    inventory = Inventory.load(args.pak, args.cnv)
    edits = read_tsv(args.workbook, EXPORT_FIELDS)
    rows, changed = merge(load_master(args.master), edits, inventory)
    problems, drift = check_master(rows, inventory)
    if problems:
        for p in problems:
            print(p, file=sys.stderr)
        print(f"FAIL: master not changed ({len(problems)} problems)")
        return 1
    write_bytes(args.master, render_tsv(rows, MASTER_FIELDS))
    print(f"{changed} rows changed; {len(rows)} master rows -> {args.master}")
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("inventory", help="refresh the inventory and compare the committed fingerprint")
    add_data(p)
    p.add_argument("--out", type=Path, default=UW1 / "build/i18n/p9")
    p.add_argument("--baseline", type=Path, default=BASELINE)
    p.add_argument("--write-baseline", action="store_true", help="accept the current data as the baseline")
    p.set_defaults(run=cmd_inventory)

    p = sub.add_parser("validate", help="validate master/literals (with --pak: source drift too)")
    add_data(p, required=False)
    add_dataset(p)
    p.add_argument("--report", type=Path)
    p.set_defaults(run=cmd_validate)

    p = sub.add_parser("normalize", help="rewrite master/literals canonically")
    add_data(p, required=False)
    add_dataset(p)
    p.set_defaults(run=cmd_normalize)

    p = sub.add_parser("build", help="validate against current data and pack the runtime catalog")
    add_data(p)
    add_dataset(p)
    p.add_argument("--output", type=Path, default=UW1 / "build/i18n/zh-TW.uwloc")
    p.add_argument("--manifest", type=Path)
    p.add_argument("--min-status", choices=list(PACKED), default="draft")
    p.set_defaults(run=cmd_build)

    p = sub.add_parser("coverage", help="coverage matrix (JSON + Markdown)")
    add_data(p)
    add_dataset(p)
    p.add_argument("--trace", type=Path, action="append", default=[], help="--i18n-trace JSONL (repeatable)")
    p.add_argument("--out", type=Path, default=UW1 / "build/i18n/p9")
    p.set_defaults(run=cmd_coverage)

    p = sub.add_parser("export", help="translator/QA workbook TSV (contains English; keep it local)")
    add_data(p)
    add_dataset(p)
    p.add_argument("--trace", type=Path, action="append", default=[])
    p.add_argument("--output", type=Path, default=UW1 / "build/i18n/p9/workbook.tsv")
    p.add_argument("--category", action="append")
    p.add_argument("--block", type=int, action="append")
    p.add_argument("--handling", action="append", choices=HANDLING)
    p.add_argument("--untranslated", action="store_true")
    p.add_argument("--runtime", action="store_true", help="only keys seen in the given traces")
    p.set_defaults(run=cmd_export)

    p = sub.add_parser("import", help="merge an edited workbook into the master dataset")
    add_data(p)
    add_dataset(p)
    p.add_argument("workbook", type=Path)
    p.set_defaults(run=cmd_import)

    args = parser.parse_args(argv)
    try:
        return args.run(args)
    except DatasetError as error:
        for problem in error.problems:
            print(problem, file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
