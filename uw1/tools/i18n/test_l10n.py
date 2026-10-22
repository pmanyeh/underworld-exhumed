"""Asset-free tests for the Phase 9 localization production pipeline (l10n.py)."""
from collections import Counter
import hashlib
from pathlib import Path
import tempfile
import unittest

import l10n
from l10n import (DatasetError, Inventory, MASTER_FIELDS, check_master, classify, coverage, merge,
                  normalize_master, pack, read_traces, read_tsv, render_tsv)

UW1 = Path(__file__).resolve().parents[2]
OUT = UW1 / "build/i18n"
OUT.mkdir(parents=True, exist_ok=True)
# Catalog of the 52 static PoC rows that P8 accepted (docs/i18n/evidence/p8/manifest.json).
P8_CATALOG_SHA256 = "fd133b9a159b10e3eb178e6b798867ac3663ea5b5bdbf54e4fba2db68d1dff98"


class FakePak:
    def __init__(self, blocks):
        self.blocks = {b: (0, n) for b, n in blocks.items()}


def inventory(entries):
    """An Inventory over {(block,index): source}, without STRINGS.PAK."""
    inv = Inventory.__new__(Inventory)
    counts = Counter()
    for block, index in entries:
        counts[block] = max(counts[block], index + 1)
    inv.entries = {(b, i): entries.get((b, i), "") for b in counts for i in range(counts[b])}
    inv.pak = FakePak(counts)
    inv.pak_sha256, inv.cnv_sha256 = "0" * 64, None
    inv.conversations = [{"slot": 67, "resolved_block": 3651}]
    inv.slot_of_block = {3651: 67}
    return inv


INV = inventory({(1, 0): "Plain.\n", (1, 1): "Second", (1, 2): "", (4, 0): "a_sword&swords",
                 (3651, 1): "lad", (3651, 2): "Hail, @SS1!\\m", (3651, 3): "Bye.", (7, 83): "Bragit",
                 (2, 600): "", (5, 0): "broken", (10, 0): "a wall", (24, 0): " for level ",
                 (0xC00, 0): "Cutscene line", (6, 0): "Light"})


def row(block, index, target="", status=None, **extra):
    source = INV.source(block, index)
    value = {"block": str(block), "index": str(index), "category": l10n.category(block), "source": source,
             "target_zh_TW": target, "status": status or ("draft" if target else "untranslated"),
             "context": "", "notes": "", "flags": l10n.join_flags(l10n.derived_flags(block, index, source))}
    value.update(extra)
    return value


class ClassificationTests(unittest.TestCase):
    def test_handling_classes(self):
        expected = {(1, 0): "direct", (1, 2): "empty", (2, 600): "unaddressable", (4, 0): "object-name",
                    (3651, 2): "template", (5, 0): "grammar", (10, 0): "grammar", (24, 0): "grammar",
                    (0xC00, 0): "bitmap-ui", (6, 0): "direct", (7, 83): "direct"}
        for key, handling in expected.items():
            with self.subTest(key=key):
                self.assertEqual(classify(*key, INV.source(*key)), handling)
        self.assertEqual(classify(1, 1, "Second", {"hardcoded-related"}), "hardcoded")
        self.assertEqual(classify(3651, 1, "lad", {"grammar-sensitive"}), "grammar")
        self.assertEqual(classify(24, 1, "Dungeon Report as of "), "grammar")
        self.assertEqual(classify(24, 1, "Critter summary"), "debug-report")

    def test_derived_flags(self):
        self.assertEqual(l10n.derived_flags(3651, 2, "Hail, @SS1!\\m"), {"dynamic-template", "control-code"})
        self.assertEqual(l10n.derived_flags(7, 256, "a_bat"), {"object-name"})
        self.assertEqual(l10n.derived_flags(7, 83, "Bragit"), set())

    def test_context(self):
        self.assertEqual(INV.context(3651, 1), "conversation slot 67 (Bragit)")
        self.assertEqual(INV.context(0xC01, 0), "cutscene 1")


class DatasetTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(dir=OUT)
        self.path = Path(self.tmp.name) / "master.tsv"

    def tearDown(self):
        self.tmp.cleanup()

    def test_committed_master_round_trips_byte_for_byte(self):
        for path, fields in ((l10n.MASTER, MASTER_FIELDS), (l10n.LITERALS, l10n.LITERAL_FIELDS)):
            with self.subTest(path=path.name):
                raw = path.read_bytes()
                self.assertEqual(render_tsv(read_tsv(path, fields), fields), raw)
        rows = l10n.load_master()
        self.assertEqual(render_tsv(normalize_master(rows), MASTER_FIELDS), l10n.MASTER.read_bytes())
        self.assertEqual(check_master(rows), ([], []))

    def test_poc_migration_is_lossless(self):
        rows = l10n.load_master()
        bragit = [r for r in rows if r["block"] == "3651"]
        self.assertEqual(len(bragit), 49)
        poc = [r for r in rows if r["block"] == "3651" or (r["block"], r["index"]) in
               {("1", "2"), ("1", "5"), ("7", "83")}]
        self.assertTrue(all(r["context"].startswith("PoC role: ") and r["notes"] for r in poc))
        data, count = pack(poc)
        self.assertEqual((count, hashlib.sha256(data).hexdigest()), (52, P8_CATALOG_SHA256))
        literal, = l10n.load_literals()
        self.assertEqual((literal["source"], literal["target_zh_TW"]), ("[MORE]", "[繼續]"))

    def test_strict_reader(self):
        good = render_tsv([row(1, 0, "純文字。\n")], MASTER_FIELDS)
        for data, message in ((b"\xef\xbb\xbf" + good, "BOM"), (good.replace(b"\n", b"\r\n"), "CR"),
                              (good + b"1\t1\tgame\t\xff\t\t\t\t\t\n", "invalid UTF-8"),
                              (b"block\tindex\n", "header"), (good + b"1\t1\n", "columns"),
                              (good + b"1\t1\tgame\tSecond\t\\q\tdraft\t\t\t\n", "escape")):
            with self.subTest(message=message):
                self.path.write_bytes(data)
                with self.assertRaises(DatasetError) as caught:
                    read_tsv(self.path, MASTER_FIELDS)
                self.assertIn(message.split()[0], str(caught.exception).replace("TSV escape", "escape"))
        self.path.write_bytes(good)
        self.assertEqual(read_tsv(self.path, MASTER_FIELDS)[0]["target_zh_TW"], "純文字。\n")

    def problems(self, rows, inv=None):
        return check_master(rows, inv)[0]

    def test_valid_rows(self):
        rows = [row(1, 0, "純文字。\n"), row(3651, 1, "小伙子", flags="grammar-sensitive"),
                row(3651, 2, "你好，@SS1！\\m", "reviewed"), row(3651, 3, notes="needs a tone pass")]
        self.assertEqual(self.problems(rows, INV), [])

    def test_row_rules(self):
        cases = {
            "duplicate": [row(1, 0, "甲\n"), row(1, 0, "甲\n")],
            "not sorted": [row(1, 1, "乙"), row(1, 0, "甲\n")],
            "canonical decimal": [dict(row(1, 0, "甲\n"), index="00")],
            "invalid block": [dict(row(1, 0, "甲\n"), block="0", category="unknown_static")],
            "dynamic block": [dict(row(1, 0, "甲\n"), block="124", category="unknown_static")],
            "not addressable": [dict(row(1, 0, "甲\n"), index="512")],
            "category must be": [row(1, 0, "甲\n", category="books")],
            "unknown status": [row(1, 0, "甲\n", status="done")],
            "unknown flag": [row(1, 0, "甲\n", flags="urgent")],
            "not canonical": [row(1, 0, "甲\n", flags="needs-context,showcase")],
            "derived flags missing": [row(3651, 2, "你好，@SS1！\\m", flags="control-code")],
            "untranslated row has a target": [row(1, 0, "甲\n", status="untranslated")],
            "needs a target": [row(1, 0, "", status="reviewed")],
            "carries no translator work": [row(1, 1)],
            "templates differ": [row(3651, 2, "你好！\\m")],
            "controls differ": [row(3651, 2, "你好，@SS1！")],
            "newline count": [row(1, 0, "甲")],
            "NUL": [row(1, 0, "甲\0\n")],
            "surrogate": [row(1, 0, "甲\ud800\n")],
            "control character": [row(1, 0, "甲\t\n")],
            "[MORE]": [row(1, 1, "乙[MORE]")],
            "U+FEFF": [row(1, 1, "﻿乙")],
            "empty source": [row(1, 2, notes="x")],
        }
        for message, rows in cases.items():
            with self.subTest(message=message):
                self.assertTrue(any(message in p for p in self.problems(rows)), self.problems(rows))

    def test_trailing_pause_must_stay_trailing(self):
        source = "One\\m two"
        rows = [dict(row(1, 1, "一 二\\m"), source=source, flags="control-code")]
        self.assertTrue(any("trailing" in p for p in self.problems(rows)))

    def test_source_drift_and_missing_key_need_inventory(self):
        drifted = [row(1, 1, "乙", source="Changed")]
        self.assertEqual(self.problems(drifted), [])
        problems, drift = check_master(drifted, INV)
        self.assertEqual(drift, [{"block": 1, "index": 1, "line": None}])
        self.assertTrue(any("source drift" in p for p in problems))
        missing = [dict(row(1, 0, "甲\n"), index="9")]
        self.assertTrue(any("does not exist" in p for p in self.problems(missing, INV)))

    def test_normalize_is_canonical_and_idempotent(self):
        rows = [row(3651, 2, "你好，@SS1！\\m", flags=""), row(1, 1), row(1, 0, "甲\n", category="")]
        once = normalize_master(rows)
        self.assertEqual([(r["block"], r["index"]) for r in once], [("1", "0"), ("3651", "2")])
        self.assertEqual(once[1]["flags"], "dynamic-template,control-code")
        self.assertEqual(once[0]["category"], "game")
        self.assertEqual(normalize_master(once), once)
        self.assertEqual(self.problems(once, INV), [])


