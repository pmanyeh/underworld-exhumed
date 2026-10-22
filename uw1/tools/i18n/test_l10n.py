"""Asset-free tests for the Phase 9 localization production pipeline (l10n.py)."""
from collections import Counter
import hashlib
import os
from pathlib import Path
import tempfile
import unittest

import l10n
from l10n import (DatasetError, Inventory, MASTER_FIELDS, check_master, classify, coverage, merge,
                  normalize_master, pack, read_traces, read_tsv, render_tsv, source_hash)

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
    value = {"block": str(block), "index": str(index), "category": l10n.category(block),
             "source_hash": source_hash(source),
             "target_zh_TW": target, "status": status or ("draft" if target else "untranslated"),
             "context": "", "notes": "", "flags": l10n.join_flags(l10n.derived_flags(block, index, source))}
    value.update(extra)
    return value


class ClassificationTests(unittest.TestCase):
    def test_handling_classes(self):
        expected = {(1, 0): "direct", (1, 2): "empty", (2, 600): "unaddressable", (4, 0): "object-name",
                    (3651, 2): "template", (5, 0): "grammar", (10, 0): "grammar", (24, 0): "debug-report",
                    (0xC00, 0): "bitmap-ui", (6, 0): "grammar", (7, 83): "bitmap-ui"}
        for key, handling in expected.items():
            with self.subTest(key=key):
                self.assertEqual(classify(*key, INV.source(*key)), handling)
        self.assertEqual(classify(1, 1, "Second", {"hardcoded-related"}), "hardcoded")
        self.assertEqual(classify(1, 1, "Second", {"needs-context"}), "needs-context")
        self.assertEqual(classify(3651, 1, "lad", {"grammar-sensitive"}), "grammar")
        self.assertEqual(classify(24, 1, "Dungeon Report as of "), "debug-report")
        self.assertEqual(classify(24, 1, "Critter summary"), "debug-report")

    def test_verified_renderer_rules(self):
        cases = {(1, 169): "bitmap-ui", (1, 256): "bitmap-ui", (1, 257): "bitmap-ui", (1, 188): "bitmap-ui",
                 (1, 190): "bitmap-ui", (1, 168): "direct", (2, 1): "bitmap-ui", (2, 0x33): "input-token",
                 (2, 0x33 + 0x19): "input-token", (2, 0x33 + 0x1A): "bitmap-ui", (6, 0x17F): "grammar",
                 (6, 0x180): "direct", (6, 0x1BF): "direct", (6, 0x1C0): "grammar", (7, 1): "direct",
                 (7, 3): "grammar", (7, 16): "grammar", (7, 17): "bitmap-ui", (7, 255): "bitmap-ui"}
        for key, handling in cases.items():
            with self.subTest(key=key):
                self.assertEqual(classify(*key, "Text"), handling)
        # Templates and article syntax keep precedence over renderer rules.
        self.assertEqual(classify(7, 300, "a_bat"), "object-name")
        self.assertEqual(classify(1, 169, "@GS1"), "template")

    def test_scope(self):
        self.assertEqual([l10n.scope(*k) for k in ((1, 0, "x"), (24, 0, "x"), (1, 2, ""), (2, 600, ""),
                                                   (2, 600, "x"), (1, 169, "x"), (2, 0x33, "RA"))],
                         ["production", "debug", "empty", "unaddressable", "unaddressable", "production",
                          "production"])

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

    def poc_rows(self):
        return [r for r in l10n.load_master() if r["block"] == "3651" or (r["block"], r["index"]) in
                {("1", "2"), ("1", "5"), ("7", "83")}]

    def test_poc_migration_is_lossless(self):
        poc = self.poc_rows()
        self.assertEqual((len(poc), sum(r["block"] == "3651" for r in poc)), (52, 49))
        self.assertTrue(all(r["context"].startswith("PoC role: ") and r["notes"] for r in poc))
        literal, = l10n.load_literals()
        self.assertEqual((literal["source"], literal["target_zh_TW"]), ("[MORE]", "[繼續]"))

    @unittest.skipUnless(os.environ.get("UW1_DATA"), "needs the player's game data (UW1_DATA)")
    def test_poc_catalog_equals_p8_with_player_data(self):
        inv = Inventory.load(Path(os.environ["UW1_DATA"]) / "DATA/STRINGS.PAK")
        data, count = pack(self.poc_rows(), inv)
        self.assertEqual((count, hashlib.sha256(data).hexdigest()), (52, P8_CATALOG_SHA256))

    def test_committed_master_holds_no_english(self):
        header = l10n.MASTER.read_bytes().split(b"\n", 1)[0].decode().split("\t")
        self.assertEqual(header, MASTER_FIELDS)
        self.assertNotIn("source", header)
        rows = l10n.load_master()
        self.assertTrue(all(l10n.HASH.fullmatch(r["source_hash"]) for r in rows))

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
            "U+FEFF": [row(1, 1, chr(0xFEFF) + "乙")],
            "empty source": [row(1, 2, notes="x")],
        }
        cases["bad hash"] = [row(1, 0, "甲\n", source_hash="ABC")]
        for message, rows in cases.items():
            with self.subTest(message=message):
                problems = self.problems(rows, INV)
                if message == "bad hash":
                    message = "64 lowercase hex"
                self.assertTrue(any(message in p for p in problems), problems)

    def test_rules_without_player_data_check_target_text_only(self):
        for message, target in (("NUL", "甲\0\n"), ("surrogate", "甲\ud800\n"), ("control character", "甲\t\n"),
                                ("U+FEFF", chr(0xFEFF) + "甲\n"), ("unknown BABL", "甲@X1\n")):
            with self.subTest(message=message):
                self.assertTrue(any(message in p for p in self.problems([row(1, 0, target)])))
        self.assertEqual(self.problems([row(3651, 2, "你好！")]), [])  # token check needs the source

    def test_trailing_pause_must_stay_trailing(self):
        inv = inventory({(1, 1): "One\\m two"})
        rows = [dict(row(1, 1, "一 二\\m"), source_hash=source_hash("One\\m two"), flags="control-code")]
        self.assertTrue(any("trailing" in p for p in self.problems(rows, inv)))

    def test_source_drift_and_missing_key_need_inventory(self):
        drifted = [row(1, 1, "乙", source_hash=source_hash("Changed"))]
        self.assertEqual(self.problems(drifted), [])
        problems, drift = check_master(drifted, INV)
        self.assertEqual(drift, [{"block": 1, "index": 1, "line": None}])
        self.assertTrue(any("SOURCE_DRIFT" in p for p in problems))
        missing = [dict(row(1, 0, "甲\n"), index="9")]
        self.assertTrue(any("does not exist" in p for p in self.problems(missing, INV)))

    def test_normalize_is_canonical_and_idempotent(self):
        rows = [row(3651, 2, "你好，@SS1！\\m", flags=""), row(1, 1), row(1, 0, "甲\n", category="")]
        once = normalize_master(rows, INV)
        self.assertEqual([(r["block"], r["index"]) for r in once], [("1", "0"), ("3651", "2")])
        self.assertEqual(once[1]["flags"], "dynamic-template,control-code")
        self.assertEqual(once[0]["category"], "game")
        self.assertEqual(normalize_master(once, INV), once)
        self.assertEqual(normalize_master(once), once)
        self.assertEqual(self.problems(once, INV), [])


