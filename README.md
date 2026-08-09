# cjson

このリポジトリは [c-modernization-kit](https://github.com/Hondarer/c-modernization-kit) の `app/cjson` としてサブモジュール管理される、個別アプリ リポジトリです。  
c-modernization-kit のワークスペース内 (`framework/makefw` 等と組み合わせた make ビルド環境) で利用することを前提としたラッパー リポジトリであり、本リポジトリ単体ではビルドできません。

## 概要

[cJSON](https://github.com/DaveGamble/cJSON) (MIT License) のコア機能 (cJSON.c / cJSON.h) および拡張ユーティリティ (cJSON_Utils.c / cJSON_Utils.h)  
を、c-modernization-kit の makefw 規約に沿って取り込んだラッパー ライブラリです。  
cJSON のリリース アーカイブからソースを展開して利用します。  
生成する `cJSON.h` には、upstream の構造体レイアウトを変更せずに `-Wpadded` を抑制する GCC diagnostic pragma を付加します。

ビルド成果物は動的ライブラリのみです。

- Linux: `libcjson.so`
- Windows: `libcjson.dll` およびリンク用の import library `libcjson.lib`

静的ライブラリ (`libcjson.a` および静的リンク用の `libcjson.lib`) は生成しません。

## パッケージの配置手順 (初回セットアップ)

本リポジトリは cJSON のソース アーカイブ (zip) を Git 管理下に手動配置する運用です。`app/cjson/packages/` に、以下のいずれかの方法で取得した zip を 1 つだけ配置してください。

- ブラウザーで GitHub の Releases / Tags からダウンロード
- curl での取得例:

  ```sh
  curl -L -o app/cjson/packages/cJSON-1.7.18.zip \
    https://github.com/DaveGamble/cJSON/archive/refs/tags/v1.7.18.zip
  ```

配置後、`make` (または `make test`) を実行すると、 `app/cjson/bin/extract_package.py` が自動的に `prod/include/`, `prod/libsrc/cjson/` へ展開します。  
展開時に `cJSON.h` 全体を GCC diagnostic push/pop で囲むため、このヘッダーをインクルードする app では個別の `-Wpadded` 抑制が不要です。  
展開先はいずれも生成物であり `.gitignore` 対象です。

`packages/` にアーカイブが存在しない状態で `make` を実行すると、ビルドはエラーで停止し、配置方法の案内が表示されます。

`packages/` に複数のアーカイブが存在する場合はエラーにはせず、ファイル名のバージョン番号が最も新しいものを自動的に採用します (バージョン番号が読み取れないファイルが混在する場合は、更新日時が最も新しいものを採用します)。  
この場合、採用されなかったアーカイブを削除するよう警告が表示されるので、単一ファイル運用に戻してください。

## バージョン更新手順

1. 新しいバージョンの zip を取得し、`app/cjson/packages/` に追加する。
2. 古いバージョンの zip を削除する (`packages/` には常に 1 個のみを置く運用)。
3. `make` を実行すると、新しい zip のタイムスタンプが展開済み生成物より新しいと判定され、自動的に再展開されます。

## ライセンス

cJSON は MIT License です。ライセンス条文の正本は `packages/cJSON-<version>.zip` 内の `LICENSE` です。  
展開後は参考コピーとして `prod/libsrc/cjson/LICENSE.cjson` (生成物、.gitignore 対象) にも配置されます。  
`libcjson.so` または `libcjson.dll` を利用したアプリケーションを配布する際は、MIT License に従い、著作権表示、MIT ライセンス本文、無保証・免責条項を含めてください。

## サンプルとテスト

- `prod/src/cmd/cjson_sample/` : cJSON の最小利用例
- `test/src/cmd/cjsonTest/` : 動作テスト

## テスト用モック

`test/lib/libmock_cjson.a` は、cJSON を利用する app の単体テストで使用する Google Mock 対応ライブラリです。  
`cJSON.h` と `cJSON_Utils.h` の公開関数を対象とし、`Mock_cjson` を生成しない場合と、生成後に個別の動作を指定しない場合は、`libcjson` の実関数を呼び出します。

テスト コードでは `mock_cjson.h` をインクルードし、テスト対象の `makepart.mk` では `cjson` の代わりに `mock_cjson` をリンクします。

```makefile
ifdef PLATFORM_WINDOWS
    DEFINES += CJSON_HIDE_SYMBOLS
endif

LIBS += mock_cjson
```

Windows では、`CJSON_HIDE_SYMBOLS` により cJSON の DLL import 宣言を無効にし、`mock_cjson` が提供する実シンボルを参照します。  
実関数への委譲では、Linux の `LD_LIBRARY_PATH` または Windows の `PATH` から `libcjson` を読み込みます。

振る舞いを変更するテストでは、`Mock_cjson` を生成して `EXPECT_CALL` または `ON_CALL` を指定します。

```cpp
NiceMock<Mock_cjson> mock_cjson;

EXPECT_CALL(mock_cjson, cJSON_Parse(StrEq("{}")))
    .WillOnce(Return(nullptr));
```

テスト用のポインターを返すように変更した場合は、実関数がそのポインターを解放しないよう、対応する `cJSON_Delete` や `cJSON_free` の動作も必要に応じて指定してください。