class CatalogTests(unittest.TestCase):
    def test_deterministic_and_status_filtered(self):
        rows = [row(3651, 3, "再見。", "approved"), row(1, 0, "甲\n"), row(1, 1, "乙", "blocked")]
        data, count = pack(rows)
        self.assertEqual(pack(list(reversed(rows))), (data, count))
        self.assertEqual(count, 2)
        self.assertEqual(pack(rows, "approved")[1], 1)
        with self.assertRaises(DatasetError):
            pack([row(1, 1, "乙", "blocked")])

    def test_layout_matches_runtime_contract(self):
        data, _ = pack([row(1, 0, "甲\n")])
        self.assertEqual(data, l10n.MAGIC + b"\x01\0\0\0" + b"\x01\0\0\0\x07\0\0\0\x04\0\0\0"
                         + b"Plain.\n\0" + "甲\n".encode() + b"\0")


class TraceCoverageTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(dir=OUT)
        self.trace = Path(self.tmp.name) / "trace.jsonl"
        self.trace.write_text("\n".join([
            '{"event":"hit","block":1,"index":0}', '{"event":"hit","block":1,"index":0}',
            '{"event":"miss","block":1,"index":1}', '{"event":"template","block":3651,"index":2}',
            '{"event":"dynamic","block":124,"index":3}', '{"event":"invalid","block":0,"index":3}']) + "\n",
            encoding="utf-8")

    def tearDown(self):
        self.tmp.cleanup()

    def test_trace_aggregation_and_rejection(self):
        traces = read_traces([self.trace, self.trace])
        self.assertEqual(traces[1, 0], Counter(hit=4))
        self.assertEqual(traces[124, 3], Counter(dynamic=2))
        bad = Path(self.tmp.name) / "bad.jsonl"
        for text in ('{"event":"seen","block":1,"index":0}', '{"block":1}', "not json"):
            bad.write_text(text, encoding="utf-8")
            with self.assertRaises(DatasetError):
                read_traces([bad])

    def test_coverage_matrix(self):
        master = [row(1, 0, "甲\n"), row(3651, 2, "你好，@SS1！\\m", "reviewed"), row(3651, 3, "再見。", "approved"),
                  row(1, 1, "", "blocked", notes="awaiting context")]
        report = coverage(INV, master, traces=read_traces([self.trace]))
        total = report["total"]
        self.assertEqual(total["total"], len(INV.entries))
        self.assertEqual((total["draft"], total["reviewed"], total["approved"], total["blocked"]), (1, 1, 1, 1))
        translatable = total["direct"] + total["special"]
        self.assertEqual(translatable + total["empty"] + total["unaddressable"], total["total"])
        self.assertEqual(sum(total[s] for s in ("untranslated", "draft", "reviewed", "approved", "blocked")),
                         translatable)
        game = report["categories"]["game"]
        self.assertEqual((game["runtime_seen"], game["runtime_hit"], game["runtime_fallback"]), (2, 1, 1))
        special = report["special_strings"]
        self.assertEqual((special["runtime_untranslated"], special["runtime_fallback"],
                          special["runtime_dynamic_or_invalid_keys"]), (1, 2, 2))
        self.assertEqual(report["runtime"]["untranslated_keys"], [[1, 1]])
        self.assertIn("| **total** |", l10n.coverage_markdown(report))


