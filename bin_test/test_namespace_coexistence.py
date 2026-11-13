#!/usr/bin/env python3
"""Verify app API isolation from an original-name shared library."""
import sys
import unittest
from pathlib import Path

BIN_TEST_PLATFORM = "linux"
APP_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(APP_DIR.parents[1] / "framework" / "testfw" / "bin_internal"))
from shared_namespace_probe import verify_namespace

@unittest.skipUnless(sys.platform.startswith("linux"), "[Linux] ELF namespace probe requires Linux")
class NamespaceCoexistenceTest(unittest.TestCase):
    def test_both_global_load_orders(self):
        verify_namespace(APP_DIR, 'cjson_app', 'cJSON_GetArraySize', '#include <cJSON.h>\nint namespace_probe(void) { cJSON *value=cjson_app_cJSON_Parse("[1,2]"); if(value==0) return 10; int n=cjson_app_cJSON_GetArraySize(value); cjson_app_cJSON_Delete(value); return n!=2; }\n')

if __name__ == "__main__":
    unittest.main()
