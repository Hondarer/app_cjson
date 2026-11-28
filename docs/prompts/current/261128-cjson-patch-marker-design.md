<!--
生成元: agree-before-implement スキル
文書種別: 最終仕様 (design)
最終更新日時: 2026-11-28 13:07:33 JST
最終更新エージェント: agy / セッション ID: af98b530-c6a7-4b3a-973f-a6b64a55d79c
稼働 OS: Linux (WSL)
上流文書: `261128-cjson-patch-marker-overview.md` (全体管理文書)、`261128-cjson-patch-marker-discussion.md` (検討用 Markdown)、`261128-cjson-patch-marker-lightadr.md` (簡易 ADR)。いずれも削除されている場合があります。
再開手順: このファイルを開いたエージェントは、作業の前に agree-before-implement スキルを読み込んでください。上流文書の全体管理文書が存在する場合は、その再開手順に従って進行位置を確認し、決定事項を簡易 ADR で、判断の経緯を検討用 Markdown の Q&A で確認します。存在しない場合は、この文書を単体の最終仕様として扱います。最終更新エージェントのセッション情報や稼働 OS・最終更新日時が参照可能であれば再開時のコンテキストや環境差分の把握に利用し、別エージェントや別環境・別 OS からの引き継ぎ、担当変更によりセッションが見つからない場合も異常とせず、文書の記載から継続してください。
-->

# cJSON パッチ 0003 の適用をビルド時に検査する仕組み 最終仕様

ファイル名: `261128-cjson-patch-marker-design.md`

## 目的と適用範囲

`app/cjson` が同梱の cJSON 本体へ適用するパッチ `0003-thread-local-global-error.patch` (`global_error` のスレッド ローカル化) が適用済みであることを、ビルド時に機械的に検査する仕組みを導入します。  
パッチ未適用の状態や、インクルード経路の誤りによる未パッチの `cJSON.h` の取り込みをコンパイル エラーとして検出します。

対象範囲は次のとおりです。

- `app/cjson/patches/0003-thread-local-global-error.patch` へのマーカー定義の追加
- 検査専用の公開ヘッダー `app/cjson/prod/include/cJSON_PatchCheck.h` の新規作成
- `app/cjson/prod/include/cJSON_JSONC.h` および `<cJSON.h>` などを直接取り込む利用側での `cJSON_PatchCheck.h` の取り込み
- `app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c` の既存 `_Static_assert` 3 か所の `static_assert` 化
- `app/cjson/bin_test/` での自動検証テストの追加・更新
- 関連する正本ドキュメントおよび Doxygen コメントへの反映

> [!NOTE]
> 動的リンク時のライブラリ取り違えに対する実行時照合は対象外とします。  
> `app/cjson` はすべての公開シンボルへ `cjson_app_` 接頭辞を付与して OS 標準ライブラリと分離しているため、公開 API、`mock_cjson` の API 表、`exportTest.cc` は変更しません。  
> また、`app/cjson/packages/` から展開される生成物 (`prod/include/cJSON.h`、`prod/libsrc/cjson/cJSON.c` 等) への直接編集は行いません。

## パッチ 0003 への差分仕様

`app/cjson/patches/0003-thread-local-global-error.patch` の先頭に、`prod/include/cJSON.h` に対する差分を追加します。  
マーカー マクロ `CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR` (値 `1`) を、インクルード ガード (`#ifndef cJSON__h`) の内側かつ `#define CJSON_VERSION_PATCH 19` の直後に定義します。

> [!IMPORTANT]
> `apply_patches.py` はファイル名の昇順にパッチを厳密適用するため、`prod/include/cJSON.h` のコンテキスト行はパッチ `0001-windows-dll-import-default.patch` 適用後の行番号と一致させます。  
> マーカー名 `CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR` は `prod/public_symbols.txt` および `prod/public_api_macros.txt` に含まれないため、`generate_symbol_namespace.py` による名前空間置換を受けません。

