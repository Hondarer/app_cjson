.PHONY: pre-build
pre-build:
	python3 "$(TESTFW_HOME)/bin_internal/generate_shared_lib_mock_overrides.py" "$(MYAPP_DIR)/test/include/mock_cjson_api_table.h" "$(MYAPP_DIR)/test/include/mock_cjson_overrides.h" --macro-prefix MOCK_CJSON
