ifdef PLATFORM_WINDOWS
    # cJSON の DLL import 宣言を無効にし、モックの実シンボルを定義する。
    DEFINES += CJSON_HIDE_SYMBOLS
endif
