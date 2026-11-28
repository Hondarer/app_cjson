<!--
生成元: agree-before-implement スキル
文書種別: 簡易 ADR (lightadr)
最終更新日時: 2026-11-28 18:08:00 JST
最終更新エージェント: agy / セッション ID: f1717ab7-aaa3-487b-8275-84c421514770
稼働 OS: Linux (WSL)
上流文書: `261128-cjson-patch-marker-overview.md` (全体管理文書)、`261128-cjson-patch-marker-discussion.md` (検討用 Markdown)。いずれも削除されている場合があります。
再開手順: このファイルを開いたエージェントは、作業の前に agree-before-implement スキルを読み込んでください。上流文書の全体管理文書が存在する場合は、その再開手順に従って進行位置を確認します。存在しない場合は、この文書を単体の決定事項の記録として扱います。この文書は正式な ADR ではありません。正式な ADR への転記はユーザーが必要時に行うため、エージェントは依頼なしに転記や正式な ADR の作成・更新を行わないでください。最終更新エージェントのセッション情報や稼働 OS・最終更新日時が参照可能であれば再開時のコンテキストや環境差分の把握に利用し、別エージェントや別環境・別 OS からの引き継ぎ、担当変更によりセッションが見つからない場合も異常とせず、文書の記載から継続してください。
-->

# cJSON パッチ 0003 の適用をビルド時に検査する仕組み 簡易 ADR

ファイル名: `261128-cjson-patch-marker-lightadr.md`

この文書は、検討で得られた事実、ユーザーの選択とコメント、およびそれらに基づく決定事項を、決定 1 件ごとにまとめた簡易 ADR (Architecture Decision Record: 設計判断の記録) です。正式な ADR ではなく、対案の比較や決定の影響の評価は記載していません。  
各決定の「決定」は、方針が変わるたびに書き換え、常に最新の決定だけを示します。変更の推移は「経緯」に時系列で記録します。

## 決定事項一覧

