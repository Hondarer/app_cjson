#!/usr/bin/env python3
"""cJSON_PatchCheck.h によるパッチ 0003 マーカーのビルド時検査を検証する。"""
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

BIN_TEST_PLATFORM = "linux"
APP_DIR = Path(__file__).resolve().parents[1]
PROD_INCLUDE_DIR = APP_DIR / "prod" / "include"

COMPILERS = (
    ("c17", "gcc", "-std=c17", "check.c"),
    ("cpp17", "g++", "-std=c++17", "check.cc"),
)

_GCC_OCTAL_SEQ_RE = re.compile(r"(?:\\(?:37777777[4-7]|[0-3])[0-7]{2})+")


def _decode_gcc_octal_escapes(text):
    """GCC 8 の C フロントエンドが static_assert 文字列の非 ASCII バイトを
    8 進エスケープして出力する仕様を吸収し、UTF-8 文字列へ戻す。"""
    def _replace(match):
        octals = re.findall(r"\\((?:37777777[4-7]|[0-3])[0-7]{2})", match.group(0))
        return bytes(int(o, 8) & 0xFF for o in octals).decode("utf-8", errors="replace")

    return _GCC_OCTAL_SEQ_RE.sub(_replace, text)


@unittest.skipUnless(
    sys.platform.startswith("linux"),
    "[Linux] GCC compile-failure test requires Linux",
)
class PatchMarkerCheckTest(unittest.TestCase):
    def run_syntax_check(self, dummy_cjson_h_content):
        results = {}
        with tempfile.TemporaryDirectory(prefix="cjson-patch-check-") as temp_dir:
            temp_path = Path(temp_dir)
            dummy_inc = temp_path / "dummy_include"
            dummy_inc.mkdir()
            (dummy_inc / "cJSON.h").write_text(
                dummy_cjson_h_content, encoding="utf-8"
            )
            for label, compiler, std_flag, filename in COMPILERS:
                compiler_path = shutil.which(compiler)
                if compiler_path is None:
                    self.fail(f"{compiler} not found in PATH")
                src_path = temp_path / filename
                src_path.write_text(
                    "#include <cJSON_PatchCheck.h>\n"
                    "int patch_marker_check_dummy(void);\n",
                    encoding="utf-8",
                )
                cmd = [
                    compiler_path,
                    std_flag,
                    "-Wall",
                    "-Wextra",
                    "-Wpedantic",
                    "-Werror",
                    "-fsyntax-only",
                    "-I",
                    str(dummy_inc),
                    "-I",
                    str(PROD_INCLUDE_DIR),
                    str(src_path),
                ]
                results[label] = subprocess.run(
                    cmd,
                    capture_output=True,
                    text=True,
                    encoding="utf-8",
                    check=False,
                )
        return results

    def test_valid_marker_compiles_cleanly(self):
        results = self.run_syntax_check(
            "#ifndef cJSON__h\n"
            "#define cJSON__h\n"
            "#define CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR 1\n"
            "#endif\n"
        )
        for label, result in results.items():
            with self.subTest(lang=label):
                self.assertEqual(0, result.returncode, result.stderr)
                self.assertEqual("", result.stderr)

    def test_missing_marker_fails_compile(self):
        results = self.run_syntax_check(
            "#ifndef cJSON__h\n"
            "#define cJSON__h\n"
            "#endif\n"
        )
        for label, result in results.items():
            with self.subTest(lang=label):
                self.assertNotEqual(0, result.returncode)
                self.assertIn(
                    "cJSON にパッチ 0003 (global_error のスレッド ローカル化) が当たっていません",
                    result.stderr,
                )

    def test_mismatched_marker_fails_compile(self):
        results = self.run_syntax_check(
            "#ifndef cJSON__h\n"
            "#define cJSON__h\n"
            "#define CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR 2\n"
            "#endif\n"
        )
        for label, result in results.items():
            with self.subTest(lang=label):
                self.assertNotEqual(0, result.returncode)
                decoded_stderr = _decode_gcc_octal_escapes(result.stderr)
                self.assertIn(
                    "cJSON のパッチ 0003 (global_error のスレッド ローカル化) の版が一致しません",
                    decoded_stderr,
                )


if __name__ == "__main__":
    unittest.main()
