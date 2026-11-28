<!--
生成元: agree-before-implement スキル
文書種別: 検討用 Markdown (discussion)
最終更新日時: 2026-11-28 13:02:51 JST
最終更新エージェント: agy / セッション ID: af98b530-c6a7-4b3a-973f-a6b64a55d79c
稼働 OS: Linux (WSL)
上流文書: `261128-cjson-patch-marker-overview.md` (全体管理文書)。削除されている場合があります。
下流文書: `261128-cjson-patch-marker-lightadr.md` (簡易 ADR)、`261128-cjson-patch-marker-design.md` (最終仕様)、`261128-cjson-patch-marker-backlog.md` (残課題一覧)。未作成の場合や、削除されている場合があります。
再開手順: このファイルを開いたエージェントは、作業の前に agree-before-implement スキルを読み込んでください。ユーザーが編集した最新版を読み直し、上流文書の全体管理文書が存在する場合は、その再開手順に従って作業 WBS の `← **CURRENT**` の項目から再開します。存在しない場合は、各 Q の **Status** から未回答と未解決の Q を特定し、扱いをユーザーに確認してください。この文書には Q&A の本文だけを置き、新しい Q は末尾へ追記します。作業 WBS、質問一覧、観点別の確認などの管理情報をこの文書へ追加しないでください。最終更新エージェントのセッション情報や稼働 OS・最終更新日時が参照可能であれば再開時のコンテキストや環境差分の把握に利用し、別エージェントや別環境・別 OS からの引き継ぎ、担当変更によりセッションが見つからない場合も異常とせず、文書の記載から継続してください。
-->

# cJSON パッチ 0003 の適用をビルド時に検査する仕組み 事前検討

ファイル名: `261128-cjson-patch-marker-discussion.md`

## 明確化のための確認項目

### Q-001 マーカー マクロの定義場所と名称

**Status**

解決済み

**Question**

パッチ `0003-thread-local-global-error.patch` が適用済みであることを示すマーカー マクロを、どのファイルにどのような名称と値で定義しますか。

**Context**

- **経緯と背景**: `app/cjson` は、`packages/cJSON-1.7.19.zip` から展開した cJSON 本体 (`prod/include/cJSON.h`、`prod/libsrc/cjson/cJSON.c` など) へ、`app/cjson/patches/` の unified diff を `framework/makefw/bin_internal/apply_patches.py` で昇順適用しています。現在の `0003-thread-local-global-error.patch` は `prod/libsrc/cjson/cJSON.c` だけを変更しており、`prod/include/cJSON.h` には変更を加えていません。展開された cJSON 本体は外部配布物由来の生成物であり、手動変更の対象外です (`app/cjson/AGENTS.md`、`app/cjson/patches/README.md`)。
- **事前調査の結果**:
    - `framework/makefw/bin_internal/apply_patches.py` (`_parse_patch_file`) は、1 つの `.patch` ファイルに複数ファイル (`prod/include/cJSON.h` と `prod/libsrc/cjson/cJSON.c`) の差分を含める形式に対応しています。
    - `app/cjson/bin_internal/extract_package.py` はパッチ適用後に `framework/makefw/bin_internal/generate_symbol_namespace.py` (`transform_public_header`) で `prod/include/cJSON.h` の公開シンボルへ `cjson_app_` を付与しますが、置換対象は `prod/public_symbols.txt` と `prod/public_api_macros.txt` に記載された識別子に限られます。そのため、パッチで `cJSON.h` に追加した独自マクロ名は書き換えられません。
    - 展開された `prod/include/cJSON.h` の先頭には、すでにパッチ `0001-windows-dll-import-default.patch` による追加ブロックがあります。`#define CJSON_VERSION_PATCH 19` 付近またはインクルード ガード (`#define cJSON__h`) の直後にマーカーを配置すると、ヘッダーが取り込まれたときに確実に参照できます。
    - 第一者管理のヘッダー (`prod/include_internal/cjson_app_thread_local.h` や `prod/include/cJSON_JSONC.h`) にマーカーを定義すると、パッチ `0003` を外した状態や、未パッチの `cJSON.h` (システムの `cJSON.h` や上流配布物) を取り込んだ状態でもマーカーが定義されたままになり、異常を検出できません。
- **各案の具体例**:

```c
/* 案 A (推奨): 0003-thread-local-global-error.patch で prod/include/cJSON.h にも差分を追加 */
/* In prod/include/cJSON.h (インクルード ガード内) */
/* PATCH: 0003-thread-local-global-error.patch の適用マーカー */
#define CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR 1
```

```c
/* 案 B: 第一者管理ヘッダー (cjson_app_thread_local.h など) に定義 */
/* パッチ 0003 が外れても定義が残るため、未適用や他版 cJSON.h の混入を検出できない */
#define CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR 1
```

**Answer**

選択方式: 単一選択

- [x] A: (推奨) `0003-thread-local-global-error.patch` に `prod/include/cJSON.h` への差分を追加し、インクルード ガード内に `#define CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR 1` を定義する。
- [ ] B: パッチ `0003` は `cJSON.c` の変更にとどめ、第一者管理のヘッダー (`prod/include_internal/cjson_app_thread_local.h` または `prod/include/cJSON_JSONC.h`) にマーカーを定義する。
- [ ] C: マーカー定義を別のパッチ ファイル (`0004-...patch`) に分離して `prod/include/cJSON.h` へ追加する。
- [ ] D: その他 (別のマクロ名や配置場所の指定を含む)
- [ ] X: 意図不明 (詳細な質問内容に再生成すること)

その他の回答 / 補足:

回答者: Tetsuo Honda  
回答日時: 2026-11-28 12:51:36 JST (対話セッションでの回答「Q-001: A」を、エージェントが記入)

**Assessment**

選択された回答を確認しました。推奨案と一致し、ほかの回答、参照資料との矛盾はありません。`framework/makefw/bin_internal/apply_patches.py` は 1 つのパッチ ファイルに複数ファイルの差分を含める形式に対応しており、パッチ `0003` は `prod/include/cJSON.h` (パッチ `0001` の適用後の内容) と `prod/libsrc/cjson/cJSON.c` の 2 ファイルを変更するパッチになります。マクロ名 `CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR` は `prod/public_symbols.txt` と `prod/public_api_macros.txt` に含まれないため、`generate_symbol_namespace.py` の名前の付け替えを受けません。インクルード ガード内の具体的な挿入位置と差分は、最終仕様で定めます。

