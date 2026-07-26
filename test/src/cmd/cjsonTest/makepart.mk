# ビルド済みライブラリの検索パス
LIBSDIR += \
	$(MYAPP_DIR)/prod/lib

# ライブラリの指定
#
# 本テストは cJSON.c/cJSON_Utils.c 自体のカバレッジ計測ではなく、
# app/cjson が提供する API (libcjson.a) の独自動作確認を目的とする。
# そのため TEST_SRCS は使用せず、ビルド済みライブラリへの通常のリンクで検証する。
LIBS += cjson