追加する差分 (`0003-thread-local-global-error.patch` 先頭) は次のとおりです。既存の `prod/libsrc/cjson/cJSON.c` に対する差分は変更せず、その後ろにそのまま維持します。

```diff
--- a/prod/include/cJSON.h
+++ b/prod/include/cJSON.h
@@ -88,5 +88,8 @@
 #define CJSON_VERSION_MAJOR 1
 #define CJSON_VERSION_MINOR 7
 #define CJSON_VERSION_PATCH 19
+
+/* PATCH: 0003-thread-local-global-error.patch の適用マーカー */
+#define CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR 1

 #include <stddef.h>
```

> [!NOTE]
> 実際の `.patch` ファイルを作成・更新する際は、`app/cjson/patches/README.md` の手順どおり `diff -u` で差分を生成し、空のコンテキスト行の先頭半角空白 1 文字を保持します。

## 検査用公開ヘッダーの仕様

ビルド時検査は 1 か所の第一者管理ヘッダーに集約し、各利用側は同じ検査を複写せずこのヘッダーを取り込みます。

- 配置パス: `app/cjson/prod/include/cJSON_PatchCheck.h`
- インクルード ガード: `CJSON_PATCH_CHECK_H`
- 対応規格: C17 (`-std=c17 -Wpedantic` / `/std:c17 /Zc:preprocessor`) および C++17 (`-std=c++17 -Wpedantic` / `/std:c++17 /Zc:__cplusplus /Zc:preprocessor /permissive-`)

`app/general/docs/coding-guideline.md` の「R2 制約を静的に検査する」に従い、`<assert.h>` を取り込んで C と C++ の双方で `static_assert` を使用します (`__cplusplus` による表記分岐は設けません)。  
マーカー未定義時に未宣言識別子エラーを併発させないため、`#ifndef` で `#error` を出力し、`#else` 分岐内で `static_assert` により値 `1` を検証します。

`app/cjson/prod/include/cJSON_PatchCheck.h` の全文仕様は次のとおりです。

```c
/**
 *******************************************************************************
 *  @file           cJSON_PatchCheck.h
 *  @brief          cJSON パッチ 0003 の適用をビルド時に検査します。
 *  @author         Tetsuo Honda
 *  @date           2026/11/28
 *  @version        1.0.0
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef CJSON_PATCH_CHECK_H
#define CJSON_PATCH_CHECK_H

#include <assert.h>
#include <cJSON.h>

#ifndef CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR
    #error "cJSON patch 0003 (thread-local global_error) is not applied"
#else
static_assert(CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR == 1,
              "cJSON patch 0003 (thread-local global_error) version mismatch");
#endif

#endif /* CJSON_PATCH_CHECK_H */
```

> [!NOTE]
> `cJSON_PatchCheck.h` は関数宣言を含まないため、`mock_cjson` の対象外であり `app/cjson/test/include_override/` への差し替えヘッダーは追加しません。

## 検査用ヘッダーを取り込む全箇所の一覧

ワークスペース全体で cJSON の公開ヘッダー (`cJSON.h`、`cJSON_Integer.h`、`cJSON_JSONC.h`、`cJSON_Utils.h`) を直接取り込む箇所を分類し、取り込み方針を定めます。

### 直接 #include <cJSON_PatchCheck.h> を追加する箇所

次の 12 ファイルに `#include <cJSON_PatchCheck.h>` を追加します。  
`#include <cJSON.h>` (または `<cJSON_Integer.h>`) の直後に配置します。`app/struct-meta/prod/include/struct_meta/json/json.h` の取り込み行にはコメントを付けません。

Table: 直接 `cJSON_PatchCheck.h` を取り込むファイル一覧