| 決定 ID | 題名 | 最終更新日 | 源泉 |
| --- | --- | --- | --- |
| [LADR-001](#ladr-001-cjson-パッチ-0003-のビルド時検査案件の前提と範囲) | cJSON パッチ 0003 のビルド時検査案件の前提と範囲 | 2026-11-28 | 初期依頼 (2026-11-28 対話)、対策元 `261126-cjson-patch-check-backlog.md` (BL-001) |
| [LADR-002](#ladr-002-パッチ-0003-の適用マーカーを-cjsonh-に定義する) | パッチ 0003 の適用マーカーを cJSON.h に定義する | 2026-11-28 | [Q-001](261128-cjson-patch-marker-discussion.md#q-001-マーカー-マクロの定義場所と名称) |
| [LADR-003](#ladr-003-検査を-1-か所の検査用ヘッダーに定義し利用側が取り込む) | 検査を 1 か所の検査用ヘッダーに定義し、利用側が取り込む | 2026-11-28 | [Q-002](261128-cjson-patch-marker-discussion.md#q-002-ビルド時検査を配置する範囲) |
| [LADR-004](#ladr-004-マーカーの未定義と値の不一致でコンパイルを失敗させる) | マーカーの未定義と値の不一致でコンパイルを失敗させる | 2026-11-28 | [Q-003](261128-cjson-patch-marker-discussion.md#q-003-マーカー未定義や値不一致時のコンパイル失敗方式)、[Q-007](261128-cjson-patch-marker-discussion.md#q-007-マーカーの値を検証する静的アサートの表記) |
| [LADR-005](#ladr-005-動的リンクの取り違えの実行時照合は対象外とする) | 動的リンクの取り違えの実行時照合は対象外とする | 2026-11-28 | [Q-004](261128-cjson-patch-marker-discussion.md#q-004-動的リンク取り違えへの実行時照合を今回の範囲に含めるか) |
| [LADR-006](#ladr-006-マーカーの出力とコンパイルの失敗を-bin_test-で自動検証する) | マーカーの出力とコンパイルの失敗を bin_test で自動検証する | 2026-11-28 | [Q-005](261128-cjson-patch-marker-discussion.md#q-005-コンパイル失敗の確認の自動化方針) |
| [LADR-007](#ladr-007-検査の導入に合わせて-cjsonporterstruct-meta-の文書を更新する) | 検査の導入に合わせて cjson、porter、struct-meta の文書を更新する | 2026-11-28 | [Q-006](261128-cjson-patch-marker-discussion.md#q-006-関連ドキュメントの更新範囲)、[Q-008](261128-cjson-patch-marker-discussion.md#q-008-struct-meta-へ追加する記述の範囲) |
| [LADR-008](#ladr-008-cjson_jsoncc-の既存の-_static_assert-を-static_assert-へ統一する) | cJSON_JSONC.c の既存の _Static_assert を static_assert へ統一する | 2026-11-28 | [Q-009](261128-cjson-patch-marker-discussion.md#q-009-jsonc-の実装の既存の静的アサートの表記を直すか) |

## LADR-001 cJSON パッチ 0003 のビルド時検査案件の前提と範囲

決定者: ユーザー (初期依頼)  
最終更新日: 2026-11-28

### 背景と課題

`app/cjson` が提供する JSONC の並行実行の契約は、同梱の cJSON 本体へ適用する独自パッチ `patches/0003-thread-local-global-error.patch` に依存しています。しかし、パッチが適用されていることをビルド時に機械的に検査する仕組みがなく、対策元の残課題一覧 (`261126-cjson-patch-check-backlog.md`) に BL-001 として記録されていました。本案件の前提と検討対象を明確にする必要があります。

### 得られた事実

- 展開された cJSON 本体 (`app/cjson/prod/include/cJSON.h`、`prod/include/cJSON_Utils.h`、`prod/libsrc/cjson/cJSON.c`、`prod/libsrc/cjson/cJSON_Utils.c`) は `app/cjson/packages/cJSON-1.7.19.zip` からの自動生成物であり、手動変更の対象外である (`AGENTS.md`、`app/cjson/AGENTS.md`、`app/cjson/patches/README.md`)。
- `app/cjson/makepart.mk` が Makefile 読み込み時に `app/cjson/bin_internal/extract_package.py` を呼び出し、`framework/makefw/bin_internal/apply_patches.py` で `app/cjson/patches/*.patch` を昇順に厳密適用したうえで、`framework/makefw/bin_internal/generate_symbol_namespace.py` で公開シンボルへ `cjson_app_` 接頭辞を付与している。
- 現在の `0003-thread-local-global-error.patch` は `prod/libsrc/cjson/cJSON.c` だけを変更しており、`prod/include/cJSON.h` には変更を加えていない。
- `framework/makefw/makefiles/_flags.mk` の既定規格は C17 (`-std=c17 -Wpedantic` / `/std:c17 /Zc:preprocessor`) および C++17 (`-std=c++17 -Wpedantic` / `/std:c++17 /Zc:__cplusplus /Zc:preprocessor /permissive-`) である。

### 決定

対策元の残課題一覧 (`261126-cjson-patch-check-backlog.md`) の BL-001 を対象とする継続作業 `261128-cjson-patch-marker` を `app/cjson/docs/prompts/current/` で進める。展開された cJSON 本体への手動編集は行わず、パッチ機構および第一者管理コードの範囲でビルド時検査の方式を検討・合意したうえで設計と実装へ進む。

### 経緯

| 日時 (JST) | 源泉 | ユーザーの選択 | ユーザーのコメント | 決定への反映 |
| --- | --- | --- | --- | --- |
| 2026-11-28 08:33 | 初期依頼 (2026-11-28 対話) | `261126-cjson-patch-check-backlog.md` の BL-001 を継続作業 `261128-cjson-patch-marker` として検討開始 | cJSON 本体は外部配布物であり手動変更の対象外。フェーズ 1 の検討文書一式を作成し、ユーザー回答の受付で停止する | 新規に決定 |

## LADR-002 パッチ 0003 の適用マーカーを cJSON.h に定義する

決定者: ユーザー ([Q-001](261128-cjson-patch-marker-discussion.md#q-001-マーカー-マクロの定義場所と名称))  
最終更新日: 2026-11-28

### 背景と課題

パッチ `0003-thread-local-global-error.patch` が適用されていることを、ビルド時に判定できる印 (マーカー) が必要です。マーカーをパッチの外の第一者管理のヘッダーに置くと、パッチを外した場合や、パッチのない `cJSON.h` を取り込んだ場合にも定義が残り、異常を検出できません。

### 得られた事実

- 現在のパッチ `0003` は `prod/libsrc/cjson/cJSON.c` だけを変更している (`app/cjson/patches/0003-thread-local-global-error.patch`)。
- `framework/makefw/bin_internal/apply_patches.py` は、1 つのパッチ ファイルに複数ファイルの差分を含める形式に対応している。
- `app/cjson/bin_internal/extract_package.py` はパッチの適用後に `generate_symbol_namespace.py` で `prod/include/cJSON.h` の識別子を付け替えるが、対象は `prod/public_symbols.txt` と `prod/public_api_macros.txt` の識別子に限られる。
- 展開された `prod/include/cJSON.h` の先頭には、パッチ `0001-windows-dll-import-default.patch` が追加したブロックがある。

### 決定

パッチ `0003-thread-local-global-error.patch` に `prod/include/cJSON.h` への差分を追加し、インクルード ガードの内側に `#define CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR 1` を定義する。マーカーとスレッド ローカル化の変更を同じパッチ ファイルに収める。

### 経緯

| 日時 (JST) | 源泉 | ユーザーの選択 | ユーザーのコメント | 決定への反映 |
| --- | --- | --- | --- | --- |
| 2026-11-28 12:51 | [Q-001](261128-cjson-patch-marker-discussion.md#q-001-マーカー-マクロの定義場所と名称) (対話セッションでの回答) | A: パッチ 0003 に `prod/include/cJSON.h` への差分を追加し、インクルード ガード内に `#define CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR 1` を定義する | なし | 新規に決定 |

## LADR-003 検査を 1 か所の検査用ヘッダーに定義し、利用側が取り込む

決定者: ユーザー ([Q-002](261128-cjson-patch-marker-discussion.md#q-002-ビルド時検査を配置する範囲))  
最終更新日: 2026-11-28

### 背景と課題

JSONC の並行実行の契約は `cJSON_JSONC.c` が提供し、`app/porter`、`app/struct-meta`、`app/cplat` が cJSON を利用します。`cJSON_JSONC.h` を経由せずに `<cJSON.h>` だけを取り込む利用側もあるため、検査を置く範囲を決める必要があります。

### 得られた事実

- `app/cjson/prod/include/cJSON_JSONC.h` は `<cJSON.h>` を取り込み、`cJSON_JSONC.c`、`app/porter` の `config_document.c`、`app/cplat` の `sym_loader_init.c`、`app/porter` の `configDecodeTest.cc`、`app/cjson` のテストが `cJSON_JSONC.h` を取り込む。
- `cJSON_JSONC.h` を経由せずに cJSON の公開ヘッダーを取り込む `app/cjson` の外の箇所は、`app/porter` の `potr_config_list_service_ids.c`、`potr_config_validate_file.c`、`potr_service_get_type.c`、`potr_service_open_from_config.c`、`config_decode.c`、および `app/struct-meta` の公開ヘッダー `struct_meta/json/json.h` (と、`json.h` をあわせて取り込む `json/decode.c`、`json/encode.c`、`jsonEncodeTest.cc`) である (ワークスペース全体の grep による。framework とほかの app には該当がない)。
- `app/cjson/test/include_override/cJSON.h` と `cJSON_JSONC.h` は、実ヘッダーの `prod/include/` の同名ヘッダーを取り込む。

### 決定

検査は、`app/cjson` の公開ヘッダーとして検査だけを持つ検査用のヘッダー 1 か所に定義する。`cJSON_JSONC.h` はこの検査用のヘッダーを取り込み、`cJSON_JSONC.h` を経由せずに cJSON の公開ヘッダーを取り込む利用側 (`app/porter` の API の 4 ファイルと `config_decode.c`、`app/struct-meta` の json 関連など) も、同じ検査用のヘッダーを取り込む。検査の記述を各所へ複写しない。検査用のヘッダーの名前と各箇所の取り込みの配置は、最終仕様で定める。

### 経緯

| 日時 (JST) | 源泉 | ユーザーの選択 | ユーザーのコメント | 決定への反映 |
| --- | --- | --- | --- | --- |
| 2026-11-28 12:51 | [Q-002](261128-cjson-patch-marker-discussion.md#q-002-ビルド時検査を配置する範囲) (対話セッションでの回答) | C: `cJSON_JSONC.h` に配置したうえで、`<cJSON.h>` のみを取り込む利用側にも検査を追加する | 推奨は A だったが C を選んだ。同じ検査を各所に複写せず、1 か所に定義した検査用のヘッダー (例: `app/cjson` の公開ヘッダーとして検査だけを持つもの) を各利用側が取り込む形など、重複のない構成を設計する。`cJSON.h` を直接取り込む全利用側を grep で洗い出して列挙する | 新規に決定 |

## LADR-004 マーカーの未定義と値の不一致でコンパイルを失敗させる

決定者: ユーザー ([Q-003](261128-cjson-patch-marker-discussion.md#q-003-マーカー未定義や値不一致時のコンパイル失敗方式)、[Q-007](261128-cjson-patch-marker-discussion.md#q-007-マーカーの値を検証する静的アサートの表記))  
最終更新日: 2026-11-28

### 背景と課題

マーカーが未定義の場合と、値が期待値 (`1`) と異なる場合に、分かりやすいメッセージでコンパイルを失敗させる方式が必要です。検査用のヘッダーは C と C++ の両方から取り込まれます。

### 得られた事実

- `framework/makefw/makefiles/_flags.mk` の既定の規格は C17 と C++17 で、GCC では `-Wpedantic` を指定する。
- 静的アサートに未定義のマクロを渡すと、指定したメッセージではなく未宣言の識別子の誤りになる。GCC 8.5 の試作では、静的アサートを `#else` の分岐に入れないと、未定義のときに `#error` に続いてこの誤りも出力された。
- `app/general/docs/coding-guideline.md` の「R2 制約を静的に検査する」は、静的アサートの表記を `static_assert` (`<assert.h>`) とし、既存の `_Static_assert` も `static_assert` へ直すと定めている。
- C17 の `<assert.h>` は `static_assert` を提供し、C++17 では `static_assert` がキーワードである。一時ディレクトリでの GCC (`-std=c17` および `-std=c++17`、`-Wall -Wextra -Wpedantic -Werror`) 試作で、`#include <assert.h>` と `static_assert` の 1 通りの表記により、正常系・未定義・値不一致の期待動作を確認した。

### 決定

検査用のヘッダーで `<assert.h>` と `<cJSON.h>` を取り込み、マーカーが未定義の場合は `#ifndef` と `#error` でコンパイルを停止し、定義済みの場合は `#else` の分岐で、C と C++ の両方で `static_assert` を用いて値が `1` であることを検証する。`__cplusplus` による表記の分岐は設けない。

### 経緯

| 日時 (JST) | 源泉 | ユーザーの選択 | ユーザーのコメント | 決定への反映 |
| --- | --- | --- | --- | --- |
| 2026-11-28 12:51 | [Q-003](261128-cjson-patch-marker-discussion.md#q-003-マーカー未定義や値不一致時のコンパイル失敗方式) (対話セッションでの回答) | A: `#ifndef` で `#error` を出力し、定義済みの場合は C++ で `static_assert`、C で `_Static_assert` を用いて値 `1` を検証する | なし | 新規に決定。表記の使い分けが規範の R2 と一致しないため、静的アサートの表記は [Q-007](261128-cjson-patch-marker-discussion.md#q-007-マーカーの値を検証する静的アサートの表記) で確認中 |
| 2026-11-28 13:02 | [Q-007](261128-cjson-patch-marker-discussion.md#q-007-マーカーの値を検証する静的アサートの表記) (対話セッションでの回答) | A: 規範の R2 に従い、検査用のヘッダーで `<assert.h>` を取り込み、C と C++ の両方で `static_assert` を使う (`#ifndef` で `#error`、`#else` で `static_assert` の構成は維持) | なし | 決定を更新。静的アサートの表記を `<assert.h>` + `static_assert` で確定 |
| 2026-11-28 16:33 | フェーズ 4 第 2 段のレビュー指摘 (対話セッション) | `#error` と `static_assert` のメッセージを `cJSON_JSONC.c` に合わせて日本語化 | MSVC での `/utf-8` 付与を確認したうえで日本語メッセージへ変更し、`test_patch_marker_check.py` も合わせる | `framework/makefw/makefiles/_flags.mk` (`FILES_LANG=ja_JP.utf8` で `/utf-8` 付与) を確認し、日本語メッセージへ更新 |

## LADR-005 動的リンクの取り違えの実行時照合は対象外とする

決定者: ユーザー ([Q-004](261128-cjson-patch-marker-discussion.md#q-004-動的リンク取り違えへの実行時照合を今回の範囲に含めるか))  
最終更新日: 2026-11-28

### 背景と課題

ビルド時の検査では、動的リンクでパッチのない `libcjson_app` を実行時に読み込む取り違えを検出できません。対策元の残課題 (BL-001) は、パッチの版を返す関数の公開とロード時の照合を候補に挙げていました。

### 得られた事実

- `app/cjson` はすべての公開シンボルへ `cjson_app_` 接頭辞を付け、OS 標準の `libcjson` とシンボル名が衝突しない (`app/cjson/README.md`)。
- 公開関数を追加すると、`mock_cjson` の API 表と、`exportTest.cc` の全エクスポートの一致の検査も変更が必要になる。

### 決定

今回はビルド時の検査だけを対象とし、動的リンクの取り違えへの実行時照合は対象外とする。残課題一覧にも記録しない。公開 API、`mock_cjson` の API 表、`exportTest.cc` は変更しない。

### 経緯

| 日時 (JST) | 源泉 | ユーザーの選択 | ユーザーのコメント | 決定への反映 |
| --- | --- | --- | --- | --- |
| 2026-11-28 12:51 | [Q-004](261128-cjson-patch-marker-discussion.md#q-004-動的リンク取り違えへの実行時照合を今回の範囲に含めるか) (対話セッションでの回答) | A: ビルド時検査のみを対象とし、実行時照合は対象外とする | 可能性は低く、残課題にする必要もない | 新規に決定 |

## LADR-006 マーカーの出力とコンパイルの失敗を bin_test で自動検証する

決定者: ユーザー ([Q-005](261128-cjson-patch-marker-discussion.md#q-005-コンパイル失敗の確認の自動化方針))  
最終更新日: 2026-11-28

### 背景と課題

パッチを当てた状態のビルドは通常の `make` で確認できますが、パッチのない `cJSON.h` でコンパイルが失敗することは、通常のビルドでは確認されません。

### 得られた事実

- `app/cjson/bin_test/test_extract_package.py` は、一時ディレクトリで `extract_package.py` を実行する Python の単体テストである。
- `app/cjson/bin_test/test_namespace_coexistence.py` は、Linux だけで実行し、それ以外では理由を `[Linux]` で始めてスキップする。
- `app/general/bin_internal/run-bin-tests.py` は、ツールの不足によるスキップを失敗として扱い、OS に依存するスキップは理由が `[Linux]` または `[Windows]` で始まる場合だけ認める。

### 決定

`app/cjson/bin_test/test_extract_package.py` に、パッチの適用後の `cJSON.h` にマーカーが出力されることの検証を追加する。あわせて、Linux 向けに、パッチのない `cJSON.h` でコンパイルが失敗することを GCC で自動検証する Python のテストを `app/cjson/bin_test/` に追加する。MSVC でのコンパイルの失敗は自動検証しない。

### 経緯

| 日時 (JST) | 源泉 | ユーザーの選択 | ユーザーのコメント | 決定への反映 |
| --- | --- | --- | --- | --- |
| 2026-11-28 12:51 | [Q-005](261128-cjson-patch-marker-discussion.md#q-005-コンパイル失敗の確認の自動化方針) (対話セッションでの回答) | A: マーカーの出力の検証と、Linux 向けのコンパイルの失敗の自動検証を追加する | なし | 新規に決定 |

## LADR-007 検査の導入に合わせて cjson、porter、struct-meta の文書を更新する

決定者: ユーザー ([Q-006](261128-cjson-patch-marker-discussion.md#q-006-関連ドキュメントの更新範囲)、[Q-008](261128-cjson-patch-marker-discussion.md#q-008-struct-meta-へ追加する記述の範囲))  
最終更新日: 2026-11-28

### 背景と課題

`app/cjson/patches/README.md` はパッチ `0003` の対象を `prod/libsrc/cjson/cJSON.c` だけと記載しており、マーカーの追加で実装と一致しなくなります。`app/cjson` と `app/porter` の文書は、パッチを当てていない cJSON と組み合わせた場合の注意を記載しています。`app/struct-meta` にも、検査の導入に合わせた記述の要否と配置先を定める必要があります。

### 得られた事実

- パッチ `0003` に触れた記述は、`app/cjson` の `patches/README.md`、`README.md`、`prod/include/cJSON_JSONC.h` (2 か所)、`prod/libsrc/cjson/jsonc/cJSON_JSONC.c`、`prod/include_internal/cjson_app_thread_local.h`、および `app/porter` の `docs/api.md` と `prod/include/porter/porter_spec.h` (3 か所) にある。
- `app/struct-meta` には、パッチ `0003`、スレッド ローカル、並行実行に触れた記述が存在しない。`app/struct-meta/docs/architecture.md` の「ビルド」の節には cJSON 実行時ライブラリの配置に関する記述がある。

### 決定

`app/cjson` の `patches/README.md`、`prod/include/cJSON_JSONC.h` の Doxygen コメント、`README.md`、および `app/porter` の `docs/api.md` と `prod/include/porter/porter_spec.h` を、パッチ `0003` の適用がビルド時に検査される旨へ更新する。あわせて、`app/struct-meta/docs/architecture.md` の「ビルド」の節に、パッチを当てた cJSON を前提とし `struct_meta/json/json.h` でビルド時に検査する旨の段落を追加する。`struct_meta/json/json.h` の取り込み行にはコメントを付けない。

### 経緯

| 日時 (JST) | 源泉 | ユーザーの選択 | ユーザーのコメント | 決定への反映 |
| --- | --- | --- | --- | --- |
| 2026-11-28 12:51 | [Q-006](261128-cjson-patch-marker-discussion.md#q-006-関連ドキュメントの更新範囲) (対話セッションでの回答) | C: 案 A に加えて、`app/porter` 側の関連コメント・文書も更新する | 案 A の範囲に加え、`app/porter` の `api.md`・`porter_spec.h`、`app/struct-meta` 側の関連コメント・文書も、検査で担保される旨へ更新する | 新規に決定。`app/struct-meta` には更新する既存の記述がないため、追加する記述の範囲は [Q-008](261128-cjson-patch-marker-discussion.md#q-008-struct-meta-へ追加する記述の範囲) で確認中 |
| 2026-11-28 13:02 | [Q-008](261128-cjson-patch-marker-discussion.md#q-008-struct-meta-へ追加する記述の範囲) (対話セッションでの回答) | A: `docs/architecture.md` の「ビルド」の節に段落を追加する。取り込みの行にはコメントを付けない | なし | 決定を更新。`app/struct-meta` の更新対象を確定 |

## LADR-008 cJSON_JSONC.c の既存の _Static_assert を static_assert へ統一する

決定者: ユーザー ([Q-009](261128-cjson-patch-marker-discussion.md#q-009-jsonc-の実装の既存の静的アサートの表記を直すか))  
最終更新日: 2026-11-28

### 背景と課題

`app/general/docs/coding-guideline.md` の「R2 制約を静的に検査する」は、静的アサートの表記を `static_assert` (`<assert.h>`) とし、既存の `_Static_assert` も `static_assert` へ直すと定めています。`app/` 配下の第一者管理の製品コードで `_Static_assert` が残っている箇所の扱いを決める必要があります。

### 得られた事実

- `app/` 配下の第一者管理の製品コード (外部由来の `app/lua`、`app/sqlite`、および生成器が出力するコードの文字列を除く) で `_Static_assert` が残っているのは、`app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c` の 47〜50 行目の 3 か所だけである。

### 決定

今回の案件で、`app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c` に `#include <assert.h>` を追加し、47〜50 行目の `_Static_assert` 3 か所を `static_assert` へ置き換える。

### 経緯

| 日時 (JST) | 源泉 | ユーザーの選択 | ユーザーのコメント | 決定への反映 |
| --- | --- | --- | --- | --- |
| 2026-11-28 13:02 | [Q-009](261128-cjson-patch-marker-discussion.md#q-009-jsonc-の実装の既存の静的アサートの表記を直すか) (対話セッションでの回答) | A: 今回の案件で、`cJSON_JSONC.c` の `_Static_assert` 3 か所を `static_assert` へ直し、`<assert.h>` を取り込む | なし | 新規に決定 |
