# cjson

この Markdown は、機能の入り口としての Markdown で、ドキュメント化対象外です。
開発者に対してフォルダーの説明やショートカットを配置し、案内することが目的です。

## 概要

[cJSON](https://github.com/DaveGamble/cJSON) (MIT License) のコア機能
(cJSON.c / cJSON.h) および拡張ユーティリティ (cJSON_Utils.c / cJSON_Utils.h)
を取り込んだライブラリです。

ビルド成果物は静的ライブラリ `libcjson.a` のみです (shared 版は作りません)。

## パッケージの配置手順 (初回セットアップ)

本リポジトリは cJSON のソース アーカイブ (zip) を Git 管理下に手動配置する
運用です。`app/cjson/packages/` に、以下のいずれかの方法で取得した zip を
1 個だけ配置してください。

- ブラウザで GitHub の Releases / Tags からダウンロード
- curl での取得例:

  ```sh
  curl -L -o app/cjson/packages/cJSON-1.7.18.zip \
    https://github.com/DaveGamble/cJSON/archive/refs/tags/v1.7.18.zip
  ```

配置後、`make` (または `make test`) を実行すると、
`app/cjson/bin/extract_package.py` が自動的に
`prod/include/cjson/`, `prod/libsrc/cjson/` へ展開します。
展開先はいずれも生成物であり `.gitignore` 対象のため、Git には
コミットしないでください。

`packages/` にアーカイブが存在しない状態で `make` を実行すると、
ビルドはエラーで停止し、配置方法の案内が表示されます。

`packages/` に複数のアーカイブが存在する場合はエラーにはせず、
ファイル名のバージョン番号が最も新しいものを自動的に採用します
(バージョン番号が読み取れないファイルが混在する場合は、更新日時が
最も新しいものを採用します)。この場合、採用されなかったアーカイブを
削除するよう警告が表示されるので、単一ファイル運用に戻してください。

## バージョン更新手順

1. 新しいバージョンの zip を取得し、`app/cjson/packages/` に追加する。
2. 古いバージョンの zip を削除する (`packages/` には常に 1 個のみを置く運用)。
3. `make` を実行すると、新しい zip のタイムスタンプが展開済み生成物より
   新しいと判定され、自動的に再展開されます。

## ライセンス

cJSON は MIT License です。ライセンス条文の正本は
`packages/cJSON-<version>.zip` 内の `LICENSE` です。
展開後は参考コピーとして `prod/libsrc/cjson/LICENSE.cjson`
(生成物、.gitignore 対象) にも配置されます。

## サンプルとテスト

- `prod/src/cmd/cjson_sample/` : cJSON の最小利用例 (実行ファイル)
- `test/src/cmd/cjsonTest/` : 独自の動作テスト (gtest ベース)

`test/src/cmd/cjsonTest` を単独ビルドする場合は、事前に
`make -C app/cjson/prod` (または app ルートで `make`) を実行して
`libcjson.a` を用意してください。
