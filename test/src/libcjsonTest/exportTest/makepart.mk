# ライブラリ全体の公開シンボルを検査する黒箱テストのため、TEST_SRCS は指定しない。
LIBS += cjson

# テスト エビデンスと C++ の条件分岐へ同じプラットフォーム名を渡す。
ifdef PLATFORM_LINUX
    DEFINES += PLATFORM_LINUX
else ifdef PLATFORM_WINDOWS
    DEFINES += PLATFORM_WINDOWS
endif
