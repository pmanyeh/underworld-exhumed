"""Compile/test the real native catalog module without SDL or original assets.

Run in the native compiler environment: python3 tools/i18n/test_runtime.py -v
"""
import ctypes
import csv
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest

from extract_strings import unescape, write_tsv
from pack_translations import MAGIC, pack

UW1 = Path(__file__).resolve().parents[2]
OUT = UW1 / "build/i18n"


def catalog(rows):
    data = MAGIC + struct.pack("<I", len(rows))
    for b, i, source, target in rows:
        data += struct.pack("<HHII", b, i, len(source), len(target)) + source + b"\0" + target + b"\0"
    return data


class RuntimeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        OUT.mkdir(parents=True, exist_ok=True)
        library = OUT / ("lookup-tests.dll" if os.name == "nt" else "lookup-tests.so")
        compiler = os.environ.get("CC") or shutil.which("clang")
        if not compiler:
            raise RuntimeError("run in the Native compiler environment (Clang required)")
        command = [compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-shared"]
        if os.name != "nt":
            command += ["-fPIC"]
        subprocess.run(command + ["-I", str(UW1 / "src/port"), str(UW1 / "src/port/i18n/localization.c"),
                                  "-o", str(library)], check=True)
        cls.lib = ctypes.CDLL(str(library))
        cls.lib.port_i18n_init.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        cls.lib.port_i18n_init.restype = ctypes.c_int
        cls.lib.port_i18n_lookup.argtypes = [ctypes.c_uint, ctypes.c_uint, ctypes.c_void_p]
        cls.lib.port_i18n_lookup.restype = ctypes.c_void_p
        cls.lib.port_i18n_observe.argtypes = cls.lib.port_i18n_lookup.argtypes
        cls.lib.port_i18n_observe.restype = ctypes.c_void_p
        cls.lib.port_i18n_trace_open.argtypes = [ctypes.c_char_p]
        cls.lib.port_i18n_shutdown.restype = None

    def setUp(self):
        self.lib.port_i18n_shutdown()
        self.tmp = tempfile.TemporaryDirectory(dir=OUT)
        self.path = Path(self.tmp.name) / "test.uwloc"
        self.data = catalog([(3651, 3, b"Hello", "你好，@SS1！".encode())])
        self.path.write_bytes(self.data)

    def tearDown(self):
        self.lib.port_i18n_shutdown()
        self.tmp.cleanup()

    def init(self, data=None, locale=b"zh-TW"):
        if data is not None:
            self.path.write_bytes(data)
        return self.lib.port_i18n_init(locale, os.fsencode(self.path))

    def lookup(self, source=b"Hello", block=3651, index=3):
        buffer = ctypes.create_string_buffer(source)
        pointer = self.lib.port_i18n_lookup(block, index, buffer)
        return buffer, pointer

    def assert_bad(self, data):
        self.assertEqual(self.init(data), -1)
        self.assertEqual(self.lib.port_i18n_enabled(), 0)

    def test_locale_off_and_null(self):
        for locale in (None, b"en"):
            self.assertEqual(self.lib.port_i18n_init(locale, b"does-not-exist"), 0)
            buffer, pointer = self.lookup()
            self.assertEqual(pointer, ctypes.addressof(buffer))
        self.assertIsNone(self.lib.port_i18n_lookup(3651, 3, None))

    def test_unsupported_or_missing_catalog(self):
        self.assertEqual(self.lib.port_i18n_init(b"zh-TW", None), -1)
        self.assertEqual(self.lib.port_i18n_init(b"zh-TW", b"does-not-exist"), -1)
        self.assertEqual(self.init(locale=b"fr"), -1)

    def test_load_actual_poc_all_52_keys(self):
        data, n, skipped = pack(UW1 / "i18n/zh-TW/poc.tsv")
        self.assertEqual((n, skipped), (52, 1))
        self.assertEqual(self.init(data), 0)
        self.assertEqual(self.lib.port_i18n_enabled(), 1)
        with (UW1 / "i18n/zh-TW/poc.tsv").open(encoding="utf-8", newline="") as stream:
            for raw in csv.DictReader(stream, delimiter="\t"):
                row = {k: unescape(v) for k, v in raw.items()}
                if row["kind"] != "pak":
                    continue
                buffer, pointer = self.lookup(row["source_en"].encode("cp437"), int(row["block"]), int(row["index"]))
                self.assertNotEqual(pointer, ctypes.addressof(buffer))
                self.assertEqual(ctypes.string_at(pointer), row["zh_tw"].encode())

    def test_missing_drift_and_dynamic_keys_return_pointer(self):
        self.assertEqual(self.init(), 0)
        for source, block, index in [(b"Changed source", 3651, 3), (b"Hello", 3651, 4),
                                     (b"Hello", 0, 3), (b"Hello", 0x7C, 3),
                                     (b"Hello", 0x7D, 3), (b"Hello", 3651, 512)]:
            buffer, pointer = self.lookup(source, block, index)
            self.assertEqual(pointer, ctypes.addressof(buffer))
        self.assertIsNone(self.lib.port_i18n_lookup(3651, 3, None))

    def test_stable_immutable_result_and_long_utf8(self):
        target = ("繁體中文" * 300).encode()
        self.assertEqual(self.init(catalog([(3651, 3, b"Hello", target)])), 0)
        buffer, pointer = self.lookup()
        for _ in range(32):
            other, other_pointer = self.lookup()
            self.assertEqual(pointer, other_pointer)
            self.assertEqual(ctypes.string_at(other), b"Hello")
        self.assertEqual(ctypes.string_at(pointer), target)
        self.assertEqual(ctypes.string_at(buffer), b"Hello")

    def test_reinitialize_and_shutdown(self):
        self.assertEqual(self.init(), 0)
        self.assertEqual(self.init(locale=b"en"), 0)
        buffer, pointer = self.lookup()
        self.assertEqual(pointer, ctypes.addressof(buffer))
        self.assertEqual(self.init(), 0)
        self.assert_bad(b"bad")
        self.lib.port_i18n_shutdown()
        self.lib.port_i18n_shutdown()

    def test_truncation_every_byte_and_bad_magic(self):
        for size in range(len(self.data)):
            with self.subTest(size=size):
                self.assert_bad(self.data[:size])
        self.assert_bad(b"BADLOC1\0" + self.data[8:])

    def test_counts_and_file_limits(self):
        for n in (0, 4097, 0xFFFFFFFF):
            self.assert_bad(MAGIC + struct.pack("<I", n))
        self.assert_bad(self.data + b"x")
        self.assert_bad(b"x" * (4 * 1024 * 1024 + 1))

    def test_sorted_unique_static_keys_only(self):
        for b, i in [(0, 3), (0x7C, 3), (0x7D, 3), (0x8000, 3), (3651, 512)]:
            self.assert_bad(catalog([(b, i, b"Hello", b"target")]))
        row = (3651, 3, b"Hello", b"target")
        self.assert_bad(catalog([row, row]))
        self.assert_bad(catalog([(3651, 4, b"Hello", b"target"), row]))

    def test_invalid_utf8_and_embedded_nul(self):
        for target in (b"", b"a\0b", b"\x80", b"\xc0\xaf", b"\xe0\x80\x80", b"\xed\xa0\x80",
                       b"\xf4\x90\x80\x80", b"\xf5\x80\x80\x80", b"\xe4\xb8", b"\xff"):
            self.assert_bad(catalog([(3651, 3, b"Hello", target)]))
        for source in (b"", b"a\0b", b"a" * 512):
            self.assert_bad(catalog([(3651, 3, source, b"target")]))
        self.assert_bad(catalog([(3651, 3, b"Hello", b"a" * 16385)]))

    def test_valid_utf8_boundaries(self):
        target = "\u007f\u0080\u07ff\u0800\ud7ff\ue000\uffff\U00010000\U0010ffff".encode()
        self.assertEqual(self.init(catalog([(1, 0, b"a" * 511, target)])), 0)
        buffer, pointer = self.lookup(b"a" * 511, 1, 0)
        self.assertEqual(ctypes.string_at(pointer), target)

    def test_trace_deduplicates_and_preserves_utf8(self):
        self.assertEqual(self.init(), 0)
        path = Path(self.tmp.name) / "trace.jsonl"
        self.assertEqual(self.lib.port_i18n_trace_open(os.fsencode(path)), 0)
        for _ in range(3):
            buffer = ctypes.create_string_buffer(b"Hello")
            pointer = self.lib.port_i18n_observe(3651, 3, buffer)
            self.assertEqual(pointer, ctypes.addressof(buffer))
        self.assertEqual([json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()],
                         [{"block": 3651, "index": 3, "text": "你好，@SS1！"}])

    def test_trace_json_control_escaping(self):
        target = '中文"\\m\n\t\r'.encode()
        self.assertEqual(self.init(catalog([(3651, 3, b"Hello", target)])), 0)
        path = Path(self.tmp.name) / "trace.jsonl"
        self.assertEqual(self.lib.port_i18n_trace_open(os.fsencode(path)), 0)
        self.lib.port_i18n_observe(3651, 3, ctypes.create_string_buffer(b"Hello"))
        self.assertEqual(json.loads(path.read_text(encoding="utf-8"))["text"], target.decode())

    def test_trace_off_or_unwritable(self):
        path = Path(self.tmp.name) / "trace.jsonl"
        self.assertEqual(self.lib.port_i18n_trace_open(os.fsencode(path)), 0)
        self.assertFalse(path.exists())
        self.assertEqual(self.init(), 0)
        self.assertEqual(self.lib.port_i18n_trace_open(os.fsencode(Path(self.tmp.name))), -1)
        self.assertEqual(self.lib.port_i18n_enabled(), 1)


class PackerTests(unittest.TestCase):
    def test_pack_repeat_and_order(self):
        path = UW1 / "i18n/zh-TW/poc.tsv"
        self.assertEqual(pack(path), pack(path))
        data, count, skipped = pack(path)
        self.assertEqual(data[:12], MAGIC + struct.pack("<I", 52))
        self.assertEqual((count, skipped), (52, 1))

    def test_reject_bad_keys_text_and_tokens(self):
        fields = ["kind", "block", "index", "source_key", "role", "source_en", "zh_tw", "note"]
        row = dict(zip(fields, ["pak", "3651", "3", "pak:3651:3", "test", "Hi @SS1\\m", "你好 @SS1\\m", ""]))
        with tempfile.TemporaryDirectory(dir=OUT) as tmp:
            path = Path(tmp) / "input.tsv"
            for change in ({"block": "0"}, {"block": "124"}, {"index": "512"},
                           {"source_key": "pak:3651:4"}, {"source_en": "a\0b"},
                           {"zh_tw": ""}, {"zh_tw": "你好 @SS2\\m"}, {"zh_tw": "你好 @SS1"}):
                with self.subTest(change=change):
                    write_tsv(path, [dict(row, **change)], fields)
                    with self.assertRaises(ValueError):
                        pack(path)
            write_tsv(path, [row, row], fields)
            with self.assertRaises(ValueError):
                pack(path)


if __name__ == "__main__":
    unittest.main()
