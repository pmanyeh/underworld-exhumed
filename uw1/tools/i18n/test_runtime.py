"""Compile/test the real native catalog module without SDL or original assets.

Run in the native compiler environment: python3 tools/i18n/test_runtime.py -v
"""
import ctypes
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest

from l10n import MAGIC, MASTER, MASTER_FIELDS, DatasetError, load_master, render_tsv
from pack_translations import pack

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
                                  str(UW1 / "src/port/i18n/utf8.c"),
                                  "-o", str(library)], check=True)
        cls.lib = ctypes.CDLL(str(library))
        cls.lib.port_i18n_init.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        cls.lib.port_i18n_init.restype = ctypes.c_int
        cls.lib.port_i18n_lookup.argtypes = [ctypes.c_uint, ctypes.c_uint, ctypes.c_void_p]
        cls.lib.port_i18n_lookup.restype = ctypes.c_void_p
        cls.lib.port_i18n_observe.argtypes = cls.lib.port_i18n_lookup.argtypes
        cls.lib.port_i18n_observe.restype = ctypes.c_void_p
        cls.lib.port_i18n_trace_open.argtypes = [ctypes.c_char_p]
        cls.lib.port_i18n_qa_open.argtypes = [ctypes.c_char_p]
        cls.lib.port_i18n_note_dynamic.argtypes = [ctypes.c_uint, ctypes.c_uint]
        cls.lib.port_i18n_note_dynamic.restype = None
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

    def test_load_actual_master_all_keys(self):
        data, n = pack(MASTER)
        rows = [r for r in load_master() if r["target_zh_TW"]]
        self.assertEqual(n, len(rows))
        self.assertGreaterEqual(n, 52)
        self.assertEqual(self.init(data), 0)
        self.assertEqual(self.lib.port_i18n_enabled(), 1)
        for row in rows:
            buffer, pointer = self.lookup(row["source"].encode("cp437"), int(row["block"]), int(row["index"]))
            self.assertNotEqual(pointer, ctypes.addressof(buffer))
            self.assertEqual(ctypes.string_at(pointer), row["target_zh_TW"].encode())

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
        for n in (0, 32769, 0xFFFFFFFF):
            self.assert_bad(MAGIC + struct.pack("<I", n))
        self.assert_bad(self.data + b"x")
        self.assert_bad(b"x" * (16 * 1024 * 1024 + 1))

    def test_full_game_sized_catalog(self):
        # Phase 9: every addressable key of a full translation fits.
        rows = [(1 + n // 512, n % 512, b"S%d" % n, "譯%d".encode() % n) for n in range(32768)]
        self.assertEqual(self.init(catalog(rows)), 0)
        buffer, pointer = self.lookup(b"S32767", 64, 511)
        self.assertEqual(ctypes.string_at(pointer), "譯32767".encode())

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

    def qa_events(self, path):
        return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()]

    def test_qa_trace_classifies_every_lookup(self):
        rows = [(1, 2, b"Plain", "純文字".encode()), (3651, 3, b"Hello", "你好，@SS1！".encode())]
        self.assertEqual(self.init(catalog(rows)), 0)
        path = Path(self.tmp.name) / "qa.jsonl"
        self.assertEqual(self.lib.port_i18n_qa_open(os.fsencode(path)), 0)
        for source, block, index in [(b"Plain", 1, 2), (b"Plain", 1, 2), (b"Hello", 3651, 3),
                                     (b"Missing", 1, 3), (b"Changed", 1, 2), (b"Hello", 0, 3),
                                     (b"Hello", 0x7C, 1), (b"Hello", 3651, 512)]:
            buffer = ctypes.create_string_buffer(source)
            self.assertEqual(self.lib.port_i18n_observe(block, index, buffer), ctypes.addressof(buffer))
        self.lib.port_i18n_note_dynamic(0x7D, 0)
        self.assertEqual([(r["event"], r["block"], r["index"]) for r in self.qa_events(path)],
                         [("hit", 1, 2), ("hit", 1, 2), ("template", 3651, 3), ("miss", 1, 3),
                          ("drift", 1, 2), ("invalid", 0, 3), ("invalid", 0x7C, 1),
                          ("invalid", 3651, 512), ("dynamic", 0x7D, 0)])

    def test_qa_trace_in_english_records_misses(self):
        self.assertEqual(self.init(locale=b"en"), 0)
        path = Path(self.tmp.name) / "qa.jsonl"
        self.assertEqual(self.lib.port_i18n_qa_open(os.fsencode(path)), 0)
        buffer = ctypes.create_string_buffer(b"Hello")
        self.assertEqual(self.lib.port_i18n_observe(3651, 3, buffer), ctypes.addressof(buffer))
        self.assertEqual(self.qa_events(path), [{"event": "miss", "block": 3651, "index": 3}])
        self.assertEqual(self.lib.port_i18n_qa_open(None), 0)
        self.assertEqual(self.lib.port_i18n_qa_open(os.fsencode(Path(self.tmp.name))), -1)

    def test_trace_off_or_unwritable(self):
        path = Path(self.tmp.name) / "trace.jsonl"
        self.assertEqual(self.lib.port_i18n_trace_open(os.fsencode(path)), 0)
        self.assertFalse(path.exists())
        self.assertEqual(self.init(), 0)
        self.assertEqual(self.lib.port_i18n_trace_open(os.fsencode(Path(self.tmp.name))), -1)
        self.assertEqual(self.lib.port_i18n_enabled(), 1)


class PackerTests(unittest.TestCase):
    def test_pack_repeat_and_order(self):
        self.assertEqual(pack(MASTER), pack(MASTER))
        data, count = pack(MASTER)
        self.assertEqual(data[:12], MAGIC + struct.pack("<I", count))

    def test_reject_bad_keys_text_and_tokens(self):
        row = dict(zip(MASTER_FIELDS, ["3651", "3", "conversation", "Hi @SS1\\m", "你好 @SS1\\m",
                                       "draft", "", "", "dynamic-template,control-code"]))
        with tempfile.TemporaryDirectory(dir=OUT) as tmp:
            path = Path(tmp) / "input.tsv"
            path.write_bytes(render_tsv([row], MASTER_FIELDS))
            self.assertEqual(pack(path)[1], 1)
            for change in ({"block": "0"}, {"block": "124"}, {"index": "512"}, {"index": "03"},
                           {"category": "game"}, {"source": "a\0b"}, {"status": "done"},
                           {"target_zh_TW": ""}, {"target_zh_TW": "你好 @SS2\\m"},
                           {"target_zh_TW": "你好 @SS1"}, {"flags": "control-code"}):
                with self.subTest(change=change):
                    path.write_bytes(render_tsv([dict(row, **change)], MASTER_FIELDS))
                    with self.assertRaises(DatasetError):
                        pack(path)
            path.write_bytes(render_tsv([row, row], MASTER_FIELDS))
            with self.assertRaises(DatasetError):
                pack(path)


if __name__ == "__main__":
    unittest.main()
