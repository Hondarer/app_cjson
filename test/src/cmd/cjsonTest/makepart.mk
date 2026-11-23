# JSONC ラッパーは TEST_SRCS へ取り込み、cJSON 本体と整数拡張は依存ソースとしてリンクする。
TEST_SRCS := $(MYAPP_DIR)/prod/libsrc/cjson/jsonc/cJSON_JSONC.c
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/cjson/cJSON.c \
    $(MYAPP_DIR)/prod/libsrc/cjson/cJSON_Utils.c \
    $(MYAPP_DIR)/prod/libsrc/cjson/integer/cJSON_Integer.c

INCDIR += $(MYAPP_DIR)/prod/include $(MYAPP_DIR)/prod/libsrc/cjson
DEFINES += CJSON_HIDE_SYMBOLS TESTFW_MOCK_STDLIB_FREE
LIBS += mock_libc

# cJSON 本体の API 名を libcjson_app と同じ名前空間へ割り当てる。
ifdef PLATFORM_WINDOWS
    CFLAGS   += /FI"$(MYAPP_DIR)/prod/include_internal/cjson_app_namespace.h"
    CXXFLAGS += /FI"$(MYAPP_DIR)/prod/include_internal/cjson_app_namespace.h"
else
    CFLAGS   += -include "$(MYAPP_DIR)/prod/include_internal/cjson_app_namespace.h"
    CXXFLAGS += -include "$(MYAPP_DIR)/prod/include_internal/cjson_app_namespace.h"
    CFLAGS   += -Wno-conversion -Wno-sign-conversion
endif
