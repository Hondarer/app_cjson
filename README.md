# cjson

作業前に [作業規則](AGENTS.md) を確認してください。

このリポジトリは [c-modernization-kit](https://github.com/Hondarer/c-modernization-kit) の `app/cjson` としてサブモジュール管理される、個別アプリ リポジトリです。  
c-modernization-kit のワークスペース内 (`framework/makefw` 等と組み合わせた make ビルド環境) で利用することを前提としたラッパー リポジトリであり、本リポジトリ単体ではビルドできません。

## 固有の公開 API 名で OS のライブラリと分離します

OS の同名ライブラリと同じプロセスで関数解決が混ざることを防ぐため、公開関数・変数と関数呼び出しマクロに `cjson_app_` を付けます。  
たとえば `cJSON_Parse` の利用側は `cjson_app_cJSON_Parse` を呼び出します。  
型名、定数、構造体フィールドは配布ライブラリの定義を使います。  
旧名の alias と mock ラッパーは提供しないため、既存の利用側はソースを更新して再ビルド・再リンクしてください。  
`dlsym()` と `GetProcAddress()` に渡す文字列も固有名へ変更します。

## 概要

[cJSON](https://github.com/DaveGamble/cJSON) (MIT License) のコア機能 (cJSON.c / cJSON.h) および拡張ユーティリティ (cJSON_Utils.c / cJSON_Utils.h) を、c-modernization-kit の makefw 規約に沿って取り込んだラッパー ライブラリです。  
cJSON のリリース アーカイブからソースを展開して利用します。  
`appdeps.mk` は cJSON の公開ヘッダーを外来ヘッダーとして定義します。  
利用側では Linux の `-isystem` または MSVC の `/external:I` で警告を分離します。

cJSON を利用するプログラムの単体テスト向けに、Google Mock 対応の cJSON API モックも含みます。

JSONC のコメントと末尾カンマを扱う拡張 API を `cJSON_JSONC.h` で提供します。

ビルド成果物は動的ライブラリのみです。

- Linux: `libcjson_app.so`
- Windows: `libcjson_app.dll` およびリンク用の import library `libcjson_app.lib`

静的ライブラリは生成しません。

## JSONC の解析

`cjson_app_cJSON_ParseJSONCWithLength()` は、行コメント、ブロック コメント、オブジェクトと配列の末尾カンマを含む文字列を解析します。  
長さには終端 NUL を含まないバイト数を指定します。入力は変更せず、成功時の戻り値は `cjson_app_cJSON_Delete()` で解放します。構文エラーやメモリ不足の場合は `NULL` を返します。

```c
#include <cJSON_JSONC.h>
#include <string.h>

const char *source = "{\"ports\": [5001,], // first service\n}";
cJSON *root = cjson_app_cJSON_ParseJSONCWithLength(source, strlen(source));
/* root を使用する */
cjson_app_cJSON_Delete(root);
```

通常の `cjson_app_cJSON_Parse()` は JSONC に変更せず、従来どおり JSON を解析します。

## パッケージの配置手順 (初回セットアップ)

本リポジトリは cJSON のソース アーカイブ (zip) を Git 管理下に手動配置する運用です。`app/cjson/packages/` に、以下のいずれかの方法で取得した zip を 1 つだけ配置してください。

- ブラウザーで GitHub の Releases / Tags からダウンロード
- curl での取得例:

  ```sh
  curl -L -o app/cjson/packages/cJSON-1.7.18.zip \
    https://github.com/DaveGamble/cJSON/archive/refs/tags/v1.7.18.zip
  ```

配置後、`make` (または `make test`) を実行すると、 `app/cjson/bin_internal/extract_package.py` が自動的に `prod/include/`, `prod/libsrc/cjson/` へ展開します。  
展開する `cJSON.h` には警告抑制 pragma を追加しません。  
利用側の警告分離は `appdeps.mk` の提供側定義を通じて makefw が行います。  
展開先はいずれも生成物であり `.gitignore` 対象です。

展開に続けて、`app/cjson/patches/` の unified diff を自動的に適用します。  
適用器は `framework/makefw/bin_internal/apply_patches.py` (app 間で共有する適用器) です。  
cJSON 本体への変更はすべてこのパッチ経由で行い、展開直後のファイルを直接編集することはありません。  
パッチの一覧と個々の目的は [patches/README.md](patches/README.md) を参照してください。

`packages/` にアーカイブが存在しない状態で `make` を実行すると、ビルドはエラーで停止し、配置方法の案内が表示されます。

`packages/` に複数のアーカイブが存在する場合はエラーにはせず、ファイル名のバージョン番号が最も新しいものを自動的に採用します (バージョン番号が読み取れないファイルが混在する場合は、更新日時が最も新しいものを採用します)。  
この場合、採用されなかったアーカイブを削除するよう警告が表示されるので、単一ファイル運用に戻してください。

`app/cjson/patches/0002-exact-integer-hooks.patch` により、数値項目の `valuestring` が非 `NULL` になる場合があります (正確な整数値を保持するための副作用)。  
数値項目から文字列を取得する際は、型を検査する `cjson_app_cJSON_GetStringValue` を使用してください。`valuestring` を直接参照しないでください。

## バージョン更新手順

1. 新しいバージョンの zip を取得し、`app/cjson/packages/` に追加します。
2. 古いバージョンの zip を削除します (`packages/` には常に 1 個のみを配置する運用)。
3. `make` を実行すると、新しい zip の内容またはパッチの内容が展開済み生成物と異なると判定され、自動的に再展開とパッチ再適用が行われます。

## ライセンス

cJSON は MIT License です。ライセンス条文の正本は `packages/cJSON-<version>.zip` 内の `LICENSE` です。  
展開後は参考コピーとして `prod/libsrc/cjson/LICENSE.cjson` (生成物、.gitignore 対象) にも配置されます。  
`libcjson_app.so` または `libcjson_app.dll` を利用したアプリケーションを配布する際は、MIT License に従い、著作権表示、MIT ライセンス本文、無保証・免責条項を含めてください。

## サンプルとテスト

- `prod/src/cmd/cjson_sample/` : cJSON の最小利用例
- `test/src/cmd/cjsonTest/` : 動作テスト
- `test/src/libcjsonTest/mockCjsonTest/` : cJSON API モックの動作テスト
- `test/src/libcjsonTest/exportTest/` : API 表、IDENT manifest、実ライブラリの全エクスポートが一致することを検出するテスト

## cJSON API モック

テスト対象ソースを `TEST_SRCS` に指定し、mock をリンクするテストの `makepart.mk` に `DEFINES += MOCK_CJSON_ENABLE_CALLER` を追加します。  
これにより呼び出し元情報の注入を有効にし、実ライブラリを使うテストには注入しません。

mock の宣言と注入マクロは API 表から生成し、mock ライブラリのビルド前に自動更新します。  
生成済みの `test/include/mock_cjson_overrides.h` は直接編集せず、通常 API 表と、可変長 API がある場合は専用の可変長 API 表を変更してください。

通常の API 呼び出しは差し替えヘッダーを通し、mock の先頭 3 引数へ呼び出し元のファイル名、行番号、関数名を渡します。  
`EXPECT_CALL` と `ON_CALL` でも、この 3 引数を実 API の引数より先に指定します。  
新しい固有名の関数アドレスを取得して関数ポインター経由で呼び出す場合も mock に到達しますが、呼び出し元は取得できないため、ファイル名と関数名は `"<unknown>"`、行番号は `0` になります。  
実委譲は app の固有シンボルだけを解決し、OS 側の旧名へ戻す探索は行いません。

`test/lib/libmock_cjson.a` は、cJSON を利用する app の単体テストで使用する Google Mock 対応ライブラリです。  
`cJSON.h`、`cJSON_Utils.h`、`cJSON_Integer.h`、`cJSON_JSONC.h` の公開関数を対象とし、`Mock_cjson` を生成しない場合と、生成後に個別の動作を指定しない場合は、`libcjson_app` の実関数を呼び出します。

テスト コードでは `mock_cjson.h` をインクルードし、テスト対象の `makepart.mk` では `cjson_app` の代わりに `mock_cjson` をリンクします。  
`cjson_app` と `mock_cjson` を同時にリンクしないでください。Linux では実ライブラリの強シンボルが弱定義のモックを上書きし、`EXPECT_CALL` が効かなくなります。

```makefile
ifdef PLATFORM_WINDOWS
    DEFINES += CJSON_HIDE_SYMBOLS
endif

LIBS += mock_cjson
```

Windows では、`CJSON_HIDE_SYMBOLS` により cJSON の DLL import 宣言を無効にし、`mock_cjson` が提供する実シンボルを参照します。  
実関数への委譲では、Linux の `LD_LIBRARY_PATH` または Windows の `PATH` から `libcjson_app` を読み込みます。

振る舞いを変更するテストでは、`Mock_cjson` を生成して `EXPECT_CALL` または `ON_CALL` を指定します。

```cpp
NiceMock<Mock_cjson> mock_cjson;

EXPECT_CALL(mock_cjson, cjson_app_cJSON_Parse(_, _, _, StrEq("{}")))
    .WillOnce(Return(nullptr));
```

実オブジェクトを生成せずに単体隔離する場合は、SUT が呼び出す関数をすべてスタブしてください。  
スタブしていない呼び出しは実関数へ委譲されるため、偽ポインターを渡すと実関数が失敗します。

テスト用のポインターを返すように変更した場合は、実関数がそのポインターを解放しないよう、対応する `cjson_app_cJSON_Delete` や `cjson_app_cJSON_free` の動作も指定してください。

`cJSON` は公開構造体です。`item->valuestring` や `cJSON_ArrayForEach`、`cJSON_SetIntValue`、`cJSON_SetBoolValue` はフィールドを直接読み書きするため、関数モックでは差し替えられません。  
偽ポインターを返す場合は、SUT がフィールドへアクセスしないこと、またはテスト側で `cJSON` グラフを組み立てることを確認してください。
