# JSONC 拡張は手書きコードのため、外来 cJSON 本体向けに親で抑制した
# 整数変換警告を再び有効にする。
ifdef PLATFORM_LINUX
    CFLAGS   += -Wconversion -Wsign-conversion
    CXXFLAGS += -Wconversion -Wsign-conversion
endif
