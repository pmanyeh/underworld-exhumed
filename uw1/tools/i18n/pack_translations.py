#!/usr/bin/env python3
"""Pack the master translation dataset into the runtime catalog without game data.

Row-level validation only (keys, status, UTF-8, controls, templates); source
drift against the player's STRINGS.PAK needs `l10n.py build`, the production
path. The catalog is a generated build artifact; do not edit or commit it.
"""
import argparse
from pathlib import Path

from l10n import DatasetError, MASTER, PACKED, check_master, load_master, pack as pack_rows


def pack(path=MASTER, min_status="draft"):
    rows = load_master(path)
    problems, _ = check_master(rows)
    if problems:
        raise DatasetError(problems)
    data, count = pack_rows(rows, min_status)
    return data, count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("master", type=Path, nargs="?", default=MASTER)
    parser.add_argument("output", type=Path)
    parser.add_argument("--min-status", choices=list(PACKED), default="draft")
    args = parser.parse_args()
    data, count = pack(args.master, args.min_status)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"{count} static entries, {len(data)} bytes -> {args.output}")


if __name__ == "__main__":
    main()
