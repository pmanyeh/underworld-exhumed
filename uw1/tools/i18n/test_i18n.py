"""Asset-free tests for the inventory format, malformed input and translation guard."""
import csv
from pathlib import Path
import struct
import tempfile
import unittest

from extract_strings import (StringsPak, FormatError, category, conversation_headers,
                             escape, unescape, write_tsv)
from scan_literals import scan_source, decode_c
from validate_poc import template_tokens, validate_tokens


def fixture(strings=(b"AB|",), relative=None, padding=0):
    # A=00, B=01, |=10, FF=11. No dependency on commercial game data.
    nodes = [(65, 0, 255, 255), (66, 0, 255, 255), (124, 0, 255, 255),
             (255, 0, 255, 255), (0, 0, 0, 1), (0, 0, 2, 3), (0, 0, 4, 5)]
    tree = b"".join(bytes(n) for n in nodes)
    payload, offsets = bytearray(padding), []
    codes = {65: "00", 66: "01", 124: "10", 255: "11"}
    for text in strings:
        offsets.append(len(payload))
        bits = "".join(codes[c] for c in text)
        bits += "0" * (-len(bits) % 8)
        payload += bytes(int(bits[i:i + 8], 2) for i in range(0, len(bits), 8))
    if relative is not None:
        offsets = relative
    start = 2 + len(tree) + 2 + 6
    return (struct.pack("<H", 7) + tree + struct.pack("<HHI", 1, 1, start) +
            struct.pack("<H", len(offsets)) + struct.pack(f"<{len(offsets)}H", *offsets) + payload)


class DecoderTests(unittest.TestCase):
    def test_msb_offsets_and_both_terminators(self):
        pak = StringsPak(fixture((b"AB|", b"BA\xff", b"|")))
        self.assertEqual([pak.read(1, i) for i in range(3)], [b"AB", b"BA", b""])

    def test_missing_keys(self):
        pak = StringsPak(fixture())
        self.assertEqual([pak.read(2, 0), pak.read(1, -1), pak.read(1, 1)], [b""] * 3)

    def test_game_512_character_limit(self):
        pak = StringsPak(fixture((b"A" * 512 + b"|",)))
        self.assertEqual(pak.read(1, 0), b"A" * 511)

    def test_high_bit_relative(self):
        self.assertEqual(StringsPak(fixture(padding=0x8000)).read(1, 0), b"AB")

    def test_uint16_seek_wrap(self):
        # Remaining table bytes + FFFE wraps to zero, reads the next offset's
        # low byte (80: immediate |), rather than seeking forward 64 KiB.
        pak = StringsPak(fixture((b"AB|", b"AB|"), relative=[0xFFFE, 0x80]))
        self.assertEqual(pak.read(1, 0), b"")

    def test_truncated_stream(self):
        pak = StringsPak(fixture()[:-1])
        with self.assertRaises(FormatError):
            pak.read(1, 0)

    def test_truncated_headers_and_tables(self):
        data = fixture()
        for length in (0, 1, 20, 32, 37, 39, 41):
            with self.subTest(length=length), self.assertRaises(FormatError):
                StringsPak(data[:length])

    def test_invalid_node_count(self):
        for n in (0, 257):
            with self.assertRaises(FormatError):
                StringsPak(struct.pack("<H", n))

    def test_cycle_and_invalid_child(self):
        for child in (6, 250):
            data = bytearray(fixture())
            data[2 + 6 * 4 + 2] = child
            with self.assertRaises(FormatError):
                StringsPak(data)

    def test_directory_overlap(self):
        data = bytearray(fixture())
        struct.pack_into("<I", data, 34, 0)
        with self.assertRaises(FormatError):
            StringsPak(data)

    def test_signed_game_header_limits(self):
        for offset, fmt, value in ((30, "<H", 0x8000), (32, "<H", 0x8000),
                                   (34, "<I", 0x80000000), (38, "<H", 0x8000)):
            data = bytearray(fixture())
            struct.pack_into(fmt, data, offset, value)
            with self.subTest(offset=offset), self.assertRaises(FormatError):
                StringsPak(data)

    def test_duplicate_block(self):
        tree = fixture()[2:30]
        data = struct.pack("<H", 7) + tree + struct.pack("<H", 2)
        data += struct.pack("<HIHI", 1, 44, 1, 44) + b"\0\0"
        with self.assertRaises(FormatError):
            StringsPak(data)

    def test_categories(self):
        self.assertEqual([category(b) for b in (1, 4, 24, 0xC00, 0xE43)],
                         ["game", "object_names", "unknown_static", "cutscene", "conversation"])

    def test_conversation_uses_header_not_slot(self):
        data = struct.pack("<HI", 1, 6) + struct.pack("<IIHHHH", 0, 1, 0, 3651, 35, 0) + b"\0\0"
        row, = conversation_headers(data)
        self.assertEqual((row["slot"], row["resolved_block"], row["code_words"]), (0, 3651, 1))

    def test_bad_conversation_bounds(self):
        for data in (b"\1\0", struct.pack("<HI", 1, 3), struct.pack("<HI", 1, 999)):
            with self.assertRaises(FormatError):
                conversation_headers(data)


