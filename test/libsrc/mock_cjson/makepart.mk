ifdef PLATFORM_WINDOWS
    # cJSON の DLL import 宣言を無効にし、モックの実シンボルを定義する。
    DEFINES += CJSON_HIDE_SYMBOLS

    # 多数の MOCK_METHOD / ON_CALL を含むオブジェクトを COFF の通常上限内へ収める必要がないようにする。
    CXXFLAGS += /bigobj
endif
