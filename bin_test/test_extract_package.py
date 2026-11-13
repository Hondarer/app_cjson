#!/usr/bin/env python3
"""公式パッケージ展開と公開 API namespace 生成を検証する。"""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

APP = Path(__file__).resolve().parents[1]
MAKEFW = APP.parents[1] / "framework" / "makefw"


class ExtractPackageTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="cjson-extract-test-")
        self.addCleanup(self.temp.cleanup)
        self.app = Path(self.temp.name)
        shutil.copytree(APP / "packages", self.app / "packages")
        shutil.copytree(APP / "patches", self.app / "patches")
        (self.app / "prod").mkdir()
        shutil.copyfile(APP / "prod/public_symbols.txt", self.app / "prod/public_symbols.txt")
        shutil.copyfile(APP / "prod/public_api_macros.txt", self.app / "prod/public_api_macros.txt")
        (self.app / "prod/include").mkdir()
        shutil.copyfile(APP / "prod/include/cJSON_Integer.h", self.app / "prod/include/cJSON_Integer.h")
        shutil.copyfile(APP / "prod/include/cJSON_JSONC.h", self.app / "prod/include/cJSON_JSONC.h")

    def run_extract(self):
        return subprocess.run(
            [sys.executable, str(APP / "bin_internal/extract_package.py"),
             "--app-dir", str(self.app), "--makefw-home", str(MAKEFW)],
            capture_output=True, text=True, encoding="utf-8", check=False)

    def test_first_party_headers_are_not_rewritten(self):
        headers = [self.app / "prod/include" / name for name in ("cJSON_Integer.h", "cJSON_JSONC.h")]
        originals = {path.name: (APP / "prod/include" / path.name).read_bytes() for path in headers}
        result = self.run_extract()
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertEqual(originals, {path.name: path.read_bytes() for path in headers})

    def test_namespace_generation_and_noop(self):
        result = self.run_extract()
        self.assertEqual(0, result.returncode, result.stderr)
        header = self.app / "prod/include/cJSON.h"
        self.assertIn("cjson_app_cJSON_Parse", header.read_text(encoding="utf-8"))
        namespace = self.app / "prod/include_internal/cjson_app_namespace.h"
        self.assertTrue(namespace.is_file())
        before = {p: p.stat().st_mtime_ns for p in self.app.rglob("*") if p.is_file()}
        result = self.run_extract()
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertEqual(before, {p: p.stat().st_mtime_ns for p in before})


    def test_symbol_list_change_triggers_regeneration(self):
        result = self.run_extract()
        self.assertEqual(0, result.returncode, result.stderr)
        symbols = self.app / "prod/public_symbols.txt"
        with symbols.open("a", encoding="utf-8", newline="\n") as stream:
            stream.write("_NamespaceProbe\n")
        result = self.run_extract()
        self.assertEqual(0, result.returncode, result.stderr)
        namespace = self.app / "prod/include_internal/cjson_app_namespace.h"
        self.assertIn("#define _NamespaceProbe cjson_app__NamespaceProbe", namespace.read_text(encoding="utf-8"))

    def test_missing_namespace_header_is_regenerated(self):
        result = self.run_extract()
        self.assertEqual(0, result.returncode, result.stderr)
        namespace = self.app / "prod/include_internal/cjson_app_namespace.h"
        namespace.unlink()
        result = self.run_extract()
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertTrue(namespace.is_file())


if __name__ == "__main__":
    unittest.main()
