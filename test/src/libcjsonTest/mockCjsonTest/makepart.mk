ifdef PLATFORM_WINDOWS
    # DLL import ではなく mock_cjson が提供する実シンボルを参照する。
    DEFINES += CJSON_HIDE_SYMBOLS
endif

LIBS += mock_cjson