**Recommendation**

推奨: A  
優先した基準: パッチ本体とマーカーの不可分性 (誤検知・見逃しの防止) および上流更新時の保守性を優先しました。  
理由: 案 A であれば、パッチ `0003` の適用とマーカーの存在が同一パッチ ファイルに結び付くため、パッチ `0003` が外れた場合や未パッチの `cJSON.h` を取り込んだ場合を確実に検出できます。案 B はパッチ未適用を検出できず本案件の目的を満たしません。案 C はパッチ `0003` だけ外して `0004` が残る不整合の余地を生みます。

**Related**

- [Q-002](#q-002-ビルド時検査を配置する範囲)
- [Q-006](#q-006-関連ドキュメントの更新範囲)

### Q-002 ビルド時検査を配置する範囲

**Status**

解決済み

**Question**

マーカーを用いたビルド時検査を、どのファイルの範囲に配置しますか。

**Context**

- **経緯と背景**: 並行実行の契約 (`global_error` がスレッド ローカルであること) は、JSONC ラッパー (`cJSON_JSONC.c`) が提供し、複数の利用側 app がその前提に依存しています。どの経路で `cJSON.h` が取り込まれているかを全 app 横断で調査しました。
- **事前調査の結果 (`cJSON.h` の取り込み経路)**:
    - `app/cjson/prod/include/cJSON_JSONC.h` (17 行目) は `#include <cJSON.h>` を取り込み、`app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c` (14 行目) は `#include <cJSON_JSONC.h>` を取り込みます。
    - `cJSON_JSONC.h` を取り込む利用側は、`app/porter/prod/libsrc/porter/protocol/config_document.c` (13 行目) に加え、`app/cplat/prod/libsrc/cplat/runtime/sym_loader_init.c` (25 行目) も該当します。
    - 一方、`app/struct-meta/prod/libsrc/struct_meta/json/file.c` (`struct_meta_json_file_load`) は `<struct_meta/json/json.h>` 経由で `<cJSON.h>` のみを取り込み、`<cJSON_JSONC.h>` は取り込んでいません。`app/porter` の API ファイル (`potr_config_list_service_ids.c`、`potr_config_validate_file.c`、`potr_service_get_type.c`、`potr_service_open_from_config.c`) も `<cJSON.h>` のみです。
    - `app/cjson/appdeps.mk` は `APP_PROD_INCLUDE_CLASS := system` を指定しており、makefw (`framework/makefw/makefiles/prepare.mk`) は利用側へ `-isystem` (Linux) または `/external:I` (Windows) で `app/cjson/prod/include` を渡します。通常ビルドでは標準の `/usr/include` より優先されますが、利用側が `-I` で外部パスを優先した場合や `APP_DEPS` に `cjson` を書き忘れた場合は別版の `cJSON.h` を取り込む余地があります。
    - `app/cjson/test/include_override/cJSON_JSONC.h` は実ヘッダーの `prod/include/cJSON_JSONC.h` を取り込むため、`cJSON_JSONC.h` に検査を置けばモック利用テストも含めて同じ検査が働きます。
- **各案の違い**:
    - 案 A: 公開ヘッダー `app/cjson/prod/include/cJSON_JSONC.h` の `#include <cJSON.h>` の直後に配置します。これにより、`cJSON_JSONC.c` 自身と、`cJSON_JSONC.h` を取り込む全利用側 (`app/porter`、`app/cplat`、テスト) に自動で検査が効きます。
    - 案 B: `app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c` だけに配置します。公開ヘッダーを変更せずに済みますが、利用側が誤った `cJSON.h` を取り込んだケースは検出できません。
    - 案 C: 案 A に加えて、`<cJSON.h>` のみを取り込み並行実行に依存する利用側 (`app/struct-meta/prod/libsrc/struct_meta/json/file.c` 等) にも個別に同じ検査を配置します。検出範囲は最も広くなりますが、検査記述が複数リポジトリへ分散します。

**Answer**

選択方式: 単一選択

- [ ] A: (推奨) 公開ヘッダー `app/cjson/prod/include/cJSON_JSONC.h` に配置し、`cJSON_JSONC.c` および `cJSON_JSONC.h` を取り込む利用側 (`app/porter`、`app/cplat` 等) のすべてに効かせる。
- [ ] B: `app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c` のみに配置する (`libcjson_app` 自身のビルド時検査に限定)。
- [x] C: `app/cjson/prod/include/cJSON_JSONC.h` に配置したうえで、`<cJSON.h>` のみを取り込む利用側 (`app/struct-meta/prod/libsrc/struct_meta/json/file.c` など) にも個別に検査を追加する。
- [ ] D: その他
- [ ] X: 意図不明 (詳細な質問内容に再生成すること)

その他の回答 / 補足:

対話セッションでの補足 (要旨): `<cJSON.h>` のみを取り込む利用側 (`app/struct-meta` の json 関連、`app/porter` の API 4 ファイルなど) にも検査を置く。推奨は A だったが C を選んだ。利用側の検査の置き方は、同じ検査を各所に複写するのではなく、1 か所に定義した検査用のヘッダー (例: `app/cjson` の公開ヘッダーとして検査だけを持つもの) を各利用側が取り込む形など、重複のない構成を設計すること。`cJSON.h` を直接取り込む全利用側を grep で洗い出して列挙すること。

回答者: Tetsuo Honda  
回答日時: 2026-11-28 12:51:36 JST (対話セッションでの回答「Q-002: C」と補足を、エージェントが記入)

**Assessment**

選択された回答と補足を確認しました。推奨案 (A) とは異なりますが、補足は選択肢 C の「個別に検査を追加する」を、検査の本体を 1 か所に定義して各所はそれを取り込むだけにする形へ具体化したもので、選択肢と矛盾しません。`cJSON_JSONC.h` も同じ検査用のヘッダーを取り込む形にすれば、検査の記述は 1 か所だけになります。

cJSON の公開ヘッダー (`cJSON.h`、および `cJSON.h` を取り込む `cJSON_Integer.h`、`cJSON_JSONC.h`、`cJSON_Utils.h`) を直接取り込む箇所を、`grep -rnE '#\s*include\s*[<"]([^">]*/)?cJSON(_JSONC|_Utils|_Integer)?\.h[">]'` でワークスペース全体 (`obj/` を除く) から洗い出しました。`<cJSON_JSONC.h>` を経由せずに cJSON の公開ヘッダーを取り込む `app/cjson` の外の箇所は、次のとおりです。

- `app/porter/prod/libsrc/porter/api/potr_config_list_service_ids.c` (`<cJSON.h>`)
- `app/porter/prod/libsrc/porter/api/potr_config_validate_file.c` (`<cJSON.h>`)
- `app/porter/prod/libsrc/porter/api/potr_service_get_type.c` (`<cJSON.h>`)
- `app/porter/prod/libsrc/porter/api/potr_service_open_from_config.c` (`<cJSON.h>`)
- `app/porter/prod/libsrc/porter/protocol/config_decode.c` (`<cJSON.h>`、`<cJSON_Integer.h>`)
- `app/struct-meta/prod/include/struct_meta/json/json.h` (`<cJSON.h>`。公開ヘッダーであり、`json/file.c`、`json/encode.c`、`json/decode.c`、`struct-meta-sample`、`jsonEncodeTest.cc`、`jsonDecodeTest.cc` がこれを取り込みます)
- `app/struct-meta/prod/libsrc/struct_meta/json/decode.c`、`encode.c`、`app/struct-meta/test/src/jsonEncodeTest/jsonEncodeTest.cc` (`<cJSON_Integer.h>`。いずれも `struct_meta/json/json.h` も取り込みます)

`<cJSON_JSONC.h>` を取り込み、`cJSON_JSONC.h` から検査を受け取る箇所は、`app/porter/prod/libsrc/porter/protocol/config_document.c`、`app/cplat/prod/libsrc/cplat/runtime/sym_loader_init.c`、`app/porter/test/src/libporter/configDecodeTest/configDecodeTest.cc`、および `app/cjson` 内の `cJSON_JSONC.c`、`mock_cjson.h`、`cjsonJsoncTest.cc`、`cjsonJsoncDynamicLoadTest.cc`、`exportTest.cc` です。`app/cjson` 内でほかに cJSON の公開ヘッダーを取り込む箇所は、提供側の `cJSON_Integer.h`、`cJSON_Integer_internal.h`、`cJSON_Integer.c`、`test/include_override/` の 4 ファイル、および利用側の `prod/src/cmd/cjson_sample/cjson_sample.c`、`test/src/cmd/cjsonTest/cjsonTest.cc`、`test/src/cmd/cjsonIntegerTest/cjsonIntegerTest.cc`、`test/src/libcjsonTest/mockCjsonTest/caller_info.c`、`caller_info_cpp.cc` です。framework と、ほかの app (`app/calc` など) には該当がありません。

検査用のヘッダーの名前、各箇所への取り込みの配置、`app/cjson` 内の利用側の扱いは、この回答の補足に従って最終仕様で設計します。

**Recommendation**

推奨: A  
優先した基準: 影響リポジトリ・変更箇所の最小化と、契約提供側・JSONC 利用側の双方を漏れなく検査する保守性の両立を優先しました。  
理由: `cJSON_JSONC.h` へ 1 箇所だけ配置すれば、`cJSON_JSONC.c` とすべての JSONC 利用側 (`porter`、`cplat`) に検査が行き渡ります。`struct-meta` の `file.c` は `cJSON_JSONC.h` を経由しないため、そこも含めて個別に検査したい場合は案 C を選択してください。

**Related**

- [Q-001](#q-001-マーカー-マクロの定義場所と名称)
- [Q-003](#q-003-マーカー未定義や値不一致時のコンパイル失敗方式)

### Q-003 マーカー未定義や値不一致時のコンパイル失敗方式

**Status**

解決済み

**Question**

マーカーが未定義または期待値 (`1`) と異なる場合、どの構文でコンパイルを失敗させますか。

**Context**

- **経緯と背景**: 対策元の残課題 (BL-001) では、`static_assert` による検査と、未定義時にも分かりやすいメッセージで失敗させること (`#ifndef` + `#error` の併用、または未定義時に `0` を与えて `static_assert` で失敗させる案) が候補に挙がっています。
- **事前調査の結果 (C/C++ 規格と処理系の互換性)**:
    - `framework/makefw/makefiles/_flags.mk` (5〜114 行目) の既定設定は C17 (`-std=c17 -Wpedantic` / `/std:c17 /Zc:preprocessor`) および C++17 (`-std=c++17 -Wpedantic` / `/std:c++17 /Zc:__cplusplus /Zc:preprocessor /permissive-`) です。
    - C17 のソース (`.c`) では `_Static_assert(expr, msg)` が `<assert.h>` 不要のキーワードとして GCC と MSVC の双方で使えます (`app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c` 47〜50 行目ですでに使用中)。一方、`static_assert` を C17 で使うには `<assert.h>` のインクルードが必要です。
    - C++17 のソース (`.cc`、`.cpp`) では `static_assert(expr, msg)` がキーワードであり、`_Static_assert` は非標準のため `-Wpedantic` や `/permissive-` でエラーまたは警告になります。
    - [Q-002](#q-002-ビルド時検査を配置する範囲) で公開ヘッダー (`cJSON_JSONC.h`) に検査を配置する場合、C と C++ の双方 (`cJSON_JSONC.c`、`cjsonJsoncTest.cc`、`exportTest.cc` 等) から取り込まれるため、C/C++ 両対応の書き方が必要です。
    - 静的アサート (`_Static_assert` / `static_assert`) 単体に未定義マクロを渡すと、指定したメッセージではなく「未定義識別子」の構文エラーになります。
- **各案の具体例**:

```c
/* 案 A (推奨): 未定義は #error で止め、定義済みなら C/C++ 両対応の静的アサートで値を検証 */
#ifndef CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR
    #error "cJSON patch 0003 (thread-local global_error) is not applied"
#else
    #ifdef __cplusplus
        static_assert(CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR == 1,
                      "cJSON patch 0003 (thread-local global_error) version mismatch");
    #else
        _Static_assert(CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR == 1,
                       "cJSON patch 0003 (thread-local global_error) version mismatch");
    #endif
#endif
```

```c
/* 案 B: プリプロセッサの #if と #error のみで完結 (C/C++・規格差の影響を受けない) */
#if !defined(CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR) || (CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR != 1)
    #error "cJSON patch 0003 (thread-local global_error) is not applied"
#endif
```

```c
/* 案 C: 未定義時に補助マクロへ 0 を与えて静的アサートのみで失敗させる */
#ifdef CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR
    #define CJSON_APP_PATCH_0003_CHECK_VALUE CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR
#else
    #define CJSON_APP_PATCH_0003_CHECK_VALUE 0
#endif
#ifdef __cplusplus
    static_assert(CJSON_APP_PATCH_0003_CHECK_VALUE == 1,
                  "cJSON patch 0003 (thread-local global_error) is not applied");
#else
    _Static_assert(CJSON_APP_PATCH_0003_CHECK_VALUE == 1,
                   "cJSON patch 0003 (thread-local global_error) is not applied");
#endif
#undef CJSON_APP_PATCH_0003_CHECK_VALUE
```

**Answer**

選択方式: 単一選択

- [x] A: (推奨) `#ifndef` で `#error` を出力し、定義済みの場合は C++ で `static_assert`、C で `_Static_assert` を用いて値 `1` を検証する。
- [ ] B: `#if !defined(...) || (... != 1)` と `#error` のみで検証する (静的アサートは使わない)。
- [ ] C: 未定義時に補助マクロへ `0` を与えたうえで、`static_assert` (C++) / `_Static_assert` (C) のみで失敗させる。
- [ ] D: その他
- [ ] X: 意図不明 (詳細な質問内容に再生成すること)

その他の回答 / 補足:

回答者: Tetsuo Honda  
回答日時: 2026-11-28 12:51:36 JST (対話セッションでの回答「Q-003: A」を、エージェントが記入)

**Assessment**

選択された回答を確認しました。未定義を `#ifndef` と `#error` で止め、定義済みの場合に値を静的アサートで検証する構成は確定とします。GCC 8.5 の試作 (`-std=c17` と `-std=c++17`、`-Wall -Wextra -Wpedantic -Werror`) では、静的アサートを `#else` の分岐に入れないと、未定義のときに `#error` に続いて未宣言の識別子の誤りも出力されることを確認しました。そのため、`#ifndef` と `#else` で分岐する構成も必要です。

一方、静的アサートの表記には、リポジトリの規範との不一致があります。`app/general/docs/coding-guideline.md` の「R2 制約を静的に検査する」は、表記を `static_assert` (`<assert.h>`) とし、既存の `_Static_assert` も `static_assert` へ直すと定めています。選択肢 A は C で `_Static_assert`、C++ で `static_assert` を使い分ける表記で、この規範と一致しません。この質問の Context は規範に触れておらず、`<assert.h>` を取り込まない理由を名前空間の汚染としていました。静的アサートの表記は [Q-007](#q-007-マーカーの値を検証する静的アサートの表記) で確認し、選択肢 A (`<assert.h>` を取り込み C と C++ の両方で `static_assert` を使う) が採用されたため、本質問を解決済みとしました。

**Recommendation**

推奨: A  
優先した基準: 元の残課題 (BL-001) の要望 (`static_assert` の活用と分かりやすいエラー メッセージ) の充足、および Linux/GCC・Windows/MSVC の C17/C++17 すべてでの警告・エラー回避を優先しました。  
理由: 案 A は未定義と版不一致を明確に区別して失敗でき、`<assert.h>` の追加インクルードによる名前空間汚染も避けられます。プリプロセッサだけの簡潔さを重視する場合は案 B も有効です。

**Related**

- [Q-002](#q-002-ビルド時検査を配置する範囲)
- [Q-007](#q-007-マーカーの値を検証する静的アサートの表記)

### Q-004 動的リンク取り違えへの実行時照合を今回の範囲に含めるか

**Status**

解決済み

**Question**

ビルド時のヘッダー検査に加えて、動的リンクでパッチのない `libcjson_app` (`.so` / `.dll`) を実行時に読み込んだ場合の照合機能 (パッチ版を返す公開関数の追加など) を、今回の案件に含めますか。

**Context**

- **経緯と背景**: 対策元の残課題 (BL-001) の推奨対応策 4 として、動的リンク時の取り違え対策 (パッチ版を返す関数の公開とロード時の照合) が検討候補に挙がっています。
- **事前調査の結果**:
    - `app/cjson` はすべての公開シンボルへ `cjson_app_` 接頭辞を付与しており (`app/cjson/README.md` 8〜14 行目)、OS 標準の `libcjson.so` とシンボル名が衝突して誤リンクする事態はすでに防がれています。
    - 一方、`app/cjson/test/src/libcjsonTest/exportTest/exportTest.cc` (31〜54 行目) は、`mock_cjson_api_table.h` と実ライブラリ `libcjson_app` の全エクスポートが完全一致することを検証しています。そのため、新しい公開関数を追加する場合は、公開ヘッダー、`mock_cjson` の API 表・モック定義・オーバーライド ヘッダー、および利用側でのロード時照合の実装まで変更が広がります。
- **各案の違い**:
    - 案 A: 今回はビルド時検査に絞り、動的リンク取り違えの実行時照合は対象外とします (必要なら残課題として記録します)。ABI と `mock_cjson` への影響がありません。
    - 案 B: 今回の案件で、パッチ版を返す公開関数の追加、`mock_cjson` の追随、および利用側またはロード時の照合まで実装します。

**Answer**

選択方式: 単一選択

- [x] A: (推奨) 今回はビルド時検査のみを対象とし、動的リンク取り違えの実行時照合は対象外とする (必要であれば残課題に記録する)。
- [ ] B: 今回の案件に含め、パッチ版を返す公開関数の追加と実行時照合まで設計・実装する。
- [ ] C: その他
- [ ] X: 意図不明 (詳細な質問内容に再生成すること)

その他の回答 / 補足:

対話セッションでの補足: 可能性は低く、残課題にする必要もない。

回答者: Tetsuo Honda  
回答日時: 2026-11-28 12:51:36 JST (対話セッションでの回答「Q-004: A」と補足を、エージェントが記入)

**Assessment**

選択された回答と補足を確認しました。推奨案と一致し、ほかの回答、参照資料との矛盾はありません。補足に従い、動的リンクの取り違えへの実行時照合は今回の対象外とし、残課題一覧 (`261128-cjson-patch-marker-backlog.md`) にも記録しません。公開 API、`mock_cjson` の API 表、`exportTest.cc` は変更しません。

**Recommendation**

推奨: A  
優先した基準: ABI 影響と変更範囲の最小化を優先しました。  
理由: `cjson_app_` 名前空間分離によりシステム `libcjson` との実行時混在はすでに防止されており、公開 API の追加は `mock_cjson` や複数 app へ波及します。まずビルド時検査を単体で確実に導入するほうが妥当です。

### Q-005 コンパイル失敗の確認の自動化方針

**Status**

解決済み

**Question**

パッチ `0003` が未適用または未パッチの `cJSON.h` を取り込んだときに期待どおりコンパイルが失敗することの確認を、どこまで自動化しますか。

**Context**

- **経緯と背景**: 正常系 (パッチ適用済み) は通常の `make` と既存テストで確認できますが、異常系 (パッチ未適用や未パッチ `cJSON.h` 取り込み時にコンパイルが失敗すること) は通常のビルド ターゲットでは検証されません。
- **事前調査の結果**:
    - `app/cjson/bin_test/test_extract_package.py` は、一時ディレクトリへ `packages/` と `patches/` を複製して `extract_package.py` を実行する Python 単体テストです (Linux と Windows の双方で動作)。ここでパッチ適用後の `cJSON.h` にマーカーが出力されることを検証できます。
    - `app/cjson/bin_test/test_namespace_coexistence.py` は、`BIN_TEST_PLATFORM = "linux"` を指定して Linux 上でコンパイラを呼び出す既存の Python テストです。同様の方式で、一時ディレクトリに未パッチの `cJSON.h` (マーカー未定義およびマーカー値 `0` など) を用意し、GCC でコンパイルが非 0 終了かつ期待メッセージが出力されることを自動検証できます。
    - Windows (MSVC) の `cl.exe` は Developer Command Prompt の環境変数に依存するため、`bin_test` から直接呼び出すと実行環境によって不安定になりやすく、既存の `bin_test` にも MSVC を直接起動するテストはありません。
- **各案の違い**:
    - 案 A: `test_extract_package.py` でのマーカー出力検証 (両 OS) に加え、Linux 環境向けに GCC でのコンパイル失敗を自動検証するテストを `app/cjson/bin_test/` に追加します。
    - 案 B: `test_extract_package.py` のマーカー出力検証だけを自動化し、コンパイル失敗自体は実装時の手動確認 (および検証記録) のみとします。
    - 案 C: Linux (GCC) と Windows (MSVC) の双方でコンパイル失敗を自動検証するテストを `app/cjson/bin_test/` に追加します。

**Answer**

選択方式: 単一選択

- [x] A: (推奨) `app/cjson/bin_test/test_extract_package.py` にマーカー出力検証を追加し、あわせて Linux 向けに未パッチ `cJSON.h` でのコンパイル失敗を自動検証する Python テストを `app/cjson/bin_test/` に追加する。
- [ ] B: `app/cjson/bin_test/test_extract_package.py` のマーカー出力検証のみを自動化し、コンパイル失敗の確認は実装時の手動確認 (検証記録) にとどめる。
- [ ] C: Linux (GCC) と Windows (MSVC) の双方でコンパイル失敗を自動検証するテストを `app/cjson/bin_test/` に追加する。
- [ ] D: その他
- [ ] X: 意図不明 (詳細な質問内容に再生成すること)

その他の回答 / 補足:

回答者: Tetsuo Honda  
回答日時: 2026-11-28 12:51:36 JST (対話セッションでの回答「Q-005: A」を、エージェントが記入)

**Assessment**

選択された回答を確認しました。推奨案と一致し、ほかの回答、参照資料との矛盾はありません。`app/general/bin_internal/run-bin-tests.py` は `bin_test/` の `test_*.py` を unittest で実行し、OS に依存するスキップは理由が `[Linux]` または `[Windows]` で始まる場合だけ認め、ツールの不足によるスキップは失敗として扱います。追加するテストは、Linux 以外では理由を `[Linux]` で始めてスキップし、Linux で GCC が見つからない場合はスキップせずに失敗させます。テストのファイル名と確認内容は最終仕様で定めます。

**Recommendation**

推奨: A  
優先した基準: 回帰防止の自動化と、既存 `bin_test` 規約・クロスプラットフォーム CI での安定性の両立を優先しました。  
理由: `test_extract_package.py` は両 OS で動作し、Linux 向けの負のコンパイル テストは `test_namespace_coexistence.py` と同じパターン (`BIN_TEST_PLATFORM = "linux"`) で安定して自動実行できます。

**Related**

- [Q-001](#q-001-マーカー-マクロの定義場所と名称)
- [Q-002](#q-002-ビルド時検査を配置する範囲)

### Q-006 関連ドキュメントの更新範囲

**Status**

解決済み

**Question**

ビルド時検査の導入に伴い、関連する既存ドキュメントをどこまで更新しますか。

**Context**

- **経緯と背景**: `app/cjson/patches/README.md` (32 行目、38〜40 行目) は、パッチ `0003-thread-local-global-error.patch` の対象を `prod/libsrc/cjson/cJSON.c` のみと記載しています。[Q-001](#q-001-マーカー-マクロの定義場所と名称) で `prod/include/cJSON.h` にも差分を追加する場合、この記載が実装と食い違います。
- **事前調査の結果 (関連する既存文書)**:
    - `app/cjson/patches/README.md` (32 行目、38〜40 行目): 収録パッチ一覧とパッチ `0003` の説明。
    - `app/cjson/prod/include/cJSON_JSONC.h` (56〜58 行目、92〜94 行目): パッチ `0003` への依存に関する Doxygen コメント。
    - `app/cjson/README.md` (50〜53 行目): パッチ `0003` によるスレッド ローカル化の説明。
    - `app/porter/prod/include/porter/porter_spec.h` (470〜471 行目、533〜534 行目、569〜570 行目) および `app/porter/docs/api.md`: パッチ未適用 cJSON とリンクした場合の注意書き。
- **各案の違い**:
    - 案 A: `app/cjson` 内の正本 (`patches/README.md`、`cJSON_JSONC.h` コメント、必要に応じ `app/cjson/README.md`) を最小限更新します。
    - 案 B: `app/cjson/patches/README.md` のみを更新します。
    - 案 C: 案 A に加えて、`app/porter` 側の注意書きにもビルド時検査がある旨を追記します (他サブモジュールへの変更が発生します)。

**Answer**

選択方式: 単一選択

- [ ] A: (推奨) `app/cjson/patches/README.md` (パッチ 0003 の対象とマーカーの目的) と、`app/cjson/prod/include/cJSON_JSONC.h` のコメント (必要に応じて `app/cjson/README.md`) を最小限更新する。
- [ ] B: `app/cjson/patches/README.md` のみを更新する。
- [x] C: 案 A に加えて、`app/porter` 側の関連コメント・文書も更新する。
- [ ] D: その他
- [ ] X: 意図不明 (詳細な質問内容に再生成すること)

その他の回答 / 補足:

対話セッションでの補足 (要旨): 案 A の範囲に加え、`app/porter` の `api.md`・`porter_spec.h`、`app/struct-meta` 側の関連コメント・文書も、検査で担保される旨へ更新する。

回答者: Tetsuo Honda  
回答日時: 2026-11-28 12:51:36 JST (対話セッションでの回答「Q-006: C」と補足を、エージェントが記入)

**Assessment**

選択された回答と補足を確認しました。`app/cjson` の `patches/README.md`、`prod/include/cJSON_JSONC.h` のコメント、`README.md`、および `app/porter` の `docs/api.md` (並行実行の注記) と `prod/include/porter/porter_spec.h` (`potr_service_get_type()`、`potr_config_validate_file()`、`potr_config_list_service_ids()` の「スレッド セーフ」の 3 か所) は、更新の対象が存在するため、範囲を確定とします。

一方、`app/struct-meta` には、パッチ `0003` や cJSON の並行実行に触れたコメントと文書が存在しません。`app/struct-meta` の `prod/`、`docs/`、`README.md` を「パッチ」「スレッド ローカル」「並行」「global_error」で検索して確認しました。`struct_meta/json/file.h` の `struct_meta_json_file_load()` には Doxygen コメントがなく、`docs/architecture.md` の cJSON への言及は、ビルド時の実行時ライブラリの配置と整数拡張 API の利用だけです。そのため、補足の「`app/struct-meta` 側の関連コメント・文書も更新する」は、新たな記述を追加するのか、検査の取り込みだけにとどめるのかが定まりません。この点は [Q-008](#q-008-struct-meta-へ追加する記述の範囲) で確認し、選択肢 A (`docs/architecture.md` の「ビルド」の節に段落を追加し、取り込みの行にはコメントを付けない) が採用されたため、本質問を解決済みとしました。

**Recommendation**

推奨: A  
優先した基準: 正本ドキュメントと実装の整合性維持、および変更対象を `app/cjson` サブモジュール内へ収めることを優先しました。  
理由: `patches/README.md` の更新はパッチ対象ファイルの変更に伴う必須の整合維持であり、`cJSON_JSONC.h` にも検査の意図を簡潔に残すことで保守時の見落としを防げます。`app/porter` 側の注意書きは実行時リンクに関する記述であり、今回のビルド時検査追加で矛盾は生じません。

**Related**

- [Q-001](#q-001-マーカー-マクロの定義場所と名称)
- [Q-002](#q-002-ビルド時検査を配置する範囲)
- [Q-008](#q-008-struct-meta-へ追加する記述の範囲)

### Q-007 マーカーの値を検証する静的アサートの表記

**Status**

解決済み

**Question**

[Q-003](#q-003-マーカー未定義や値不一致時のコンパイル失敗方式) で決めた検査のうち、マーカーの値が `1` であることを検証する静的アサートを、どの表記で記述しますか。

**Context**

- **経緯と背景**: [Q-003](#q-003-マーカー未定義や値不一致時のコンパイル失敗方式) では選択肢 A が選択されました。選択肢 A は、マーカーが未定義なら `#ifndef` と `#error` で止め、定義済みなら C++ で `static_assert`、C で `_Static_assert` を使い分けて値を検証する案です。回答の検証で、この表記がリポジトリのコーディング規範と一致しないことが分かりました。Q-003 の Context は規範に触れておらず、`<assert.h>` を取り込まない理由を名前空間の汚染としていたため、判断の前提が欠けていました。未定義を `#error` で止める構成は、この質問の対象外で、確定しています。
- **根拠資料**:
    - `app/general/docs/coding-guideline.md` の「R2 制約を静的に検査する」は、「表記は `static_assert` (`<assert.h>`) とします。既存の `_Static_assert` は、`static_assert` へ直します。」と定めています。同じ規範の「assert を使用しない」も、コンパイル時に判定できる条件には `static_assert` を使うとしています。
    - C17 の `<assert.h>` は `static_assert` を `_Static_assert` へ展開するマクロとして定義します。C++17 では `static_assert` がキーワードで、`<assert.h>` を取り込んでも `static_assert` の意味は変わりません。そのため、`#include <assert.h>` と `static_assert(...)` の 1 通りの表記で、C と C++ の両方に対応でき、`__cplusplus` による分岐は不要です。
    - `app/cplat/prod/include/cplat/compress/compress.h`、`cplat/net/endpoint.h` などの公開ヘッダーは、`#include <assert.h>` と `static_assert` の形で記述されており、C と C++ の両方から取り込まれ、Linux と Windows の双方でビルドされています。
    - GCC 8.5 で、`-std=c17` と `-std=c++17`、`-Wall -Wextra -Wpedantic -Werror` を指定して試作を確認しました。案 A と案 B のどちらも、マーカーが `1` なら警告なしでコンパイルでき、未定義と値 `2` ではコンパイルが失敗します。MSVC での試作は行っていません。
    - 規範の「assert を使用しない」の検証コマンドは、`#include <assert.h>` も検出の対象にしています。`static_assert` のための `<assert.h>` の取り込みも検出されるため、現時点で `app/` 配下から 84 件が検出されます。案 A では、検査用のヘッダーもこの検出に加わります。この検証コマンドの問題はこの案件と別の既存の問題であり、残課題一覧 (`261128-cjson-patch-marker-backlog.md`) の BL-001 に記録しました。
- **各案の具体例・コード比較**: 次の例の `#error` の行は、どちらの案でも同じです。

```c
/* 案 A (推奨): 規範の R2 に従い、<assert.h> の static_assert を C と C++ で共通に使う */
#include <assert.h>
#include <cJSON.h>

#ifndef CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR
    #error "..."
#else
static_assert(CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR == 1, "...");
#endif
```

```c
/* 案 B: Q-003 の選択肢 A の記述どおり、C と C++ で表記を分ける (規範の R2 と一致しない) */
#include <cJSON.h>

#ifndef CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR
    #error "..."
#else
    #ifdef __cplusplus
static_assert(CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR == 1, "...");
    #else
_Static_assert(CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR == 1, "...");
    #endif
#endif
```

**Answer**

選択方式: 単一選択

- [x] A: (推奨) 規範の R2 に従い、検査用のヘッダーで `<assert.h>` を取り込み、C と C++ の両方で `static_assert` を使う。
- [ ] B: Q-003 の選択肢 A の記述どおり、C++ で `static_assert`、C で `_Static_assert` を使い分ける (規範の R2 と異なる表記を、このヘッダーの例外として扱う)。
- [ ] C: その他
- [ ] X: 意図不明 (詳細な質問内容に再生成すること)

その他の回答 / 補足:

回答者: Tetsuo Honda  
回答日時: 2026-11-28 13:02:51 JST (対話セッションでの回答「Q-007: A」を、エージェントが記入)

**Assessment**

選択された回答を確認しました。推奨案と一致し、`app/general/docs/coding-guideline.md` の「R2 制約を静的に検査する」と整合します。他の回答や参照資料との矛盾はありません。未定義を `#ifndef` と `#error` で止め、`#else` で `static_assert` を用いて値 `1` を検証する構成を確定とし、一時ディレクトリで GCC の `-std=c17` および `-std=c++17` (`-Wall -Wextra -Wpedantic -Werror`) により正常系、未定義、値不一致の期待動作を実測確認しました。[Q-003](#q-003-マーカー未定義や値不一致時のコンパイル失敗方式) もあわせて解決済みとします。

**Recommendation**

推奨: A  
優先した基準: リポジトリの規範との整合を優先しました。  
理由: 案 A は規範の R2 の表記どおりで、`cplat` の公開ヘッダーと同じ形になり、`__cplusplus` の分岐も不要です。案 B は規範から外れるため、規範に例外を追記するか、検査用のヘッダーに例外の理由を記載する必要があります。`<assert.h>` の取り込みは、利用側の翻訳単位へ `assert` マクロを持ち込みます。ただし、利用側は規範により `assert` を使わないため、この持ち込みで利用側の動作は変わりません。

**Related**

- [Q-003](#q-003-マーカー未定義や値不一致時のコンパイル失敗方式)
- [Q-009](#q-009-jsonc-の実装の既存の静的アサートの表記を直すか)

### Q-008 struct-meta へ追加する記述の範囲

**Status**

解決済み

**Question**

`app/struct-meta` には更新する既存の記述がないため、検査用のヘッダーの取り込みに合わせて、どのコメントや文書を追加しますか。

**Context**

- **経緯と背景**: [Q-006](#q-006-関連ドキュメントの更新範囲) では選択肢 C が選択され、補足で「`app/struct-meta` 側の関連コメント・文書も、検査で担保される旨へ更新する」と示されました。[Q-002](#q-002-ビルド時検査を配置する範囲) の回答により、`app/struct-meta` でも cJSON の公開ヘッダーを取り込む箇所で検査を受け取ります。`app/struct-meta` で cJSON の公開ヘッダーを直接取り込むのは、公開ヘッダー `prod/include/struct_meta/json/json.h` と、`json.h` をあわせて取り込む `json/decode.c`、`json/encode.c`、`jsonEncodeTest.cc` です。検査用のヘッダーを `json.h` で取り込めば、`json/file.c` を含む `json.h` の利用側のすべてが検査を受け取ります。
- **調査の結果**: `app/struct-meta` の `prod/`、`docs/`、`README.md` には、パッチ `0003`、スレッド ローカル、並行実行、`global_error` に触れた記述が存在しません。
    - `prod/include/struct_meta/json/file.h` の `struct_meta_json_file_load()` には Doxygen コメントがありません。`app/porter` の `porter_spec.h` は、porter の解析と並行に実行できるほかの解析として `struct_meta_json_file_load()` を挙げていますが、`app/struct-meta` 自身はスレッド安全性を記載していません。
    - `docs/architecture.md` の cJSON への言及は、「ビルド」の節の実行時ライブラリの配置と、「JSON で扱える整数の範囲」の節の整数拡張 API の利用だけです。
    - 規範の「実装コメント」の「記載量」は、定義から自明な追加に理由を書き加えないとしています。検査用のヘッダーの名前と、そのヘッダーの Doxygen コメントで目的が分かる場合は、取り込みの行のコメントは不要です。
- **各案の具体例**: 案 A では、`docs/architecture.md` の「ビルド」の節に、次のような段落を追加します。取り込みの行にはコメントを付けません。案 B では、`json.h` の取り込みの行に取り込む理由のコメントだけを付け、`docs/architecture.md` は変更しません。案 C では、検査用のヘッダーの取り込みだけを行い、`app/struct-meta` のコメントと文書は追加しません。

```markdown
JSON の変換は、`app/cjson` が独自のパッチを当てた cJSON を前提とします。
`struct_meta/json/json.h` は、cJSON のパッチの適用を検査するヘッダーを取り込み、パッチのない `cJSON.h` を取り込んだ場合はコンパイルを失敗させます。
```

**Answer**

選択方式: 単一選択

- [x] A: (推奨) `docs/architecture.md` の「ビルド」の節に、パッチを当てた cJSON を前提とし、`json.h` でビルド時に検査する旨の段落を追加する。取り込みの行にはコメントを付けない。
- [ ] B: `json.h` の検査用のヘッダーの取り込みの行に、取り込む理由のコメントを付ける。`docs/architecture.md` は変更しない。
- [ ] C: 検査用のヘッダーの取り込みだけを行い、`app/struct-meta` のコメントと文書は追加しない。
- [ ] D: その他 (`struct_meta_json_file_load()` へのスレッド安全性の記載の追加など)
- [ ] X: 意図不明 (詳細な質問内容に再生成すること)

その他の回答 / 補足:

回答者: Tetsuo Honda  
回答日時: 2026-11-28 13:02:51 JST (対話セッションでの回答「Q-008: A」を、エージェントが記入)

**Assessment**

選択された回答を確認しました。推奨案と一致し、規範の「実装コメント」の記載量の規則および [Q-006](#q-006-関連ドキュメントの更新範囲) の補足と整合します。`app/struct-meta/docs/architecture.md` の「ビルド」の節にパッチを当てた cJSON を前提とする旨と `struct_meta/json/json.h` でビルド時に検査する旨の段落を追加し、`json.h` の取り込み行にはコメントを付けません。[Q-006](#q-006-関連ドキュメントの更新範囲) もあわせて解決済みとします。

**Recommendation**

推奨: A  
優先した基準: ユーザーの補足 (struct-meta 側の文書へも検査で担保される旨を記載する) の充足と、規範の「実装コメント」の記載量の規則との両立を優先しました。  
理由: `docs/architecture.md` は struct-meta の設計の正本であり、cJSON の前提とビルド時の検査を 1 か所で説明できます。取り込みの行のコメントは、検査用のヘッダーの名前と Doxygen コメントから目的が分かるため、規範の記載量の規則では不要です。`struct_meta_json_file_load()` へスレッド安全性を記載すると、struct-meta の公開 API に新しい契約を追加することになるため、推奨案には含めていません。

**Related**

- [Q-002](#q-002-ビルド時検査を配置する範囲)
- [Q-006](#q-006-関連ドキュメントの更新範囲)

### Q-009 JSONC の実装の既存の静的アサートの表記を直すか

**Status**

解決済み

**Question**

`app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c` の既存の `_Static_assert` 3 か所を、今回の案件で `static_assert` (`<assert.h>`) へ直しますか。

**Context**

- **経緯と背景**: [Q-007](#q-007-マーカーの値を検証する静的アサートの表記) の調査で、規範の「R2 制約を静的に検査する」が「既存の `_Static_assert` は、`static_assert` へ直します」と定めていることを確認しました。`app/` 配下の製品コード (外部由来の `app/lua`、`app/sqlite` と、生成器が出力するコードの文字列を除く) で `_Static_assert` が残っているのは、`cJSON_JSONC.c` の 47〜50 行目の 3 か所 (JSONC の確保ヘッダーの整列と大きさの検査) だけです。
- **影響**: 今回の案件は `cJSON_JSONC.c` を変更しない見込みです。検査は `cJSON_JSONC.h` が取り込む検査用のヘッダーに置くためです。この 3 か所を直す場合は、`cJSON_JSONC.c` に `#include <assert.h>` を追加し、3 か所の `_Static_assert` を `static_assert` へ置き換えます。動作と生成されるコードは変わりません。
- **各案の具体例**:

```c
/* 案 A (推奨): 今回の案件で直す */
#include <assert.h>
/* ... */
static_assert(_Alignof(jsonc_block_header) >= _Alignof(max_align_t), "JSONC の確保ヘッダーの整列が不足しています");
```

```c
/* 案 B: 今回は直さず、残課題一覧へ記録する (現状のまま) */
_Static_assert(_Alignof(jsonc_block_header) >= _Alignof(max_align_t), "JSONC の確保ヘッダーの整列が不足しています");
```

**Answer**

選択方式: 単一選択

- [x] A: (推奨) 今回の案件で、`cJSON_JSONC.c` の `_Static_assert` 3 か所を `static_assert` へ直し、`<assert.h>` を取り込む。
- [ ] B: 今回の案件では直さず、残課題一覧 (`261128-cjson-patch-marker-backlog.md`) へ記録する。
- [ ] C: その他
- [ ] X: 意図不明 (詳細な質問内容に再生成すること)

その他の回答 / 補足:

回答者: Tetsuo Honda  
回答日時: 2026-11-28 13:02:51 JST (対話セッションでの回答「Q-009: A」を、エージェントが記入)

**Assessment**

選択された回答を確認しました。推奨案と一致し、[Q-007](#q-007-マーカーの値を検証する静的アサートの表記) の決定および規範の「R2 制約を静的に検査する」と整合します。`app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c` に `#include <assert.h>` を追加し、47〜50 行目の `_Static_assert` 3 か所を `static_assert` へ置き換えます。

**Recommendation**

推奨: A  
優先した基準: 漏れのない最終形 (同じ app の同じ検査の表記を規範にそろえること) を、変更量の抑制より優先しました。  
理由: 3 か所の置き換えと 1 行の取り込みで、`app/` の製品コードから規範に反する `_Static_assert` がなくなります。変更は `app/cjson` の中にとどまり、`app/cjson` の局所ビルドとテストで確認できます。[Q-007](#q-007-マーカーの値を検証する静的アサートの表記) で案 B (表記の使い分け) を選択する場合は、この質問の前提が変わるため、選択肢 C で扱いを指定してください。

**Related**

- [Q-007](#q-007-マーカーの値を検証する静的アサートの表記)
