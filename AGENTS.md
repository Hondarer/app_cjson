# AGENTS.md

## 対象

このリポジトリは、cJSON の配布アーカイブをワークスペースへ取り込むラッパーであり、cJSON を利用するプログラムの単体テスト向け API モックも含みます。

## 必須参照

- [README.md](README.md)
- パッケージの展開と更新では、README.md の手順を確認してください。

## 注意点

- `packages/` から展開された cJSON 本体を直接編集しないでください。
- mock の API 表を変更した場合は、公開関数の網羅テストと README.md の利用方法を確認してください。
- Windows の利用側では `CJSON_HIDE_SYMBOLS`、リンク先では `mock_cjson` を使用し、実ライブラリを同時リンクしないでください。