class RoundTripTests(unittest.TestCase):
    def test_export_edit_import(self):
        master = [row(1, 0, "甲\n")]
        rows = list(l10n.export_rows(INV, master, {(1, 1): Counter(miss=3)}))
        by_key = {r["key"]: r for r in rows}
        self.assertNotIn("1:2", by_key)  # empty
        self.assertEqual(by_key["1:1"]["runtime_events"], "miss=3")
        self.assertEqual(by_key["3651:1"]["auto_context"], "conversation slot 67 (Bragit)")
        self.assertEqual(by_key["1:1"]["previous_source"], "Plain.\n")
        with tempfile.TemporaryDirectory(dir=OUT) as tmp:
            workbook = Path(tmp) / "workbook.tsv"
            by_key["1:1"].update(target_zh_TW="乙", status="draft")
            by_key["1:0"].update(target_zh_TW="", status="untranslated")
            workbook.write_bytes(render_tsv(rows, l10n.EXPORT_FIELDS))
            edits = read_tsv(workbook, l10n.EXPORT_FIELDS)
            merged, changed = merge(master, edits, INV)
            self.assertEqual(changed, 2)
            self.assertEqual([(r["block"], r["index"], r["target_zh_TW"]) for r in merged], [("1", "1", "乙")])
            self.assertEqual(check_master(merged, INV), ([], []))
            stale = [dict(e, source="Old text") for e in edits if e["key"] == "1:1"]
            with self.assertRaises(DatasetError):
                merge(master, stale, INV)


class BaselineTests(unittest.TestCase):
    def test_compare_baseline(self):
        summary = {"pak_sha256": "a", "cnv_sha256": "b", "conversation_slots": {"67": 3651},
                   "blocks": {"1": {"entries": 2, "sha256": "x"}, "2": {"entries": 1, "sha256": "y"}}}
        self.assertEqual(l10n.compare_baseline(summary, summary)["status"], "SAME")
        changed = dict(summary, pak_sha256="c", blocks={"1": {"entries": 2, "sha256": "z"},
                                                        "3": {"entries": 1, "sha256": "w"}})
        result = l10n.compare_baseline(changed, summary)
        self.assertEqual((result["status"], result["blocks_changed"], result["blocks_added"],
                          result["blocks_removed"]), ("DRIFT", [1], [3], [2]))


if __name__ == "__main__":
    unittest.main()
