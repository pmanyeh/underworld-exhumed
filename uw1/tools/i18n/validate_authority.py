#!/usr/bin/env python3
"""Validate the terminology authority without changing game translations.

No game assets or manual images are needed. Optional --guide checks the supplied
guide's hash and line references; --workbook verifies keys against a local export.
"""
import argparse
from collections import Counter
import csv
import hashlib
import json
from pathlib import Path
import re

UW1 = Path(__file__).resolve().parents[2]


def read_tsv(path):
    with path.open(encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if not reader.fieldnames or len(reader.fieldnames) != len(set(reader.fieldnames)):
            raise ValueError(f"Invalid header: {path.name}")
        rows = list(reader)
    if any(None in row or any(value is None for value in row.values()) for row in rows):
        raise ValueError(f"Invalid column count: {path.name}")
    return rows


def require(condition, message):
    if not condition:
        raise ValueError(message)


def validate(root, guide=None, workbook=None):
    rows = read_tsv(root / "GLOSSARY.tsv")
    by_id = {row["term_id"]: row for row in rows}
    by_identity = {(row["category"], row["source_en"].casefold()): row for row in rows}
    require(len(rows) == len(by_id) == len(by_identity), "Duplicate term identity")
    guide_sources = {row["source_id"]: row for row in read_tsv(root / "GUIDE-SOURCES.tsv")}
    public_sources = {row["source_id"] for row in read_tsv(root / "PUBLIC-REFERENCES.tsv")}
    manual_sources = {row["source_id"] for row in read_tsv(root / "MANUAL-SOURCES.tsv")}
    decision_sources = {row["source_id"] for row in read_tsv(root / "USER-DECISION-SOURCES.tsv")}
    decisions = read_tsv(root / "USER-DECISIONS.tsv")
    decision_by_id = {row["term_id"]: row for row in decisions}
    require(len(decisions) == len(decision_by_id), "Multiple current human decisions for one term")
    key_pattern = re.compile(r"[1-9][0-9]*:[0-9]+")
    aliases = {}
    for row in rows:
        ident = row["category"] + "\0" + re.sub(r"\s+", " ", row["source_en"].strip()).casefold()
        expected = "UW1-" + hashlib.sha256(ident.encode()).hexdigest()[:12].upper()
        require(row["term_id"] == expected, "Unstable term ID")
        require(bool(row["canonical_zh_tw"].strip()), "Empty selected translation")
        require(row["status"] in {"project-selected", "review-needed", "approved"}, "Invalid status")
        require(row["source_scope"] in {"game-text", "guide-only"}, "Invalid source scope")
        game_keys = set(filter(None, row["game_keys"].split(";")))
        direct_keys = set(filter(None, row["direct_keys"].split(";")))
        require(all(key_pattern.fullmatch(key) for key in game_keys), "Invalid game key")
        require(direct_keys <= game_keys, "Direct keys absent from context keys")
        require(row["source_scope"] != "guide-only" or not game_keys, "Guide-only entry claims game keys")
        require(row["source_scope"] != "game-text" or bool(game_keys), "Game entry lacks evidence")
        for field, known in [("guide_source_ids", guide_sources), ("public_source_ids", public_sources),
                             ("manual_source_ids", manual_sources), ("decision_source_ids", decision_sources)]:
            require(set(filter(None, row[field].split(";"))) <= set(known), "Unknown source ID")
        require(bool(row["guide_source_ids"]) == bool(row["guide_lines"]), "Incomplete guide reference")
        require(all(re.fullmatch(r"[1-9][0-9]*", line) for line in filter(None, row["guide_lines"].split(";"))),
                "Invalid guide line")
        require("manual_variants" not in row and "manual_preferred_zh_tw" not in row,
                "Raw manual working columns in publication")
        require("魔魔法師" not in row["canonical_zh_tw"], "Repeated compound root")
        if row["status"] == "approved":
            require(row["term_id"] in decision_by_id, "Approval lacks human decision")
            decision = decision_by_id[row["term_id"]]
            require(decision["status"] == "approved" and decision["source_en"] == row["source_en"] and
                    decision["canonical_zh_tw"] == row["canonical_zh_tw"], "Approved value differs from human decision")
            require(row["decision_ids"] == decision["decision_id"] and
                    row["decision_source_ids"] == decision["source_id"], "Approval provenance differs from decision")
            policy = decision["entity_policy"]
            if policy:
                kind, other_id = policy.split(":", 1)
                require(other_id in by_id and other_id != row["term_id"], "Invalid human entity relation")
                other = by_id[other_id]
                if kind == "same-translation-as":
                    require(row["canonical_zh_tw"] == other["canonical_zh_tw"], "User-selected translations diverged")
                elif kind == "provisional-separate-from":
                    require(other["source_en"].casefold() not in row["aliases_en"].casefold().split(" ; "),
                            "Temporarily separate people merged by alias")
                else:
                    require(False, "Unknown human entity policy")
        for alias in filter(None, row["aliases_en"].split(" ; ")):
            key = (row["category"], alias.casefold())
            owner = by_identity.get(key)
            require(not owner or owner["term_id"] == row["term_id"], "Alias shadows a primary term")
            require(key not in aliases or aliases[key] == row["term_id"], "Ambiguous English alias")
            aliases[key] = row["term_id"]
    expected_views = {
        "PROPER-NOUNS.tsv": {"person", "place", "race", "faction", "organization", "title", "creature", "world-lore"},
        "SPELLS.tsv": {"spell", "rune", "equipment"},
        "OBJECTS.tsv": {"object", "weapon", "armor", "quest-object", "dungeon-term"},
    }
    for filename, categories in expected_views.items():
        require(read_tsv(root / filename) == [row for row in rows if row["category"] in categories],
                f"Outdated derived view: {filename}")
    runes = [row for row in rows if row["term_kind"] == "rune-token"]
    mantras = [row for row in rows if row["term_kind"] == "mantra-token"]
    require(len(runes) == 24 and len(mantras) == 26, "Input-token coverage changed")
    require(all(row["canonical_zh_tw"].startswith(row["source_en"] + "（") for row in runes),
            "Rune identifier translated")
    require(all(row["canonical_zh_tw"] == row["source_en"] for row in mantras), "Mantra identifier translated")
    require(all(row["translation_policy"] == "preserve-input-token" for row in runes + mantras),
            "Input-token policy changed")
    require(sum(row["term_kind"] == "runestone" for row in rows) == 24, "Runestone coverage changed")
    require(sum(row["category"] == "skill" for row in rows) == 20, "Skill coverage changed")
    require(sum(row["category"] == "virtue" for row in rows) == 8, "Eight virtues incomplete")
    alignment = read_tsv(root / "GUIDE-ALIGNMENT.tsv")
    for entry in alignment:
        require(entry["term_id"] in by_id, "Alignment references absent term")
        current = by_id[entry["term_id"]]
        require(current["canonical_zh_tw"] == entry["selected_zh_tw"], "Stale alignment choice")
        require(current["category"] == entry["category"], "Guide crossed term category")
        require(entry["guide_line"] in current["guide_lines"].split(";"), "Guide line missing from term")

    # Verify spell identity from primary code, not similar-looking English names.
    source = (UW1 / "src/combat/SPELLS.C").read_text(encoding="utf-8")
    table = re.search(r"struct Spell far spells\[NUM_SPELLS\] = \{(.*?)\n\};", source, re.S)
    require(bool(table), "Spell table not found")
    packed = re.findall(r"\{\s*(0x[0-9A-Fa-f]+),\s*(0x[0-9A-Fa-f]+),\s*(0x[0-9A-Fa-f]+)\s*\}", table[1])
    require(len(packed) == 53, "Spell table layout changed; review validator")
    rune_order = "An Bet Corp Des Ex Flam Grav Hur In Jux Kal Lor Mani Nox Ort Por Quas Rel Sanct Tym Uus Vas Wis Ylem".split()
    checked_spells = 0
    for entry in alignment:
        if not entry["guide_circle"]:
            continue
        row = by_id[entry["term_id"]]
        spell_indices = [int(key.split(":")[1]) - 256 for key in row["direct_keys"].split(";")
                         if key.startswith("6:") and 256 <= int(key.split(":")[1]) < 304]
        require(len(spell_indices) == 1, "Guide spell lacks an unambiguous player-spell identity")
        index = spell_indices[0]
        runes_value = int(packed[index][1], 16)
        components = [(runes_value >> shift) & 31 for shift in (10, 5, 0)]
        require(all(part <= 24 for part in components), "Invalid packed rune")
        actual = " ".join(rune_order[part] for part in components if part < 24)
        require(actual == entry["guide_runes"], "Guide rune sequence differs from game: " + row["source_en"])
        require(index // 6 + 1 == int(entry["guide_circle"]), "Guide circle differs from game")
        checked_spells += 1
    require(checked_spells == 40, "Expected the guide's 40 named rune spells")
    if guide:
        digest = hashlib.sha256(guide.read_bytes()).hexdigest()
        require(any(row["sha256"] == digest for row in guide_sources.values()), "Guide source hash drift")
        lines = guide.read_text(encoding="utf-8-sig").splitlines()
        for row in rows:
            for number in filter(None, row["guide_lines"].split(";")):
                require(int(number) <= len(lines), "Guide line outside source")
        for entry in alignment:
            source_line = lines[int(entry["guide_line"]) - 1]
            if entry["decision"] == "guide-range-expansion":
                require("Level 1 ~ Level 8" in source_line, "Range expansion lacks source")
            else:
                require(all(en in source_line for en in entry["guide_english"].split(" / ")),
                        "Guide English label absent from recorded line")
    if workbook:
        keys = {row["key"] for row in read_tsv(workbook)}
        require(all(set(filter(None, row["game_keys"].split(";"))) <= keys for row in rows),
                "Game key absent from local inventory")
    summary = json.loads((root / "SUMMARY.json").read_text(encoding="utf-8"))
    counts = Counter(row["category"] for row in rows)
    require(summary["terms"] == len(rows) and summary["categories"] == dict(sorted(counts.items())),
            "Stale summary counts")
    require(summary["guide_mapping_records"] == len(alignment), "Stale alignment count")
    require(summary["guide_changed_existing_terms"] == len(read_tsv(root / "GUIDE-CHANGES.tsv")),
            "Stale change count")
    statuses = Counter(row["status"] for row in rows)
    for status in ("project-selected", "review-needed"):
        require(summary[status.replace("-", "_")] == statuses[status], "Stale status count")
    require(summary["canonical_human_approved"] == statuses["approved"], "Stale approval count")
    require(summary["human_decision_records"] == len(decisions), "Stale human decision count")
    require(set(decision_by_id) == {row["term_id"] for row in rows if row["status"] == "approved"},
            "Human decision not applied")
    pending = read_tsv(root / "REVIEW-NEEDED.tsv")
    require({row["term_id"] for row in pending} ==
            {row["term_id"] for row in rows if row["status"] == "review-needed"},
            "Stale review queue")
    require(all(row["canonical_zh_tw"] == by_id[row["term_id"]]["canonical_zh_tw"] for row in pending),
            "Stale review choice")
    require(statuses["review-needed"] == 0, "Authority has unresolved review-needed terms")
    return {"status": "PASS", "terms": len(rows), "categories": len(counts),
            "guide_spell_sequences_verified": checked_spells, "runes": len(runes),
            "mantras": len(mantras), "skills": 20, "virtues": 8,
            "guide_only_terms": sum(row["source_scope"] == "guide-only" for row in rows),
            "project_selected": statuses["project-selected"],
            "review_needed": statuses["review-needed"], "human_approved": statuses["approved"],
            "human_decisions": "PASS", "derived_views": "PASS"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=UW1 / "i18n/zh-TW/authority")
    parser.add_argument("--guide", type=Path)
    parser.add_argument("--workbook", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    try:
        result = validate(args.root, args.guide, args.workbook)
    except (ValueError, KeyError, OSError) as error:
        parser.exit(1, f"Authority validation failed: {error}\n")
    if args.report:
        args.report.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False))


if __name__ == "__main__":
    main()
