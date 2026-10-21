#!/usr/bin/env python3
"""Lossless, deterministic UW1 text inventory; no game data is bundled.

Byte semantics follow src/ui/GAMESTRN.C, including its uint16 seek and
511-byte result limit. TSV fields use reversible backslash escapes, not C
or Python string evaluation. Full inventories belong in ignored build/.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import struct


class FormatError(ValueError):
    pass


def unpack(data, fmt, offset):
    size = struct.calcsize(fmt)
    if offset < 0 or offset + size > len(data):
        raise FormatError(f"truncated {fmt} at {offset:#x}")
    return struct.unpack_from(fmt, data, offset)


def escape(text):
    return text.replace("\\", "\\\\").replace("\t", "\\t").replace("\r", "\\r").replace("\n", "\\n").replace("\0", "\\0")


def unescape(text):
    result, i = [], 0
    codes = {"\\": "\\", "t": "\t", "r": "\r", "n": "\n", "0": "\0"}
    while i < len(text):
        c = text[i]
        i += 1
        if c == "\\":
            if i == len(text) or text[i] not in codes:
                raise ValueError("invalid TSV escape")
            c = codes[text[i]]
            i += 1
        result.append(c)
    return "".join(result)


CATEGORIES = {1: "game", 2: "chargen", 3: "books", 4: "object_names",
              5: "object_look", 6: "spells", 7: "conversation_common",
              8: "writings", 9: "trap_text", 10: "textures"}


def category(block):
    if block in CATEGORIES:
        return CATEGORIES[block]
    if 0xC00 <= block < 0xE00:
        return "cutscene"
    if block >= 0xE00:
        return "conversation"
    return "unknown_static"


class StringsPak:
    def __init__(self, data):
        self.data = data
        n, = unpack(data, "<H", 0)
        if not 1 <= n <= 256:
            raise FormatError("Huffman node count outside 1..256")
        self.nodes = [unpack(data, "<BBBB", 2 + 4 * i) for i in range(n)]
        self.root = n - 1
        # Validate every node, including unreachable ones, before reading bits.
        done, visiting = set(), set()

        def visit(index):
            if index >= n:
                raise FormatError("Huffman child outside tree")
            if index in visiting:
                raise FormatError("cyclic Huffman tree")
            if index in done:
                return
            visiting.add(index)
            _, _, left, right = self.nodes[index]
            if left != 255:
                visit(left)
                visit(right)
            visiting.remove(index)
            done.add(index)

        for i in range(n):
            visit(i)
        pos = 2 + 4 * n
        count, = unpack(data, "<H", pos)
        if count > 0x7FFF:
            raise FormatError("block count exceeds GAMESTRN signed word")
        self.blocks = {}
        directory_end = pos + 2 + 6 * count
        for i in range(count):
            block, start = unpack(data, "<HI", pos + 2 + 6 * i)
            if block > 0x7FFF or start > 0x7FFFFFFF:
                raise FormatError("block identity/offset exceeds GAMESTRN signed fields")
            if block in self.blocks:
                raise FormatError(f"duplicate block {block}")
            if start < directory_end:
                raise FormatError("block overlaps directory")
            length, = unpack(data, "<H", start)
            if length > 0x7FFF:
                raise FormatError("string count exceeds GAMESTRN signed word")
            unpack(data, f"<{length}H", start + 2)
            self.blocks[block] = (start, length)

    def read(self, block, index):
        """Return the bytes visible to C callers (up to the first NUL)."""
        if block not in self.blocks or index < 0 or index >= self.blocks[block][1]:
            return b""
        start, count = self.blocks[block]
        entry = start + 2 + 2 * index
        relative, = unpack(self.data, "<H", entry)
        pos = entry + 2 + ((2 * (count - index - 1) + relative) & 0xFFFF)
        bit = pos * 8
        result = bytearray()
        for _ in range(512):
            node = self.root
            while self.nodes[node][2] != 255:
                if bit >= len(self.data) * 8:
                    raise FormatError(f"truncated string {block}:{index}")
                right = (self.data[bit // 8] >> (7 - bit % 8)) & 1
                bit += 1
                node = self.nodes[node][2 + right]
            c = self.nodes[node][0]
            if c in (ord("|"), 255):
                return bytes(result).split(b"\0", 1)[0]
            result.append(c)
        # GAMESTRN always overwrites the last decoded byte with NUL.
        return bytes(result[:-1]).split(b"\0", 1)[0]

    def rows(self):
        for block in sorted(self.blocks):
            for index in range(self.blocks[block][1]):
                yield {"block": block, "index": index, "category": category(block),
                       "source_en": self.read(block, index).decode("cp437")}


def conversation_headers(data):
    """Read actual CNV.ARK headers; never infer the string block from whoami."""
    count, = unpack(data, "<H", 0)
    offsets = unpack(data, f"<{count}I", 2)
    result = []
    for slot, start in enumerate(offsets):
        if not start:
            continue
        end = min([o for o in offsets if o > start] + [len(data)])
        if start < 2 + 4 * count or end > len(data):
            raise FormatError("conversation outside archive")
        chunk = data[start:end]
        _, words, flags, block, variables, imports = unpack(chunk, "<IIHHHH", 0)
        pos, names = 16, []
        for _ in range(imports):
            size, = unpack(chunk, "<H", pos)
            pos += 2
            unpack(chunk, f"<{size}s", pos)
            name = chunk[pos:pos + size].rstrip(b"\0").decode("ascii")
            pos += size
            index, n, kind, typ = unpack(chunk, "<HHHH", pos)
            pos += 8
            names.append({"name": name, "index": index, "count": n, "kind": kind, "type": typ})
        unpack(chunk, f"<{words}H", pos)
        result.append({"slot": slot, "resolved_block": block, "code_words": words,
                       "variables": variables, "flags": flags, "imports": names})
    return result


def write_tsv(path, rows, fields):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fields, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        for row in rows:
            writer.writerow({k: escape(str(row[k])) for k in fields})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pak", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--cnv", type=Path)
    parser.add_argument("--metadata", type=Path)
    args = parser.parse_args()
    data = args.pak.read_bytes()
    pak = StringsPak(data)
    rows = list(pak.rows())
    write_tsv(args.output, rows, ["block", "index", "category", "source_en"])
    metadata = {"pak_sha256": hashlib.sha256(data).hexdigest(), "encoding": "cp437",
                "strings": len(rows), "blocks": {str(b): n for b, (_, n) in sorted(pak.blocks.items())}}
    if args.cnv:
        cnv = args.cnv.read_bytes()
        metadata["cnv_sha256"] = hashlib.sha256(cnv).hexdigest()
        metadata["conversations"] = conversation_headers(cnv)
    if args.metadata:
        args.metadata.parent.mkdir(parents=True, exist_ok=True)
        args.metadata.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    print(f"{len(rows)} strings, {len(pak.blocks)} blocks -> {args.output}")


if __name__ == "__main__":
    main()