class SourceHashTests(unittest.TestCase):
    def test_deterministic_known_vector(self):
        self.assertEqual(source_hash("Bragit"), hashlib.sha256(b"Bragit").hexdigest())
        self.assertEqual(source_hash("Bragit"), source_hash("Bragit"))
        self.assertTrue(l10n.HASH.fullmatch(source_hash("")))

    def test_canonical_bytes_are_the_games_cp437(self):
        # The decoder maps game bytes through CP437; hashing re-encodes them exactly.
        self.assertEqual(source_hash("café"), hashlib.sha256(b"caf\x82").hexdigest())
        self.assertEqual(source_hash("a\nb"), hashlib.sha256(b"a\nb").hexdigest())

    def test_any_source_change_is_a_different_hash(self):
        variants = ["Bye.", "Bye. ", "bye.", "Bye.\n", "Bye" + chr(92) + "m"]
        self.assertEqual(len({source_hash(v) for v in variants}), len(variants))

    def test_build_rejects_drift_without_updating(self):
        drifted = [row(1, 1, "乙", source_hash=source_hash("Changed"))]
        with self.assertRaises(DatasetError) as caught:
            pack(drifted, INV)
        self.assertIn("SOURCE_DRIFT", str(caught.exception))
        self.assertEqual(normalize_master(drifted, INV)[0]["source_hash"], source_hash("Changed"))

    def test_target_only_change_keeps_hash_and_builds(self):
        master = [row(1, 0, "甲\n")]
        edit, = [{k: str(v) for k, v in r.items()} for r in l10n.export_rows(INV, master) if r["key"] == "1:0"]
        merged, changed = merge(master, [dict(edit, target_zh_TW="純文字。\n")], INV)
        self.assertEqual(changed, 1)
        self.assertEqual(merged[0]["source_hash"], master[0]["source_hash"])
        self.assertEqual(merged[0]["target_zh_TW"], "純文字。\n")
        self.assertEqual(check_master(merged, INV), ([], []))
        self.assertEqual(pack(merged, INV)[1], 1)

    def test_stale_workbook_rejected_master_untouched(self):
        master = [row(1, 0, "甲\n")]
        edit, = [{k: str(v) for k, v in r.items()} for r in l10n.export_rows(INV, master) if r["key"] == "1:0"]
        changed_data = inventory({**INV.entries, (1, 0): "Plain, changed.\n"})
        before = [dict(r) for r in master]
        with self.assertRaises(DatasetError) as caught:
            merge(master, [dict(edit, target_zh_TW="新")], changed_data)
        self.assertIn("stale export", str(caught.exception))
        self.assertEqual(master, before)
        problems, drift = check_master(master, changed_data)
        self.assertEqual(len(drift), 1)
        self.assertTrue(any("SOURCE_DRIFT" in p for p in problems))

    def test_explicit_accept_records_current_hash(self):
        drifted = [row(1, 1, "乙", source_hash=source_hash("Changed"))]
        accepted = normalize_master(drifted, INV, accept_drift=True)
        self.assertEqual(accepted[0]["source_hash"], source_hash("Second"))
        self.assertEqual(check_master(accepted, INV), ([], []))


