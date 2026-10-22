#!/usr/bin/env python3
"""Pack the master translation dataset into the runtime catalog.

The master keeps only source hashes, so the English bytes the runtime checks
come from the player's STRINGS.PAK; every row must match its source_hash.
Same as `l10n.py build` without the literal checks. The catalog is a generated
build artifact; do not edit or commit it.
"""
import argparse
from pathlib import Path

from l10n import DatasetError, Inventory, MASTER, PACKED, check_master, load_master, pack as pack_rows


def pack(pak, path=MASTER, min_status="draft"):
    inventory = Inventory.load(pak)
    rows = load_master(path)
    problems, _ = check_master(rows, inventory)
    if problems:
        raise DatasetError(problems)
    return pack_rows(rows, inventory, min_status)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pak", type=Path, required=True, help="the player's DATA/STRINGS.PAK")
    parser.add_argument("master", type=Path, nargs="?", default=MASTER)
    parser.add_argument("output", type=Path)
    parser.add_argument("--min-status", choices=list(PACKED), default="draft")
    args = parser.parse_args()
    data, count = pack(args.pak, args.master, args.min_status)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"{count} static entries, {len(data)} bytes -> {args.output}")


if __name__ == "__main__":
    main()
