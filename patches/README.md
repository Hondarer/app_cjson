# patches

このディレクトリには、`packages/` の配布アーカイブから展開した cJSON 本体へ適用するパッチを配置します。

展開された `prod/include/cJSON.h`、`prod/include/cJSON_Utils.h`、`prod/libsrc/cjson/cJSON.c`、`prod/libsrc/cjson/cJSON_Utils.c` は生成物です。  
これらを直接編集しないでください。再展開で上書きされます。  
cJSON 本体へ手を入れる唯一の方法は、このディレクトリへパッチを追加することです。

## 適用の仕組み

`make` 実行時に `bin/extract_package.py` がアーカイブを展開し、続けてこのディレクトリのパッチを適用します。  
適用器は `framework/makefw/bin/apply_patches.py` です。標準ライブラリだけで動作し、`patch` や `git apply` に依存しません。

適用は **厳密** です。文脈行が 1 バイトでも一致しない場合、探索や fuzz による救済を行わずビルドを停止します。  
アーカイブのバージョンを更新した際にパッチが当たらなくなったら、それは上流の変更を確認すべき合図です。

## ファイルの規約

- ファイル名は `NNNN-<要約>.patch` とします。適用順はファイル名の昇順です。
- 形式は unified diff です。`--- a/<path>` と `+++ b/<path>` の見出しを持ちます。
- パスは先頭 1 階層を除去したうえで、`app/cjson` からの相対パスとして解決します (`git apply -p1` 相当)。
- 差分だけを書き、説明文は含めないでください。パッチの意図はこの README に記載します。

## 収録しているパッチ

| ファイル | 対象 | 目的 |
|---|---|---|
| `0001-windows-dll-import-default.patch` | `prod/include/cJSON.h` | Windows の利用側で既定の `CJSON_IMPORT_SYMBOLS` を定義します。 |
| `0002-exact-integer-hooks.patch` | `prod/libsrc/cjson/cJSON.c` | 正確な整数の保持と出力のフックを 5 箇所へ追加します。 |

`0002` の実装本体は `prod/libsrc/cjson/integer/` にあり、パッチ側は呼び出しだけを追加します。  
上流バージョン更新時の追随範囲を小さく保つための分割です。  
公開 API は `prod/include/cJSON_Integer.h`、ライブラリ内共有の宣言は `prod/include_internal/cJSON_Integer_internal.h` です。

## パッチの追加と再生成

1. 展開済みのファイルを別の場所へ複製します。
2. 複製を編集します。
3. `diff -u` で差分を取り、見出しの行を `--- a/<相対パス>` と `+++ b/<相対パス>` に書き換えます (タイムスタンプは残さないでください)。
4. このディレクトリへ配置し、`make clean` の後に `make` を実行して適用されることを確認します。

パッチを追加または変更すると `make_extract.stamp` のダイジェストが変わり、次回の `make` で自動的に再展開と再適用が行われます。
