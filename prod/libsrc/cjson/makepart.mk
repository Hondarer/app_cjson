# cJSON コアおよび cJSON_Utils (cJSON.c/h, cJSON_Utils.c/h) を取り込む。
#
# 本ディレクトリの .c ファイルおよび ../../include/cjson/ 配下のヘッダーは
# app/cjson/packages 配下の zip から機械的に展開される生成物であり、
# 手動改変しない (再展開で上書きされる)。そのため、本リポジトリの
# コーディング規範 (goto・三項演算子の不使用、新規ファイルへの
# clang-format 適用) は cJSON 本体には適用しない。
# (このディレクトリの makefile/makepart.mk 自体は手書きファイルであり、
#  通常どおり規範・clang-format の対象とする)

# LIB_TYPE は指定しない (デフォルト static -> libcjson.a)。
# shared 版は作らない。
