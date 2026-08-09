# cJSON コアおよび cJSON_Utils (cJSON.c/h, cJSON_Utils.c/h) を取り込む。
#
# 本ディレクトリの .c ファイルおよび ../../include/cjson/ 配下のヘッダーは
# app/cjson/packages 配下の zip から機械的に展開される生成物であり、
# 手動改変しない (再展開で上書きされる)。そのため、本リポジトリの
# コーディング規範 (goto・三項演算子の不使用、新規ファイルへの
# clang-format 適用) は cJSON 本体には適用しない。
# (このディレクトリの makefile/makepart.mk 自体は手書きファイルであり、
#  通常どおり規範・clang-format の対象とする)

ifdef PLATFORM_WINDOWS
    # DLL 本体のビルド時だけ公開 API をエクスポートする。
    DEFINES += CJSON_EXPORT_SYMBOLS
else ifdef PLATFORM_LINUX
    # 公開 API 以外を hidden とし、CJSON_PUBLIC のシンボルだけを公開する。
    DEFINES += CJSON_API_VISIBILITY
    CFLAGS   += -fvisibility=hidden
    CXXFLAGS += -fvisibility=hidden
endif

# 静的ライブラリは生成せず、Linux では .so、Windows では .dll を生成する。
LIB_TYPE = shared