| サブモジュール | ファイル パス | 既存の cJSON インクルード | 配置方法 |
| --- | --- | --- | --- |
| `app/cjson` | `prod/include/cJSON_JSONC.h` | `<cJSON.h>` | `#include <cJSON.h>` の直後に追加 |
| `app/cjson` | `prod/src/cmd/cjson_sample/cjson_sample.c` | `<cJSON.h>` | `#include <cJSON.h>` の直後に追加 |
| `app/cjson` | `test/src/cmd/cjsonTest/cjsonTest.cc` | `<cJSON.h>` | `#include <cJSON.h>` の直後に追加 |
| `app/cjson` | `test/src/cmd/cjsonIntegerTest/cjsonIntegerTest.cc` | `<cJSON.h>`、`<cJSON_Integer.h>` | `#include <cJSON.h>` の直後に追加 |
| `app/cjson` | `test/src/libcjsonTest/mockCjsonTest/caller_info.c` | `<cJSON.h>` | `#include <cJSON.h>` の直後に追加 |
| `app/cjson` | `test/src/libcjsonTest/mockCjsonTest/caller_info_cpp.cc` | `<cJSON.h>` | `#include <cJSON.h>` の直後に追加 |
| `app/porter` | `prod/libsrc/porter/api/potr_config_list_service_ids.c` | `<cJSON.h>` | `#include <cJSON.h>` の直後に追加 |
| `app/porter` | `prod/libsrc/porter/api/potr_config_validate_file.c` | `<cJSON.h>` | `#include <cJSON.h>` の直後に追加 |
| `app/porter` | `prod/libsrc/porter/api/potr_service_get_type.c` | `<cJSON.h>` | `#include <cJSON.h>` の直後に追加 |
| `app/porter` | `prod/libsrc/porter/api/potr_service_open_from_config.c` | `<cJSON.h>` | `#include <cJSON.h>` の直後に追加 |
| `app/porter` | `prod/libsrc/porter/protocol/config_decode.c` | `<cJSON.h>`、`<cJSON_Integer.h>` | `#include <cJSON.h>` の直後に追加 |
| `app/struct-meta` | `prod/include/struct_meta/json/json.h` | `<cJSON.h>` | `#include <cJSON.h>` の直後に追加 (コメントなし) |

### 経由ヘッダーを通じて間接的に検査を受け取る箇所 (変更不要)

次のファイルは、`cJSON_JSONC.h` または `struct_meta/json/json.h` を取り込むため、追加の編集なしでビルド時検査を受け取ります。

Table: 間接的に検査を受け取るファイル一覧

| 経由ヘッダー | 対象ファイル |
| --- | --- |
| `<cJSON_JSONC.h>` | `app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c`、`app/cjson/test/include/mock_cjson.h`、`app/cjson/test/src/cmd/cjsonTest/cjsonJsoncTest.cc`、`app/cjson/test/src/cmd/cjsonTest/cjsonJsoncDynamicLoadTest.cc`、`app/cjson/test/src/libcjsonTest/exportTest/exportTest.cc`、`app/cplat/prod/libsrc/cplat/runtime/sym_loader_init.c`、`app/porter/prod/libsrc/porter/protocol/config_document.c`、`app/porter/test/src/libporter/configDecodeTest/configDecodeTest.cc` |
| `<struct_meta/json/json.h>` | `app/struct-meta/prod/libsrc/struct_meta/json/file.c`、`app/struct-meta/prod/libsrc/struct_meta/json/encode.c` (`<cJSON_Integer.h>` も取り込み)、`app/struct-meta/prod/libsrc/struct_meta/json/decode.c` (`<cJSON_Integer.h>` も取り込み)、`app/struct-meta/prod/src/cmd/struct_meta_sample/` の関連ソース、`app/struct-meta/test/src/jsonEncodeTest/jsonEncodeTest.cc` (`<cJSON_Integer.h>` も取り込み)、`app/struct-meta/test/src/jsonDecodeTest/jsonDecodeTest.cc` |

### 取り込み対象外とする箇所

次のファイルは提供側実装、自動生成物、またはモック差し替えヘッダーであるため、`cJSON_PatchCheck.h` を追加しません。

