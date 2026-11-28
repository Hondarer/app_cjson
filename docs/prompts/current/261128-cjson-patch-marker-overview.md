<!--
生成元: agree-before-implement スキル
文書種別: 全体管理文書 (overview)
最終更新日時: 2026-11-28 18:08:00 JST
最終更新エージェント: agy / セッション ID: f1717ab7-aaa3-487b-8275-84c421514770
稼働 OS: Linux (WSL)
上流文書: `261126-cjson-patch-check-backlog.md` (対策元の残課題一覧、BL-001)。削除されている場合があります。
下流文書: `261128-cjson-patch-marker-discussion.md` (検討用 Markdown)、`261128-cjson-patch-marker-lightadr.md` (簡易 ADR)、`261128-cjson-patch-marker-design.md` (最終仕様)、`261128-cjson-patch-marker-backlog.md` (残課題一覧)。未作成の場合や、削除されている場合があります。
再開手順: このファイルを開いたエージェントは、作業の前に agree-before-implement スキルを読み込んでください。この文書と、ユーザーが編集した最新の検討用 Markdown を読み直し、作業 WBS の `← **CURRENT**` の項目から再開します。最終更新エージェントのセッション情報や稼働 OS・最終更新日時が参照可能であれば再開時のコンテキストや環境差分の把握に利用し、別エージェントや別環境・別 OS からの引き継ぎ、担当変更によりセッションが見つからない場合も異常とせず、文書の記載から継続してください。`← **CURRENT**` の項目は処理が中止された項目である場合があり、どこまで進んでいたかは文書だけでは分かりません。対象成果物の内容を確認して実施済みの範囲を特定してから、作業を再開してください。`← **CURRENT**` がなく、作業 WBS がすべて完了 (✅) であれば、この案件の合意フローは完了しています。
-->

# cJSON パッチ 0003 の適用をビルド時に検査する仕組み 全体管理

ファイル名: `261128-cjson-patch-marker-overview.md`

この文書は、案件の識別、作業 WBS、対象範囲、確認状況、経緯を管理します。質問と回答の本文は検討用 Markdown (`261128-cjson-patch-marker-discussion.md`) に、決定事項は簡易 ADR (`261128-cjson-patch-marker-lightadr.md`) に記載し、この文書には重複して記載しません。

## 案件の識別

検討 ID: `261128-cjson-patch-marker`

検討用 Markdown の版: 2

状態: 成果物承認待ち (第 3 ゲート)

## 作業 WBS と進捗管理

この案件の作業 WBS です。対策元の残課題一覧 (`261126-cjson-patch-check-backlog.md`) の BL-001 を解消するため、情報収集と質問の実施、残懸念の解消 (第 1 ゲート)、最終仕様 (`261128-cjson-patch-marker-design.md`) の作成と承認 (第 2 ゲート)、対象成果物の更新と検証、成果物の承認 (第 3 ゲート) までを管理します。  
具体的な作業対象は、Q&A の決定 (版 1 の回答) に基づいて見直しました。版 2 の回答と最終仕様に基づいて、さらに具体化します。小項目 (末端項目) 1 件ごとに着手直前と完了直後にこの文書を更新し、対策元の `261126-cjson-patch-check-backlog.md` の状態と経緯も進行に合わせて随時更新します。

凡例: ⏸️ 未着手、🔄 実施中、🚫 ブロック中、⚠️ 要注意、✅ 完了。現在取り組んでいる項目の末尾に `← **CURRENT**` を付けます。

