# libcjson_app を暗黙にはリンクしない。LoadLibrary / dlopen のあとで TLS を確かめる。
ifdef PLATFORM_LINUX
    LIBS += dl
endif