- 展開生成物: `app/cjson/prod/include/cJSON.h`、`prod/include/cJSON_Utils.h`、`prod/libsrc/cjson/cJSON.c`、`prod/libsrc/cjson/cJSON_Utils.c`
- 整数拡張の提供側: `app/cjson/prod/include/cJSON_Integer.h`、`prod/include_internal/cJSON_Integer_internal.h`、`prod/libsrc/cjson/integer/cJSON_Integer.c`
- モック差し替えヘッダー: `app/cjson/test/include_override/` の 4 ファイル (`cJSON.h`、`cJSON_Integer.h`、`cJSON_JSONC.h`、`cJSON_Utils.h`。実ヘッダーを取り込む)

## cJSON_JSONC.c の静的アサート表記の統一

コーディング規範の「R2 制約を静的に検査する」に従い、`app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c` の既存の `_Static_assert` 3 か所を `static_assert` へ統一します。

- `#include <assert.h>` を標準ヘッダーのインクルード群の先頭へ追加します。
- 確保ヘッダー `jsonc_block_header` の整列・大きさを検証している `_Static_assert` 3 か所を `static_assert` へ置き換えます。

## 自動テストの仕様 (app/cjson/bin_test/)

### app/cjson/bin_test/test_extract_package.py の更新 (Linux / Windows 両対応)

既存の展開・名前空間テストへ次の 2 点の検証を追加します。

1. `setUp()` で `prod/include/cJSON_PatchCheck.h` を一時ディレクトリへ複製し、`test_first_party_headers_are_not_rewritten` の対象に `cJSON_PatchCheck.h` を加えて、`extract_package.py` が第一者管理ヘッダーを書き換えないことを検証します。
2. 展開・パッチ適用・名前空間変換後の `prod/include/cJSON.h` に `#define CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR 1` が含まれることを検証します (既存テストへのアサーション追加または独立テスト メソッド `test_patch_0003_marker_is_defined` の追加)。

### app/cjson/bin_test/test_patch_marker_check.py の新規作成 (Linux 向け)

`app/general/bin_internal/run-bin-tests.py` の規約に従い、`BIN_TEST_PLATFORM = "linux"` を定義し、Linux 以外では `@unittest.skipUnless(sys.platform.startswith("linux"), "[Linux] GCC compile-failure test requires Linux")` でスキップします。Linux 環境で `gcc` または `g++` が見つからない場合はスキップせず失敗させます。

一時ディレクトリにテスト用の最小ソース (`.c` と `.cc`) およびダミーの `cJSON.h` を生成し、`-std=c17` (`gcc`) と `-std=c++17` (`g++`) の双方で `-Wall -Wextra -Wpedantic -Werror -fsyntax-only` を指定して次の 3 ケースを検証します。

Table: `test_patch_marker_check.py` の検証ケース

| ケース | 渡す `cJSON.h` の内容 | 期待結果 |
| --- | --- | --- |
| 正常系 (C17 / C++17) | `#define CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR 1` を含むヘッダー (`prod/include/cJSON_PatchCheck.h` を直接取り込み) | 終了コード 0、警告・エラーなし |
| マーカー未定義 (C17 / C++17) | マーカー定義を含まない `cJSON.h` を `-I` で優先配置 | 非 0 終了、標準エラーに `cJSON patch 0003 (thread-local global_error) is not applied` を含む |
| マーカー値不一致 (C17 / C++17) | `#define CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR 2` を含む `cJSON.h` を `-I` で優先配置 | 非 0 終了、標準エラーに `cJSON patch 0003 (thread-local global_error) version mismatch` を含む |

## 正本ドキュメントと Doxygen コメントへの反映計画

永続的に参照される仕様として、次の 6 ファイルへビルド時検査の導入を反映します。

Table: 更新する文書とコメントの一覧

