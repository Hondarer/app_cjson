ifdef PLATFORM_WINDOWS
    # DLL import ではなく mock_cjson が提供する実シンボルを参照する。
    DEFINES += CJSON_HIDE_SYMBOLS
endif

LIBS += mock_cjson
DEFINES += MOCK_CJSON_ENABLE_CALLER
TEST_SRCS += $(MYAPP_DIR)/test/src/libcjsonTest/mockCjsonTest/caller_info.c \
    $(MYAPP_DIR)/test/src/libcjsonTest/mockCjsonTest/caller_info_cpp.cc