class CatalogTests(unittest.TestCase):
    def test_deterministic_and_status_filtered(self):
        rows = [row(3651, 3, "再見。", "approved"), row(1, 0, "甲\n"), row(1, 1, "乙", "blocked")]
        data, count = pack(rows, INV)
        self.assertEqual(pack(list(reversed(rows)), INV), (data, count))
        self.assertEqual(count, 2)
        self.assertEqual(pack(rows, INV, "approved")[1], 1)
        with self.assertRaises(DatasetError):
            pack([row(1, 1, "乙", "blocked")], INV)

    def test_layout_matches_runtime_contract(self):
        data, _ = pack([row(1, 0, "甲\n")], INV)
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
        self.assertEqual(total["debug"], 1)
        self.assertEqual(translatable + total["debug"] + total["empty"] + total["unaddressable"], total["total"])
        self.assertEqual(sum(total[s] for s in ("untranslated", "draft", "reviewed", "approved", "blocked")),
                         translatable)
        production = report["production"]
        self.assertEqual((production["translatable"], production["translated_draft_or_better"]), (translatable, 3))
        self.assertEqual(report["all_inventory"]["non_empty_addressable"], translatable + 1)
        game = report["categories"]["game"]
        self.assertEqual((game["runtime_seen"], game["runtime_hit"], game["runtime_fallback"]), (2, 1, 1))
        special = report["special_strings"]
        self.assertEqual((special["runtime_untranslated"], special["runtime_fallback"],
                          special["runtime_dynamic_or_invalid_keys"]), (1, 2, 2))
        self.assertEqual(report["runtime"]["untranslated_keys"], [[1, 1]])
        self.assertIn("| **total** |", l10n.coverage_markdown(report))

    def test_debug_translation_never_counts_toward_production(self):
        debug_row = dict(row(24, 0, " 層 "), category="dungeon_report")
        before = coverage(INV, [])["production"]
        after = coverage(INV, [debug_row])
        self.assertEqual(after["production"], before)
        self.assertEqual(after["all_inventory"]["debug_translated"], {"draft": 1})


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
            self.assertEqual(merged[0]["source_hash"], source_hash("Second"))
            self.assertNotIn("source", merged[0])
            stale = [dict(e, source="Old text") for e in edits if e["key"] == "1:1"]
            with self.assertRaises(DatasetError):
                merge(master, stale, INV)

    def test_drifted_row_needs_reviewed_reimport(self):
        master = [row(1, 1, "乙", source_hash=source_hash("Changed"))]
        edit, = [r for r in l10n.export_rows(INV, master) if r["key"] == "1:1"]
        self.assertEqual((edit["source"], edit["source_drift"]), ("Second", "yes"))
        edit = {k: str(v) for k, v in edit.items()}
        merged, changed = merge(master, [edit], INV)
        self.assertEqual((merged, changed), (master, 0))   # still flagged: nothing accepted
        merged, changed = merge(master, [dict(edit, source_drift="")], INV)
        self.assertEqual((merged[0]["source_hash"], changed), (source_hash("Second"), 1))
        self.assertEqual(check_master(merged, INV), ([], []))


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