| ファイル | 更新箇所 | 反映する内容 |
| --- | --- | --- |
| `app/cjson/patches/README.md` | 収録パッチ一覧表と `0003` の説明段落 | 対象へ `prod/include/cJSON.h` を追加し、マーカー `CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR` の定義と `prod/include/cJSON_PatchCheck.h` によるビルド時検査を記載する。 |
| `app/cjson/README.md` | 「JSONC の解析」のパッチ `0003` の説明 | パッチ `0003` の適用は `cJSON_PatchCheck.h` によりビルド時に検査され、パッチ未適用の `cJSON.h` ではコンパイルが失敗する旨を追記する。 |
| `app/cjson/prod/include/cJSON_JSONC.h` | `cjson_app_cJSON_ParseJSONCWithLength()` と `cjson_app_cJSON_ParseJSONCWithLengthOpts()` の `@par スレッド セーフ` (2 か所) | パッチ `0003` の適用は `cJSON_PatchCheck.h` によりビルド時に検査される旨を追記する。 |
| `app/porter/docs/api.md` | 「スレッド セーフ」の `> [!IMPORTANT]` ブロック | ビルド時に `cJSON_PatchCheck.h` でパッチ `0003` の適用を検査し、パッチのない `cJSON.h` の取り込みをコンパイル エラーとして検出する旨を追記する。 |
| `app/porter/prod/include/porter/porter_spec.h` | `potr_service_get_type()`、`potr_config_validate_file()`、`potr_config_list_service_ids()` の `@par スレッド セーフ` (3 か所) | パッチ `0003` の適用はビルド時に検査される旨を追記する。 |
| `app/struct-meta/docs/architecture.md` | 「ビルド」の節 | JSON の変換は `app/cjson` が独自のパッチを当てた cJSON を前提とし、`struct_meta/json/json.h` が `cJSON_PatchCheck.h` を取り込んでパッチのない `cJSON.h` でのコンパイルを失敗させる旨の段落を追加する。 |

## 検証手順

対象成果物の変更完了後、次の順序で検証を実施します。`make` は同時に 1 本だけ実行します。

1. **局所ビルドと単体テスト (`make` の対象範囲)**:
    - `app/cjson`: `make -C app/cjson` および `make -C app/cjson test`
    - `app/cplat`: `make -C app/cplat` および `make -C app/cplat test`
    - `app/porter`: `make -C app/porter` および `make -C app/porter test`
    - `app/struct-meta`: `make -C app/struct-meta` および `make -C app/struct-meta test`
2. **警告確認**:
    - 上記 4 app 配下で生成された `.warn` ファイルのうち内容があるものを確認し、変更起因の警告がないことを確認します。
3. **`bin_test` の実行**:
    - `app/cjson/bin_test/test_extract_package.py`、`test_patch_marker_check.py`、`test_namespace_coexistence.py` を実行し、すべて成功することを確認します。
4. **Markdown の機械整形**:
    - 変更した全 Markdown ファイルに対して `framework/docsfw` 配下の `text_style_jp.py --dry-run` を実行し、変換内容に問題がないことを確認してから `--in-place` を実行します。

## 受入条件

次の条件をすべて満たすことをもって実装完了とします。

1. `app/cjson/patches/0003-thread-local-global-error.patch` が `extract_package.py` / `apply_patches.py` で厳密適用され、展開・名前空間変換後の `prod/include/cJSON.h` に `#define CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR 1` が含まれること。
2. `app/cjson/prod/include/cJSON_PatchCheck.h` が C17 と C++17 の双方で、マーカー `1` のとき警告なしでコンパイルでき、未定義のとき `#error`、値不一致のとき `static_assert` で失敗すること。
3. 直接取り込み対象の 12 ファイルすべてに `#include <cJSON_PatchCheck.h>` が配置され、`struct_meta/json/json.h` の取り込み行にコメントが付いていないこと。
4. `app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c` の既存 `_Static_assert` 3 か所が `static_assert` (`<assert.h>`) へ置き換わっていること。
5. `app/cjson/bin_test/` の全テスト (`test_extract_package.py`、`test_patch_marker_check.py`、`test_namespace_coexistence.py`) が成功すること。
6. `app/cjson`、`app/cplat`、`app/porter`、`app/struct-meta` のビルドと単体テストが成功し、変更起因の `.warn` がないこと。
7. 反映計画の 6 文書・コメントが更新され、変更した Markdown が `text_style_jp.py` の検証を通過していること。