- ✅ 1. 検討用 Markdown のための情報収集と質問の実施
    - ✅ 1.1. 前提条件と要求事項の調査 (`app/cjson/patches/` の構成と適用機構、`cJSON.h` の取り込み経路、`framework/makefw/makefiles/_flags.mk` の C/C++ 規格設定の調査、および対策元 `261126-cjson-patch-check-backlog.md` の BL-001 の状態を「検討中」へ更新)
    - ✅ 1.2. 課題・論点の抽出と明確化のための確認項目 ([Q-001](261128-cjson-patch-marker-discussion.md#q-001-マーカー-マクロの定義場所と名称) 〜 [Q-006](261128-cjson-patch-marker-discussion.md#q-006-関連ドキュメントの更新範囲)) の作成、および初期簡易 ADR・残課題一覧の作成
    - ✅ 1.3. Q&A ターンごとのユーザー回答の受付と記録 (JST 日時、Git ユーザー名または発言者)
    - ✅ 1.4. 回答に基づく前提・制約の検証、簡易 ADR (`261128-cjson-patch-marker-lightadr.md`) への決定事項 (LADR-002 〜 LADR-007) の反映、版 2 の確認項目 ([Q-007](261128-cjson-patch-marker-discussion.md#q-007-マーカーの値を検証する静的アサートの表記) 〜 [Q-009](261128-cjson-patch-marker-discussion.md#q-009-jsonc-の実装の既存の静的アサートの表記を直すか)) の作成、残課題一覧への BL-001 の追記、WBS の見直し、対策元 `261126-cjson-patch-check-backlog.md` への合意方針の反映
    - ✅ 1.5. 版 2 のユーザー回答の受付と記録 (JST 日時、Git ユーザー名または発言者)
    - ✅ 1.6. 版 2 の回答の検証、簡易 ADR (LADR-004、LADR-007、LADR-008) への反映、WBS の見直し (対策元 `261126-cjson-patch-check-backlog.md` への反映を含む)
- ✅ 2. 前提合意と残懸念の解消 (第 1 ゲート: 最終仕様作成への着手)
    - ✅ 2.1. 全論点 ([Q-001](261128-cjson-patch-marker-discussion.md#q-001-マーカー-マクロの定義場所と名称) 〜 [Q-009](261128-cjson-patch-marker-discussion.md#q-009-jsonc-の実装の既存の静的アサートの表記を直すか)) に対する未解決事項および質問の解消確認
    - ✅ 2.2. 方針・前提条件の合意確定 (簡易 ADR の決定事項と、根拠となった解決済み Q の回答またはユーザーの明示的な指示との一致を確認し、懸念がないことをもってフェーズ 2 へ着手)
- ✅ 3. 最終決定事項 (最終仕様) の作成と承認の受付
    - ✅ 3.1. `261128-cjson-patch-marker-design.md` の作成 (パッチの差分案、検査用のヘッダーの内容、利用側の取り込みの一覧、テストの具体、更新する文書の一覧、検証手順を含む。対策元 `261126-cjson-patch-check-backlog.md` へ確定方針と状態を反映)
    - ✅ 3.2. 最終仕様における受入条件と整合性の確認 (`text_style_jp.py` による確認を含む)
    - ✅ 3.3. 対話セッションにおける最終仕様 (`261128-cjson-patch-marker-design.md`) の承認の受付 (第 2 ゲート: 作業着手の合意)
- ✅ 4. 承認された最終仕様に従った作業の実施
    - ✅ 4.1. パッチ `app/cjson/patches/0003-thread-local-global-error.patch` への `prod/include/cJSON.h` の差分 (マーカーの定義) の追加
    - ✅ 4.2. `app/cjson/prod/include/cJSON_PatchCheck.h` の新規作成 (`<assert.h>` と `<cJSON.h>` を取り込み、`#ifndef` で `#error`、`#else` で `static_assert` により値 `1` を検証)
    - ✅ 4.3. `app/cjson/prod/include/cJSON_JSONC.h` での `cJSON_PatchCheck.h` の取り込みと、パッチ `0003` の依存に関する Doxygen コメントの更新
    - ✅ 4.4. 利用側での `cJSON_PatchCheck.h` の取り込み
        - ✅ 4.4.1. `app/porter` (`prod/libsrc/porter/api/potr_config_list_service_ids.c`、`potr_config_validate_file.c`、`potr_service_get_type.c`、`potr_service_open_from_config.c`、`prod/libsrc/porter/protocol/config_decode.c`)
        - ✅ 4.4.2. `app/struct-meta` (`prod/include/struct_meta/json/json.h`。取り込みの行にはコメントを付けない)
        - ✅ 4.4.3. `app/cjson` 内の利用側 (`prod/src/cmd/cjson_sample/cjson_sample.c`、`test/src/cmd/cjsonTest/cjsonTest.cc`、`test/src/cmd/cjsonIntegerTest/cjsonIntegerTest.cc`、`test/src/libcjsonTest/mockCjsonTest/caller_info.c`、`caller_info_cpp.cc`)
    - ✅ 4.5. `app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c` の既存の `_Static_assert` 3 か所を `static_assert` (`<assert.h>`) へ修正
    - ✅ 4.6. テストの追加と更新
        - ✅ 4.6.1. `app/cjson/bin_test/test_extract_package.py` へのマーカーの出力の検証と、`cJSON_PatchCheck.h` が書き換えられないことの検証の追加
        - ✅ 4.6.2. `app/cjson/bin_test/test_patch_marker_check.py` (コンパイルの失敗を検証する Linux 向けテスト) の新規作成
    - ✅ 4.7. 関連ドキュメントへの永続的知見の反映
        - ✅ 4.7.1. `app/cjson` (`patches/README.md`、`README.md`)
        - ✅ 4.7.2. `app/porter` (`docs/api.md`、`prod/include/porter/porter_spec.h` の 3 か所)
        - ✅ 4.7.3. `app/struct-meta` (`docs/architecture.md` の「ビルド」の節に段落を追加)
    - ✅ 4.8. 局所ビルド、`.warn` ファイル確認、局所テスト (`app/cjson`、`app/cplat`、`app/porter`、`app/struct-meta`。make は同時に 1 本だけ実行する)、`app/cjson/bin_test` の実行、変更した Markdown への `text_style_jp.py` の実行
    - ✅ 4.9. 受入条件への適合確認および対策元 `261126-cjson-patch-check-backlog.md` の BL-001 の状態・対処結果の更新
    - ✅ 4.10. 簡易 ADR (`261128-cjson-patch-marker-lightadr.md`) の最終確認
    - ✅ 4.11. 残課題一覧 (`261128-cjson-patch-marker-backlog.md`) の再構築と整理
- 🔄 5. 成果物の承認の受付 (第 3 ゲート)
    - ✅ 5.1. 成果物・検証結果・残課題の提示
    - 🔄 5.2. 対話セッションにおける成果物の承認の受付 (「承認」または「完了」の受領によるスキル完了) ← **CURRENT**

## 対象と参照資料

対象成果物 (版 2 までの合意で確定):

- `app/cjson/patches/0003-thread-local-global-error.patch` (`prod/include/cJSON.h` への差分の追加)
- `app/cjson/prod/include/cJSON_PatchCheck.h` (新規)
- `app/cjson/prod/include/cJSON_JSONC.h` (`cJSON_PatchCheck.h` の取り込みと Doxygen コメント更新)
- `app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c` (`<assert.h>` 追加と `_Static_assert` 3 か所の `static_assert` 化)
- `app/porter/prod/libsrc/porter/api/potr_config_list_service_ids.c`、`potr_config_validate_file.c`、`potr_service_get_type.c`、`potr_service_open_from_config.c`、`app/porter/prod/libsrc/porter/protocol/config_decode.c` (`cJSON_PatchCheck.h` の取り込み)
- `app/struct-meta/prod/include/struct_meta/json/json.h` (`cJSON_PatchCheck.h` の取り込み)
- `app/cjson` 内の利用側 (`prod/src/cmd/cjson_sample/cjson_sample.c`、`test/src/cmd/cjsonTest/cjsonTest.cc`、`test/src/cmd/cjsonIntegerTest/cjsonIntegerTest.cc`、`test/src/libcjsonTest/mockCjsonTest/caller_info.c`、`caller_info_cpp.cc`)
- `app/cjson/bin_test/test_extract_package.py` と `app/cjson/bin_test/test_patch_marker_check.py` (新規)
- `app/cjson/patches/README.md`、`app/cjson/README.md`、`app/porter/docs/api.md`、`app/porter/prod/include/porter/porter_spec.h`、`app/struct-meta/docs/architecture.md`

今回の対象範囲: `261126-cjson-patch-check-backlog.md` の BL-001 に対応する、cJSON パッチ `0003-thread-local-global-error.patch` の適用をビルド時に検査する仕組みの設計と実装。動的リンクの取り違えへの実行時照合は対象外です ([LADR-005](261128-cjson-patch-marker-lightadr.md#ladr-005-動的リンクの取り違えの実行時照合は対象外とする))。`app/cjson/packages/` から展開される cJSON 本体の生成物 (`prod/include/cJSON.h`、`prod/libsrc/cjson/cJSON.c` など) への直接編集は対象外です。

書き込み可能な検討文書: この文書、`261128-cjson-patch-marker-discussion.md`、`261128-cjson-patch-marker-lightadr.md`、`261128-cjson-patch-marker-backlog.md`、および対策元の残課題一覧 `261126-cjson-patch-check-backlog.md` (継続作業の状況更新のみ)

参照資料と版:

- `AGENTS.md` (リポジトリ ルート、2026-11-28 時点のワーク ツリー)
- `app/cjson/AGENTS.md` (2026-11-28 時点のワーク ツリー)
- `app/cjson/docs/prompts/current/261126-cjson-patch-check-backlog.md` (対策元の残課題一覧 BL-001、2026-11-26 作成)
- `app/cjson/patches/README.md`、`0001-windows-dll-import-default.patch`、`0002-exact-integer-hooks.patch`、`0003-thread-local-global-error.patch`
- `app/cjson/makepart.mk`、`app/cjson/appdeps.mk`、`app/cjson/bin_internal/extract_package.py`
- `framework/makefw/bin_internal/apply_patches.py`、`framework/makefw/bin_internal/generate_symbol_namespace.py`
- `framework/makefw/makefiles/_flags.mk`、`framework/makefw/makefiles/prepare.mk`、`framework/makefw/makefiles/makelibsrc_c_cpp.mk`、`framework/makefw/makefiles/makesrc_c_cpp.mk`
- `app/cjson/prod/include/cJSON.h` (展開済み `cJSON-1.7.19.zip`)、`app/cjson/prod/include/cJSON_JSONC.h`、`app/cjson/prod/include_internal/cjson_app_thread_local.h`、`app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c`
- `app/cjson/test/include_override/cJSON.h`、`app/cjson/test/include_override/cJSON_JSONC.h`、`app/cjson/test/src/libcjsonTest/exportTest/exportTest.cc`、`app/cjson/bin_test/test_extract_package.py`、`app/cjson/bin_test/test_namespace_coexistence.py`
- 利用側: `app/cplat/prod/libsrc/cplat/runtime/sym_loader_init.c`、`app/porter/prod/libsrc/porter/protocol/config_document.c`、`app/porter/prod/libsrc/porter/protocol/config_decode.c`、`app/porter/prod/libsrc/porter/api/` の 4 ファイル、`app/struct-meta/prod/include/struct_meta/json/json.h`、`app/struct-meta/prod/libsrc/struct_meta/json/file.c` (cJSON の公開ヘッダーを取り込む全箇所の一覧は [Q-002](261128-cjson-patch-marker-discussion.md#q-002-ビルド時検査を配置する範囲) の Assessment)
- 規範: `app/general/docs/coding-guideline.md` (「R2 制約を静的に検査する」「assert を使用しない」「実装コメント」)、`app/general/bin_internal/run-bin-tests.py`
- 関連文書: `app/porter/docs/api.md`、`app/porter/prod/include/porter/porter_spec.h`、`app/struct-meta/docs/architecture.md`

## 観点別の確認

| 観点 | 状態 | 根拠・関連 Q |
| --- | --- | --- |
| 目的・利用者 | 確認済み | 対策元 `261126-cjson-patch-check-backlog.md` (BL-001)、[LADR-001](261128-cjson-patch-marker-lightadr.md#ladr-001-cjson-パッチ-0003-のビルド時検査案件の前提と範囲) |
| 範囲・境界 | 確認済み | [Q-002](261128-cjson-patch-marker-discussion.md#q-002-ビルド時検査を配置する範囲)、[Q-004](261128-cjson-patch-marker-discussion.md#q-004-動的リンク取り違えへの実行時照合を今回の範囲に含めるか)、[Q-006](261128-cjson-patch-marker-discussion.md#q-006-関連ドキュメントの更新範囲)、[Q-008](261128-cjson-patch-marker-discussion.md#q-008-struct-meta-へ追加する記述の範囲)、[Q-009](261128-cjson-patch-marker-discussion.md#q-009-jsonc-の実装の既存の静的アサートの表記を直すか) |
| 入力・根拠 | 確認済み | パッチ一式、`extract_package.py`、`apply_patches.py`、`generate_symbol_namespace.py`、`_flags.mk`、`prepare.mk`、cJSON の公開ヘッダーの全取り込み箇所 (ワークスペース全体の grep) を実測確認済み |
| 整合性・依存関係 | 確認済み | [Q-001](261128-cjson-patch-marker-discussion.md#q-001-マーカー-マクロの定義場所と名称)、[Q-003](261128-cjson-patch-marker-discussion.md#q-003-マーカー未定義や値不一致時のコンパイル失敗方式)、[Q-007](261128-cjson-patch-marker-discussion.md#q-007-マーカーの値を検証する静的アサートの表記)、[Q-009](261128-cjson-patch-marker-discussion.md#q-009-jsonc-の実装の既存の静的アサートの表記を直すか) により、コーディング規範 R2 および既存実装との整合を確認済み |
| 実現性・例外 | 確認済み | [Q-001](261128-cjson-patch-marker-discussion.md#q-001-マーカー-マクロの定義場所と名称)、[Q-003](261128-cjson-patch-marker-discussion.md#q-003-マーカー未定義や値不一致時のコンパイル失敗方式)、[Q-007](261128-cjson-patch-marker-discussion.md#q-007-マーカーの値を検証する静的アサートの表記)。一時ディレクトリで GCC の `-std=c17` と `-std=c++17` (`-Wall -Wextra -Wpedantic -Werror`) による正常系・未定義・値不一致の挙動を実測確認済み |
| 受入条件・検証 | 確認済み | [Q-005](261128-cjson-patch-marker-discussion.md#q-005-コンパイル失敗の確認の自動化方針)、[LADR-006](261128-cjson-patch-marker-lightadr.md#ladr-006-マーカーの出力とコンパイルの失敗を-bin_test-で自動検証する) |

## 質問一覧

各 Q の本文は検討用 Markdown にあります。Q-ID には検討用 Markdown の該当見出しへのリンクを設けます。

| Q-ID | 論点 | 状態 | 依存・関連 | 反映先の決定 |
| --- | --- | --- | --- | --- |
| [Q-001](261128-cjson-patch-marker-discussion.md#q-001-マーカー-マクロの定義場所と名称) | マーカー マクロの定義場所と名称 | 解決済み | [Q-002](261128-cjson-patch-marker-discussion.md#q-002-ビルド時検査を配置する範囲)、[Q-006](261128-cjson-patch-marker-discussion.md#q-006-関連ドキュメントの更新範囲) | [LADR-002](261128-cjson-patch-marker-lightadr.md#ladr-002-パッチ-0003-の適用マーカーを-cjsonh-に定義する) |
| [Q-002](261128-cjson-patch-marker-discussion.md#q-002-ビルド時検査を配置する範囲) | ビルド時検査を配置する範囲 | 解決済み | [Q-001](261128-cjson-patch-marker-discussion.md#q-001-マーカー-マクロの定義場所と名称)、[Q-003](261128-cjson-patch-marker-discussion.md#q-003-マーカー未定義や値不一致時のコンパイル失敗方式)、[Q-008](261128-cjson-patch-marker-discussion.md#q-008-struct-meta-へ追加する記述の範囲) | [LADR-003](261128-cjson-patch-marker-lightadr.md#ladr-003-検査を-1-か所の検査用ヘッダーに定義し利用側が取り込む) |
| [Q-003](261128-cjson-patch-marker-discussion.md#q-003-マーカー未定義や値不一致時のコンパイル失敗方式) | マーカー未定義や値不一致時のコンパイル失敗方式 | 解決済み | [Q-002](261128-cjson-patch-marker-discussion.md#q-002-ビルド時検査を配置する範囲)、[Q-007](261128-cjson-patch-marker-discussion.md#q-007-マーカーの値を検証する静的アサートの表記) | [LADR-004](261128-cjson-patch-marker-lightadr.md#ladr-004-マーカーの未定義と値の不一致でコンパイルを失敗させる) |
| [Q-004](261128-cjson-patch-marker-discussion.md#q-004-動的リンク取り違えへの実行時照合を今回の範囲に含めるか) | 動的リンク取り違えへの実行時照合を今回の範囲に含めるか | 解決済み | なし | [LADR-005](261128-cjson-patch-marker-lightadr.md#ladr-005-動的リンクの取り違えの実行時照合は対象外とする) |
| [Q-005](261128-cjson-patch-marker-discussion.md#q-005-コンパイル失敗の確認の自動化方針) | コンパイル失敗の確認の自動化方針 | 解決済み | [Q-001](261128-cjson-patch-marker-discussion.md#q-001-マーカー-マクロの定義場所と名称)、[Q-002](261128-cjson-patch-marker-discussion.md#q-002-ビルド時検査を配置する範囲) | [LADR-006](261128-cjson-patch-marker-lightadr.md#ladr-006-マーカーの出力とコンパイルの失敗を-bin_test-で自動検証する) |
| [Q-006](261128-cjson-patch-marker-discussion.md#q-006-関連ドキュメントの更新範囲) | 関連ドキュメントの更新範囲 | 解決済み | [Q-001](261128-cjson-patch-marker-discussion.md#q-001-マーカー-マクロの定義場所と名称)、[Q-002](261128-cjson-patch-marker-discussion.md#q-002-ビルド時検査を配置する範囲)、[Q-008](261128-cjson-patch-marker-discussion.md#q-008-struct-meta-へ追加する記述の範囲) | [LADR-007](261128-cjson-patch-marker-lightadr.md#ladr-007-検査の導入に合わせて-cjsonporterstruct-meta-の文書を更新する) |
| [Q-007](261128-cjson-patch-marker-discussion.md#q-007-マーカーの値を検証する静的アサートの表記) | マーカーの値を検証する静的アサートの表記 | 解決済み | [Q-003](261128-cjson-patch-marker-discussion.md#q-003-マーカー未定義や値不一致時のコンパイル失敗方式)、[Q-009](261128-cjson-patch-marker-discussion.md#q-009-jsonc-の実装の既存の静的アサートの表記を直すか) | [LADR-004](261128-cjson-patch-marker-lightadr.md#ladr-004-マーカーの未定義と値の不一致でコンパイルを失敗させる) |
| [Q-008](261128-cjson-patch-marker-discussion.md#q-008-struct-meta-へ追加する記述の範囲) | struct-meta へ追加する記述の範囲 | 解決済み | [Q-002](261128-cjson-patch-marker-discussion.md#q-002-ビルド時検査を配置する範囲)、[Q-006](261128-cjson-patch-marker-discussion.md#q-006-関連ドキュメントの更新範囲) | [LADR-007](261128-cjson-patch-marker-lightadr.md#ladr-007-検査の導入に合わせて-cjsonporterstruct-meta-の文書を更新する) |
| [Q-009](261128-cjson-patch-marker-discussion.md#q-009-jsonc-の実装の既存の静的アサートの表記を直すか) | JSONC の実装の既存の静的アサートの表記を直すか | 解決済み | [Q-007](261128-cjson-patch-marker-discussion.md#q-007-マーカーの値を検証する静的アサートの表記) | [LADR-008](261128-cjson-patch-marker-lightadr.md#ladr-008-cjson_jsoncc-の既存の-_static_assert-を-static_assert-へ統一する) |

## 対象成果物と合意版

最終決定事項 (最終仕様) の設計書: `261128-cjson-patch-marker-design.md`

検討用 Markdown の合意版 (第 1 ゲート): 版 2 (2026-11-28 13:07 確定、Q-001 〜 Q-009 解決済み・残懸念なし)

最終仕様 (`design.md`) の合意版 (第 2 ゲート): 2026-11-28 13:18 JST、対話セッションで Tetsuo Honda が承認 (第 2 ゲート通過)

更新予定の成果物:

- パッチ: `app/cjson/patches/0003-thread-local-global-error.patch`
- 検査用公開ヘッダー (新規): `app/cjson/prod/include/cJSON_PatchCheck.h`
- 検査取り込み・表記統一: `app/cjson/prod/include/cJSON_JSONC.h`、`app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c`、`app/cjson` 内の利用側 5 ファイル、`app/porter` の 5 ファイル、`app/struct-meta/prod/include/struct_meta/json/json.h`
- 自動テスト: `app/cjson/bin_test/test_extract_package.py`、`app/cjson/bin_test/test_patch_marker_check.py` (新規)
- 文書・Doxygen コメント: `app/cjson/patches/README.md`、`app/cjson/README.md`、`app/cjson/prod/include/cJSON_JSONC.h`、`app/porter/docs/api.md`、`app/porter/prod/include/porter/porter_spec.h`、`app/struct-meta/docs/architecture.md`

受入条件:

1. `0003-thread-local-global-error.patch` が `apply_patches.py` で厳密適用され、展開・名前空間変換後の `prod/include/cJSON.h` に `#define CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR 1` が出力される。
2. `cJSON_PatchCheck.h` が C17 と C++17 の双方で、マーカー `1` では警告なしにコンパイルでき、未定義では `#error`、値不一致では `static_assert` でコンパイルが失敗する。
3. 対象の利用側すべてが直接または間接に `cJSON_PatchCheck.h` を取り込み、`cJSON_JSONC.c` の既存の `_Static_assert` 3 か所が `static_assert` へ置き換わっている。
4. `app/cjson/bin_test` (`test_extract_package.py` および新規 `test_patch_marker_check.py`) がすべて成功する。
5. `app/cjson`、`app/cplat`、`app/porter`、`app/struct-meta` の局所ビルドと単体テストが成功し、変更起因の `.warn` がない。
6. 更新対象の文書および変更した Markdown が `text_style_jp.py` の確認を通過している。

## 経緯

案件の進行上の出来事を、発生順に末尾へ追記します。個々の決定の内容は簡易 ADR に記載し、ここには出来事と参照先だけを記載します。

| 日時 (JST) | 出来事 | 内容・参照先 |
| --- | --- | --- |
| 2026-11-28 08:35 | 案件の開始 | 対策元 `261126-cjson-patch-check-backlog.md` (BL-001) の継続作業として、全体管理文書、検討用 Markdown (版 1、Q-001 〜 Q-006)、初期簡易 ADR (LADR-001)、残課題一覧を作成。対策元 backlog の BL-001 を「検討中」へ更新 |
| 2026-11-28 12:51 | 作業の再開と版 1 の回答の記録 | 別のエージェント (claude code) が WBS 1.3 から再開。Q-001 〜 Q-006 の対話セッションでの回答を検討用 Markdown へ記録し、LADR-002 〜 LADR-007 を追加 |
| 2026-11-28 12:51 | 版 2 の確認項目の追加と WBS の見直し | Q-003 (規範の R2 との不一致) と Q-006 (struct-meta に既存の記述がない) を未解決とし、[Q-007](261128-cjson-patch-marker-discussion.md#q-007-マーカーの値を検証する静的アサートの表記) 〜 [Q-009](261128-cjson-patch-marker-discussion.md#q-009-jsonc-の実装の既存の静的アサートの表記を直すか) を追加。残課題一覧に BL-001 を追記。作業 WBS の 4 を具体的な対象ファイルへ詳細化。版 2 の回答の受付で停止 |
| 2026-11-28 13:02 | 作業の再開と版 2 の回答の記録・検証 | 別のエージェント (agy) が WBS 1.5 から再開。Q-007 〜 Q-009 の対話セッションでの回答を記録・検証し、LADR-004・LADR-007 を確定、LADR-008 を追加。WBS 4 と対策元 backlog を更新 |
| 2026-11-28 13:07 | 第 1 ゲート通過 | 全論点 (Q-001 〜 Q-009) と 6 観点の解消および簡易 ADR (LADR-001 〜 LADR-008) との一致を確認。残懸念なしとしてフェーズ 2 (`261128-cjson-patch-marker-design.md` の作成) へ着手 |
| 2026-11-28 13:10 | 最終仕様の作成と整合確認完了 | `261128-cjson-patch-marker-design.md` を作成し、受入条件・整合性・`text_style_jp.py` による機械整形を確認。対策元 `261126-cjson-patch-check-backlog.md` の BL-001 を「対応中」へ更新。第 2 ゲート (承認の受付) で停止 |
| 2026-11-28 13:18 | 第 2 ゲート通過・フェーズ 4 第 1 段着手 | 2026-11-28、対話セッションで Tetsuo Honda が最終仕様 (`261128-cjson-patch-marker-design.md`) を承認 (第 2 ゲート通過)。並行ビルドへの影響を避けるため、フェーズ 4 を第 1 段 (新規ヘッダー、`bin_test`、文書、`porter_spec.h`) と第 2 段 (パッチ、既存 C/C++、ビルド・テスト実行) に分けて第 1 段を開始 |
| 2026-11-28 13:22 | フェーズ 4 第 1 段完了・第 2 段待ち | WBS 4.2、4.6.1、4.6.2、4.7.1、4.7.2、4.7.3 を完了。別作業が `app/cplat` 等を並行ビルド中のため、ビルドへ影響する WBS 4.1、4.3、4.4、4.5、4.8 以降は第 2 段待ちとして停止 |
| 2026-11-28 18:08 | フェーズ 4 第 2 段完了・第 3 ゲート待ち | WBS 4.1、4.3、4.4.x、4.5、4.8 〜 4.11 およびレビュー指摘 (`cJSON_PatchCheck.h` の `#error` / `static_assert` メッセージの日本語化と `test_patch_marker_check.py` の追従) を完了。4 app のビルド・テスト・`.warn` 確認・`app/cjson/bin_test`・scratchpad でのパッチ未適用検証をすべて通過し、第 3 ゲート (成果物の承認の受付) で停止 |

## 反映と検証の記録

### フェーズ 4 第 1 段 (完了)

- **WBS 4.2**: `app/cjson/prod/include/cJSON_PatchCheck.h` を新規作成。`app/cjson/prod/include/.gitignore` (対象は `/cJSON.h` と `/cJSON_Utils.h` のみ) および `app/cjson/.gitignore` (`!*.*`) を確認し、Git 無視対象外であることを `git check-ignore` で確認済み。
- **WBS 4.6.1 / 4.6.2**: `app/cjson/bin_test/test_extract_package.py` を更新し、`app/cjson/bin_test/test_patch_marker_check.py` を新規作成。
- **WBS 4.7.1 / 4.7.2 / 4.7.3**: `app/cjson/patches/README.md`、`app/cjson/README.md`、`app/porter/docs/api.md`、`app/porter/prod/include/porter/porter_spec.h` (3 か所)、`app/struct-meta/docs/architecture.md` を更新。

### フェーズ 4 第 2 段 (完了)

- **WBS 4.1**: `app/cjson/patches/0003-thread-local-global-error.patch` の先頭に `prod/include/cJSON.h` への差分 (`CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR` 値 `1`) を追加。`make -C app/cjson` 実行時の再展開で `app/cjson/prod/include/cJSON.h` の 93 行目に `#define CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR 1` が出力されることを確認。
- **レビュー指摘反映**: `app/cjson/prod/include/cJSON_PatchCheck.h` の `#error` と `static_assert` のメッセージを `cJSON_JSONC.c` に合わせて日本語化 (`framework/makefw/makefiles/_flags.mk` で `FILES_LANG=ja_JP.utf8` 時に MSVC へ `/utf-8` が付与されることも確認)。あわせて `app/cjson/bin_test/test_patch_marker_check.py` の期待文字列を更新し、GCC 8 の C フロントエンドが `static_assert` 内の非 ASCII バイトを 8 進エスケープして出力する仕様を復号して厳密照合するよう対応。
- **WBS 4.3 / 4.4.x / 4.5**: `app/cjson/prod/include/cJSON_JSONC.h` (`cJSON_PatchCheck.h` の取り込みと `@par スレッド セーフ` 2 か所の更新)、利用側 11 ファイル (`app/porter` 5 ファイル、`app/struct-meta/prod/include/struct_meta/json/json.h` コメントなし、`app/cjson` 5 ファイル) への `#include <cJSON_PatchCheck.h>` 追加、および `app/cjson/prod/libsrc/cjson/jsonc/cJSON_JSONC.c` の `<assert.h>` 追加と `_Static_assert` 3 か所の `static_assert` 化を完了。
- **WBS 4.8 (検証結果)**:
    - `make -C app/cjson` / `make -C app/cjson test`: 成功 (終了コード 0)
    - `make bin-test BIN_TEST_ARGS=app/cjson`: 全 9 テスト成功 (終了コード 0)
    - `make -C app/cplat` / `make -C app/cplat test`: 成功 (終了コード 0)
    - `make -C app/porter` / `make -C app/porter test`: 成功 (終了コード 0)
    - `make -C app/struct-meta` / `make -C app/struct-meta test`: 成功 (終了コード 0)
    - 内容がある `.warn` ファイル (`app/cjson`、`app/cplat`、`app/porter`、`app/struct-meta`): 0 件
    - 一時領域 (scratchpad) でのパッチ `0003` 未適用時の検証: `extract_package.py` でパッチ `0001`・`0002` のみ適用した `cJSON.h` に対し、C17 (`gcc`) / C++17 (`g++`) の双方で `#error "cJSON にパッチ 0003 (global_error のスレッド ローカル化) が当たっていません"` によりコンパイルが失敗することを確認 (終了コード 1)
- **WBS 4.9 〜 4.11**: 受入条件 1〜7 の適合を確認し、対策元 `261126-cjson-patch-check-backlog.md` の BL-001 を「完了 (承認待ち)」へ更新、簡易 ADR (`261128-cjson-patch-marker-lightadr.md`) の LADR-004 経緯へレビュー指摘反映を追記、残課題一覧 (`261128-cjson-patch-marker-backlog.md`) を「整理済み」へ再構築。変更した Markdown は `text_style_jp.py` で確認済み。
