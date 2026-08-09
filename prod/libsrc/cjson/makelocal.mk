ifdef PLATFORM_LINUX
# shared-only への移行前に生成された静的ライブラリを残さない。
.PHONY: pre-build
pre-build:
	$(RM) "$(OUTPUT_DIR)/libcjson.a" "$(OUTPUT_DIR)/libcjson.a.warn"

.PHONY: pre-clean
pre-clean:
	$(RM) "$(OUTPUT_DIR)/libcjson.a" "$(OUTPUT_DIR)/libcjson.a.warn"
endif
