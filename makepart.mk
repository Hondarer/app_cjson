# app/cjson/makepart.mk
# cJSON パッケージの展開を、app/cjson 配下のどのディレクトリで make を
# 起動しても最初に保証するためのフック。
#
# 注意: TEST_SRCS / SRCS_C の $(wildcard) 判定は Makefile の読み込み時に
# 即時評価されるため、対象ディレクトリの pre-build (ターゲット実行フェーズ)
# では展開が間に合わない。そのため、より上位の (全階層に継承される)
# makepart.mk で $(shell) を用いて「読み込み時」に展開を完了させる。
# see: framework/makefw/docs/makeparts.md

# sync_c_cpp_properties.sh による両 OS 同期評価時は実ビルドとは目的が
# 異なるため、実ビルド時 (MAKEFW_SYNC_EVAL 未定義時) のみ実行する。
#
# extract_package.py の案内/警告メッセージは標準エラー出力に書くため、
# $(shell) 実行中にそのまま端末へ表示される (make が変数へキャプチャする
# のは標準出力のみであり、$(shell) が改行をスペースへ潰すのも標準出力側の
# 挙動のため、複数行メッセージを崩さずに表示できる)。ここでは終了コードの
# みを標準出力経由で受け取る。
ifndef MAKEFW_SYNC_EVAL
    _CJSON_EXTRACT_STATUS := $(shell python3 "$(MYAPP_DIR)/bin/extract_package.py" --app-dir "$(MYAPP_DIR)" >&2; echo $$?)
    ifneq ($(_CJSON_EXTRACT_STATUS),0)
        $(error cJSON パッケージの準備に失敗しました。上記のメッセージに従って app/cjson/packages にアーカイブを配置してください)
    endif
endif

# 警告抑制 (app/makepart.mk の GCC_WARN_BASE) について:
#
# cJSON 本体 (cJSON.c / cJSON_Utils.c および展開ヘッダー) は upstream 側の設計
# であり、本リポジトリ側ではソースを改変しない方針のため実装スタイルを変更
# できない。本リポジトリは警告抑制フラグ (-Wno-*) を原則使わない方針だが
# (see: framework/makefw/docs/gcc-warning-guide/README.md)、lua/sqlite と同様に
# ソース非改変を優先し、cJSON 本体に起因する以下の警告に限り app/cjson 配下の
# コンパイル オプションで例外的に抑制する。
#
# - -Wconversion / -Wsign-conversion: 暗黙の型変換と符号変換 (上流実装全体に及び、
#   改変せずに解消できない)
#
# -Wpadded は extract_package.py が cJSON.h へ diagnostic pragma を付与するため、
# ここでは抑制しない。
#
# app/makepart.mk (親階層) で CFLAGS/CXXFLAGS に GCC_WARN_BASE が設定された後、
# makepart.mk は親から子の順に評価されるため、ここで追記する -Wno-* は
# 常に元の -W* より後方に置かれ、GCC 上で有効に上書きできる。
ifdef PLATFORM_LINUX
    CFLAGS   += -Wno-conversion -Wno-sign-conversion
    CXXFLAGS += -Wno-conversion -Wno-sign-conversion
endif
