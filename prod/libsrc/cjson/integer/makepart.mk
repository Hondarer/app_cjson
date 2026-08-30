# cJSON の整数拡張 (cJSON_Integer.c) は手書きコードであり、cJSON 本体 (zip からの
# 展開生成物) とは異なり、本リポジトリのコーディング規範と clang-format の対象とする。
#
# 親 (prod/libsrc/cjson/makepart.mk) は OSS 本体向けに -Wno-conversion /
# -Wno-sign-conversion を付与しており、makepart.mk は親から子へ継承される。
# このディレクトリでは規範どおり変換警告を検査させるため、親の抑制をここで
# 打ち消す (GCC はコマンドライン上で後に指定したオプションを優先する)。
# 可視性の設定 (CJSON_EXPORT_SYMBOLS / CJSON_API_VISIBILITY / -fvisibility=hidden) は
# 親からの継承をそのまま活かし、ここでは変更しない。
ifdef PLATFORM_LINUX
    CFLAGS   += -Wconversion -Wsign-conversion
    CXXFLAGS += -Wconversion -Wsign-conversion
endif
