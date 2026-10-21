"""Test the real UTF-8/layout C core without SDL, FreeType or game assets."""
import ctypes as C
import csv
from itertools import chain
from pathlib import Path
import os
import random
import shutil
import subprocess
import unittest
from extract_strings import unescape

UW1 = Path(__file__).resolve().parents[2]
OUT = UW1 / "build/i18n"
MEASURE = C.CFUNCTYPE(C.c_int, C.c_uint32, C.c_void_p)


class Unit(C.Structure):
    _fields_ = [("byte_offset", C.c_size_t), ("byte_length", C.c_size_t),
                ("scalar", C.c_uint32), ("advance", C.c_int), ("kind", C.c_uint),
                ("flags", C.c_uint), ("break_before", C.c_uint)]


class Line(C.Structure):
    _fields_ = [("unit_start", C.c_size_t), ("unit_count", C.c_size_t),
                ("width", C.c_int64), ("flags", C.c_uint)]


class Layout(C.Structure):
    _fields_ = [("units", C.POINTER(Unit)), ("unit_count", C.c_size_t),
                ("lines", C.POINTER(Line)), ("line_count", C.c_size_t)]


def library():
    OUT.mkdir(parents=True, exist_ok=True)
    target = OUT / ("layout-tests.dll" if os.name == "nt" else "layout-tests.so")
    compiler = os.environ.get("CC") or shutil.which("clang")
    if not compiler:
        raise RuntimeError("Clang required; run in the Native compiler environment")
    flags = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-shared"]
    if os.name != "nt":
        flags += ["-fPIC"]
    subprocess.run([compiler] + flags + ["-I", str(UW1 / "src/port"),
                   str(UW1 / "src/port/i18n/utf8.c"), str(UW1 / "src/port/i18n/layout.c"),
                   "-o", str(target)], check=True)
    lib = C.CDLL(str(target))
    for name in ("utf8_next", "utf8_prev"):
        getattr(lib, name).argtypes = [C.c_char_p, C.c_size_t, C.POINTER(C.c_size_t), C.POINTER(C.c_uint32)]
    for name in ("utf8_validate", "utf8_count"):
        getattr(lib, name).argtypes = [C.c_char_p, C.c_size_t, C.POINTER(C.c_size_t)]
    lib.utf8_boundary_before.argtypes = [C.c_char_p, C.c_size_t, C.c_size_t]
    lib.utf8_boundary_before.restype = C.c_size_t
    lib.layout_text.argtypes = [C.c_char_p, C.c_size_t, C.c_int, MEASURE, C.c_void_p,
                               C.POINTER(Layout), C.POINTER(C.c_size_t)]
    lib.layout_free.argtypes = [C.POINTER(Layout)]
    return lib


class CoreTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = library()

    def walk(self, text):
        data = text.encode()
        pos, scalar = C.c_size_t(0), C.c_uint32(0xDEADBEEF)
        forward, boundaries = [], [0]
        while self.lib.utf8_next(data, len(data), C.byref(pos), C.byref(scalar)) == 1:
            forward.append(scalar.value)
            boundaries.append(pos.value)
        self.assertEqual(forward, list(map(ord, text)))
        self.assertEqual(pos.value, len(data))
        backward = []
        while self.lib.utf8_prev(data, len(data), C.byref(pos), C.byref(scalar)) == 1:
            backward.append(scalar.value)
        self.assertEqual(backward, forward[::-1])
        self.assertEqual(pos.value, 0)
        count, error = C.c_size_t(999), C.c_size_t(999)
        self.assertEqual(self.lib.utf8_count(data, len(data), C.byref(count)), 1)
        self.assertEqual(count.value, len(text))
        self.assertEqual(self.lib.utf8_validate(data, len(data), C.byref(error)), 1)
        self.assertEqual(error.value, len(data))
        return data, boundaries

    def test_ascii_chinese_mixed_and_empty(self):
        for text in ("", "ASCII 123", "繁體中文", "Hello，世界！", "\0a\0", "😀𠀀"):
            with self.subTest(text=text):
                self.walk(text)

    def test_scalar_encoding_boundaries(self):
        self.walk("".join(map(chr, (0, 0x7F, 0x80, 0x7FF, 0x800, 0xD7FF,
                                   0xE000, 0xFFFF, 0x10000, 0x10FFFF))))

    def test_all_unicode_scalars_validate_and_count(self):
        # Independent oracle: Python's strict encoder, every non-surrogate scalar.
        text = "".join(map(chr, chain(range(0xD800), range(0xE000, 0x110000))))
        data = text.encode()
        count = C.c_size_t()
        self.assertEqual(self.lib.utf8_validate(data, len(data), None), 1)
        self.assertEqual(self.lib.utf8_count(data, len(data), C.byref(count)), 1)
        self.assertEqual(count.value, 0x110000 - 0x800)

    def test_seeded_scalar_roundtrip(self):
        rng = random.Random(3629)
        points = [rng.randrange(0x110000) for _ in range(4096)]
        self.walk("".join(chr(p) for p in points if not 0xD800 <= p <= 0xDFFF))

    def test_invalid_and_truncated_sequences(self):
        bad = [b"\x80", b"\xc0\x80", b"\xc1\xbf", b"\xe0\x80\x80", b"\xed\xa0\x80",
               b"\xf0\x80\x80\x80", b"\xf4\x90\x80\x80", b"\xf5\x80\x80\x80", b"\xff", b"\xe4a\xad"]
        for encoded in ("¢".encode(), "中".encode(), "😀".encode()):
            bad += [encoded[:i] for i in range(1, len(encoded))]
        for suffix in bad:
            data = b"a" + suffix
            error, count = C.c_size_t(999), C.c_size_t(999)
            self.assertEqual(self.lib.utf8_validate(data, len(data), C.byref(error)), 0)
            self.assertEqual(error.value, 1)
            self.assertEqual(self.lib.utf8_count(data, len(data), C.byref(count)), 0)
            self.assertEqual(count.value, 999)
            pos, scalar = C.c_size_t(1), C.c_uint32(123)
            self.assertEqual(self.lib.utf8_next(data, len(data), C.byref(pos), C.byref(scalar)), -1)
            self.assertEqual((pos.value, scalar.value), (1, 123))

    def test_random_bytes_match_python_strict_decoder(self):
        rng = random.Random(3629)
        for _ in range(10000):
            data = rng.randbytes(rng.randrange(1, 17))
            try:
                data.decode("utf-8", errors="strict")
                expected = 1
            except UnicodeDecodeError:
                expected = 0
            self.assertEqual(self.lib.utf8_validate(data, len(data), None), expected, data.hex())

    def test_boundary_before_every_byte_and_malformed_prefix(self):
        data, boundaries = self.walk("A中文😀𠀀")
        for requested in range(len(data) + 3):
            expected = max(p for p in boundaries if p <= requested)
            self.assertEqual(self.lib.utf8_boundary_before(data, len(data), requested), expected)
        data = b"a\xe4\xb8X"
        self.assertEqual(self.lib.utf8_boundary_before(data, len(data), 999), 1)

    def test_prev_rejects_interior_and_invalid_sequence(self):
        data = "中😀".encode()
        for offset in (1, 2, 4, 5, 6):
            pos, scalar = C.c_size_t(offset), C.c_uint32(999)
            self.assertEqual(self.lib.utf8_prev(data, len(data), C.byref(pos), C.byref(scalar)), -1)
            self.assertEqual((pos.value, scalar.value), (offset, 999))
        for data in (b"\xff", b"\xc0\x80", b"\xe4\xb8"):
            pos, scalar = C.c_size_t(len(data)), C.c_uint32(999)
            self.assertEqual(self.lib.utf8_prev(data, len(data), C.byref(pos), C.byref(scalar)), -1)

    def test_utf8_null_and_cursor_contract(self):
        pos, scalar, count = C.c_size_t(0), C.c_uint32(999), C.c_size_t(999)
        self.assertEqual(self.lib.utf8_next(None, 0, C.byref(pos), C.byref(scalar)), 0)
        self.assertEqual(self.lib.utf8_prev(None, 0, C.byref(pos), C.byref(scalar)), 0)
        self.assertEqual(scalar.value, 999)
        self.assertEqual(self.lib.utf8_validate(None, 0, None), 1)
        self.assertEqual(self.lib.utf8_count(None, 0, C.byref(count)), 1)
        self.assertEqual(count.value, 0)
        self.assertEqual(self.lib.utf8_validate(None, 1, None), 0)
        self.assertEqual(self.lib.utf8_count(b"a", 1, None), 0)
        pos.value = 99
        self.assertEqual(self.lib.utf8_next(b"a", 1, C.byref(pos), C.byref(scalar)), -1)

    def render(self, text, width, metric=None):
        data = text.encode() if isinstance(text, str) else text
        output, error, calls = Layout(), C.c_size_t(), []

        @MEASURE
        def measure(scalar, context):
            calls.append(scalar)
            return metric(scalar) if metric else (1 if scalar < 128 else 2)

        result = self.lib.layout_text(data, len(data), width, measure, None, C.byref(output), C.byref(error))
        if result:
            self.assertFalse(output.units)
            self.assertFalse(output.lines)
            return result, error.value, calls
        self.assertEqual(error.value, len(data))
        units = [Unit.from_buffer_copy(output.units[i]) for i in range(output.unit_count)]
        lines = [Line.from_buffer_copy(output.lines[i]) for i in range(output.line_count)]
        self.lib.layout_free(C.byref(output))
        self.assertFalse(output.units)
        self.lib.layout_free(C.byref(output))
        self.assertEqual(b"".join(data[u.byte_offset:u.byte_offset + u.byte_length] for u in units), data)
        pos = 0
        for u in units:
            self.assertEqual(u.byte_offset, pos)
            pos += u.byte_length
            raw = data[u.byte_offset:pos]
            if u.kind == 0:
                self.assertEqual(raw.decode(), chr(u.scalar))
            elif u.kind == 1:
                self.assertIn(raw, (b"\n", b"\r", b"\r\n"))
            else:
                self.assertEqual(raw, b"\\" + bytes([u.scalar]))
                self.assertEqual(u.advance, 0)
        actual, cursor = [], 0
        for line in lines:
            self.assertEqual(line.unit_start, cursor)
            part = units[cursor:cursor + line.unit_count]
            cursor += line.unit_count
            self.assertEqual(line.width, sum(u.advance for u in part if not u.flags & 16))
            self.assertEqual(bool(line.flags & 2), line.width > width)
            actual.append("".join(chr(u.scalar) for u in part if u.kind == 0 and not u.flags & 16))
        self.assertEqual(cursor, len(units))
        for i in range(len(lines) - 1):
            if lines[i].flags & 1:
                continue  # Author's explicit breaks override soft-wrap policy.
            left = [u for u in units[lines[i].unit_start:lines[i].unit_start + lines[i].unit_count]
                    if u.kind == 0 and not u.flags & (1 | 16)]
            right = [u for u in units[lines[i + 1].unit_start:lines[i + 1].unit_start + lines[i + 1].unit_count]
                     if u.kind == 0 and not u.flags & (1 | 16)]
            if left:
                self.assertFalse(left[-1].flags & 8)
            if right:
                self.assertFalse(right[0].flags & 4)
        return actual, units, lines, calls

    def test_ascii_word_wrap_and_long_unbroken(self):
        self.assertEqual(self.render("hello world", 5)[0], ["hello", "world"])
        self.assertEqual(self.render("abcdefghijklmnopqrstuvwxyz", 5)[0], ["abcde", "fghij", "klmno", "pqrst", "uvwxy", "z"])

    def test_chinese_and_mixed(self):
        self.assertEqual(self.render("繁體中文測試", 4)[0], ["繁體", "中文", "測試"])
        self.assertEqual(self.render("UW1 中文測試", 5)[0], ["UW1", "中文", "測試"])

    def test_pixel_metrics_not_character_count(self):
        result = self.render("WiWi", 11, lambda cp: {ord("W"): 9, ord("i"): 2}[cp])
        self.assertEqual(result[0], ["Wi", "Wi"])
        self.assertEqual([line.width for line in result[2]], [11, 11])

    def test_closing_punctuation_never_begins_soft_line(self):
        for punctuation in "，。！？；：、":
            with self.subTest(punctuation=punctuation):
                actual, _, lines, _ = self.render("甲" + punctuation + "乙", 2)
                self.assertEqual(actual, ["甲" + punctuation, "乙"])
                self.assertTrue(lines[0].flags & 2)

    def test_all_bracket_pairs_and_nesting(self):
        for opening, closing in zip("（《「『【", "）》」』】"):
            with self.subTest(pair=opening + closing):
                actual, _, lines, _ = self.render("甲" + opening + "乙" + closing + "丙", 4)
                self.assertEqual(actual, ["甲", opening + "乙" + closing, "丙"])
                self.assertTrue(lines[1].flags & 2)
        self.assertEqual(self.render("甲「《乙》」丙", 6)[0], ["甲", "「《乙》」", "丙"])

    def test_ascii_punctuation_policy(self):
        self.assertEqual(self.render("a,b", 1)[0], ["a,", "b"])
        self.assertEqual(self.render("a(b)c", 2)[0], ["a", "(b)", "c"])

    def test_three_and_four_byte_units(self):
        actual, units, _, _ = self.render("中😀𠀀文", 2)
        self.assertEqual(actual, ["中", "😀", "𠀀", "文"])
        self.assertEqual([u.byte_length for u in units], [3, 4, 4, 3])

    def test_explicit_newlines_empty_and_trailing_lines(self):
        for text, expected in [("", [""]), ("a\n", ["a", ""]), ("\n\n", ["", "", ""]),
                               ("甲\r\n乙\r丙\n", ["甲", "乙", "丙", ""])]:
            with self.subTest(text=text):
                self.assertEqual(self.render(text, 20)[0], expected)

    def test_all_game_controls_are_atomic_zero_width(self):
        actual, units, lines, calls = self.render("\\0\\1\\2\\3\\4\\5\\6\\p\\P\\m", 1)
        self.assertEqual((actual, len(units), lines[0].width, calls), ([""], 10, 0, []))
        self.assertTrue(all(u.kind == 2 and u.byte_length == 2 for u in units))

    def test_controls_attach_right_and_do_not_hide_punctuation(self):
        actual, units, lines, calls = self.render("甲\\1乙\\m丙", 2)
        self.assertEqual(actual, ["甲", "乙", "丙"])
        self.assertEqual([u.kind for u in units], [0, 2, 0, 2, 0])
        self.assertEqual([line.unit_count for line in lines], [1, 2, 2])
        self.assertEqual(calls, list(map(ord, "甲乙丙")))
        self.assertEqual(self.render("甲\\2，乙", 2)[0], ["甲，", "乙"])
        self.assertEqual(self.render("甲（\\2乙）丙", 4)[0], ["甲", "（乙）", "丙"])

    def test_unknown_and_trailing_backslash_remain_glyphs(self):
        self.assertEqual(self.render("a\\q中\\", 100)[0], ["a\\q中\\"])

    def test_spaces_tabs_retained_but_trimmed_at_edges(self):
        actual, units, _, calls = self.render(" \t hello \t world \t", 5)
        self.assertEqual(actual, ["hello", "world"])
        self.assertTrue(any(u.flags & 16 for u in units))
        self.assertEqual(len(calls), len(units))
        self.assertEqual(self.render(" \t ", 1)[0], [""])
        self.assertEqual(self.render("　甲", 4)[0], ["　甲"])  # Full-width indent is retained.

    def test_malformed_nul_and_callback_failure(self):
        for data, offset in [(b"a\xe4\xb8", 1), (b"x\0z", 1), (b"\xed\xa0\x80", 0)]:
            result = self.render(data, 10)
            self.assertEqual(result, (-1, offset, []))
        self.assertEqual(self.render("ab", 10, lambda cp: -1 if cp == ord("b") else 1)[:2], (-4, 1))

    def test_argument_limits_and_zero_width_glyphs(self):
        self.assertEqual(self.render("a", 0)[0], -2)
        self.assertEqual(self.render("a" * (1024 * 1024 + 1), 1)[0], -2)
        self.assertEqual(self.render("abcdef", 1, lambda cp: 0)[0], ["abcdef"])

    def test_null_arguments_and_nonempty_output_not_modified(self):
        output, error = Layout(), C.c_size_t(99)
        measure = MEASURE(lambda cp, ctx: 1)
        self.assertEqual(self.lib.layout_text(b"a", 1, 1, MEASURE(), None, C.byref(output), C.byref(error)), -2)
        self.assertEqual(self.lib.layout_text(None, 1, 1, measure, None, C.byref(output), C.byref(error)), -2)
        self.assertEqual(self.lib.layout_text(b"a", 1, 1, measure, None, None, C.byref(error)), -2)
        self.assertEqual(self.lib.layout_text(b"a", 1, 1, measure, None, C.byref(output), C.byref(error)), 0)
        pointer = C.cast(output.units, C.c_void_p).value
        self.assertEqual(self.lib.layout_text(b"a", 1, 1, measure, None, C.byref(output), C.byref(error)), -2)
        self.assertEqual(C.cast(output.units, C.c_void_p).value, pointer)
        self.lib.layout_free(C.byref(output))

    def test_measure_callback_can_reenter_with_independent_layout(self):
        invoked = []
        def metric(cp):
            if not invoked:
                invoked.append(self.render("內層", 2)[0])
            return 2
        self.assertEqual(self.render("外層", 2, metric)[0], ["外", "層"])
        self.assertEqual(invoked, [["內", "層"]])

    def test_all_poc_translations_at_four_widths(self):
        with (UW1 / "i18n/zh-TW/poc.tsv").open(encoding="utf-8", newline="") as stream:
            rows = list(csv.DictReader(stream, delimiter="\t"))
        self.assertEqual(len(rows), 53)
        for row in rows:
            text = unescape(row["zh_tw"])
            for width in (18, 32, 64, 160):
                with self.subTest(key=row["source_key"], width=width):
                    self.render(text, width)

    def test_oversized_glyph_and_wide_accumulator(self):
        result = self.render("甲，乙", 1, lambda cp: 0x7FFFFFFF)
        self.assertEqual(result[0], ["甲，", "乙"])
        self.assertEqual(result[2][0].width, 2 * 0x7FFFFFFF)

    def test_long_text_callback_once_per_scalar(self):
        text = "中文abc " * 15000
        actual, units, lines, calls = self.render(text, 19)
        self.assertEqual(len(calls), len(text))
        self.assertEqual(len(units), len(text))
        self.assertGreater(len(lines), 1000)
        self.assertTrue(actual[-1])

    def test_seeded_mixed_wrap_invariants(self):
        rng = random.Random(303)
        alphabet = list("中文甲乙ABC abc，。！？；：、（）《》「」『』【】😀𠀀") + ["\\2", "\\m", "\n"]
        # Structured non-dangling punctuation is checked above. Random strings
        # may already start/end with illegal punctuation at hard boundaries.
        for _ in range(100):
            text = "甲" + "".join(rng.choice(alphabet) for _ in range(80)) + "乙"
            self.render(text, rng.randrange(1, 31))


if __name__ == "__main__":
    unittest.main()
