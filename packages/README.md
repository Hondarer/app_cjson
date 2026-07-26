# packages

このディレクトリには、[cJSON の Releases ページ](https://github.com/DaveGamble/cJSON/releases) から取得した cJSON のリリース アーカイブ (zip) を配置します。

- ファイル名の例: `cJSON-1.7.19.zip` (バージョン番号をファイル名に含めること)
- 常に 1 個のみを配置する運用とします。バージョン更新時は、古いアーカイブを削除し、新しいアーカイブに置き換えてください。
- `make` 実行時、`app/cjson/bin/extract_package.py` がここに配置された zip を自動検出し、`prod/include/`, `prod/libsrc/cjson/` へ展開します。
- 万一複数のアーカイブが存在する場合はエラーにはせず、ファイル名のバージョン番号が最も新しいものを自動的に採用し、未採用のアーカイブを削除するよう警告を表示します。

取得方法や配置手順の詳細は [../README.md](../README.md) を参照してください。