class TextTests(unittest.TestCase):
    def test_escape_roundtrip_and_invalid_escape(self):
        text = '\\p\\m\\6\n\r\t\0"繁體中文'
        self.assertEqual(unescape(escape(text)), text)
        for bad in ("\\", "\\x", "\\p"):
            with self.assertRaises(ValueError):
                unescape(bad)

    def test_tsv_roundtrip(self):
        text = '"quoted"\t繁體中文\n\\m'
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "test.tsv"
            write_tsv(path, [{"source_en": text}], ["source_en"])
            with path.open(encoding="utf-8", newline="") as stream:
                row, = csv.DictReader(stream, delimiter="\t")
            self.assertEqual(unescape(row["source_en"]), text)
            self.assertEqual(path.read_bytes().count(b"\n"), 2)

    def test_lexer_comments_chars_and_adjacent_literals(self):
        source = '''// scroll_print("fake")
        char quote = '"'; /* "fake" */
        scroll_print("Hello " /* x */ "world\\n");
        plat_message(1, "title", "message");'''
        rows = scan_source("uw1/src/ui/X.C", source)
        self.assertEqual([r["source_en"] for r in rows], ["Hello world\n", "title", "message"])
        self.assertEqual([r["classification"] for r in rows], ["PLAYER_VISIBLE", "PORT_UI", "PORT_UI"])

    def test_literal_key_stable_across_line_change(self):
        a = scan_source("uw1/src/ui/X.C", 'scroll_print("X");')[0]
        b = scan_source("uw1/src/ui/X.C", '\n\nscroll_print("X");')[0]
        self.assertEqual(a["source_key"], b["source_key"])
        self.assertEqual(b["line"], 3)

    def test_c_escapes_and_line_splice(self):
        self.assertEqual(decode_c('"\\141\\x42\\n\\\\m"'), 'aB\n\\m')
        rows = scan_source("uw1/src/ui/X.C", 'scroll_print("a\\\nb");\nscroll_print("c");')
        self.assertEqual([r["source_en"] for r in rows], ["ab", "c"])
        self.assertEqual(rows[1]["line"], 3)

    def test_classification_conservative(self):
        src = 'port_log("trace"); printf("maybe visible"); const char *x = "named UI?";'
        self.assertEqual([r["classification"] for r in scan_source("uw1/src/X.C", src)],
                         ["DEBUG_ONLY", "UNKNOWN", "UNKNOWN"])
        self.assertEqual(scan_source("uw1/tools/i18n/probe.c", 'printf("x");')[0]["classification"], "TEST_ONLY")

    def test_templates_controls_and_nested_indexes(self):
        validate_tokens('Hi @SS1 @GI2C3 @@\\m\n', '你好 @GI2C3 @SS1 @@\\m\n')
        self.assertEqual(template_tokens('@GS8'), {'@GS8': 1})
        for source, target in [('Hi @SS1', '你好'), ('@SS1', '@SS2'), ('a\\m', 'a'),
                               ('\\1a\\2', '\\2a\\1'), ('a\n', 'a'), ('@ZZ1', '@ZZ1'),
                               ('@SS1', '@SS1@SS1')]:
            with self.subTest(source=source), self.assertRaises(ValueError):
                validate_tokens(source, target)


if __name__ == "__main__":
    unittest.main()
