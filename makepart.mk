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

# CJSON_HIDE_SYMBOLS:
# libcjson は static のみを生成し、shared ライブラリ (例: libcom_util.so) へ
# 取り込む用途を想定する。既定の CJSON_EXPORT_SYMBOLS のままだと Windows で
# dllexport 付きオブジェクトが共有ライブラリへ再エクスポートされうるため、
# 埋め込み向けにシンボルを非公開にする。
# Linux では default visibility のままでは shared へ取り込まれた後も
# 動的シンボルとして露出するため、-fvisibility=hidden を併用する。
DEFINES += CJSON_HIDE_SYMBOLS
ifdef PLATFORM_LINUX
    CFLAGS   += -fvisibility=hidden
    CXXFLAGS += -fvisibility=hidden
endif
