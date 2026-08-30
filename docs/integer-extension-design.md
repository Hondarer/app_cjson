# cJSON 正確な整数型拡張の設計

## 目的

cJSON が JSON の数値を double でしか保持しないために壊れる整数を、正確に往復できるようにします。

対象は次の 8 種類です。

- `int8_t` / `uint8_t`
- `int16_t` / `uint16_t`
- `int32_t` / `uint32_t`
- `int64_t` / `uint64_t`

## 背景

cJSON は `parse_number()` で数値トークンを `strtod()` に渡し、結果を `valuedouble` (double) と `valueint` (int) に保持します。  
double の仮数部は 53 ビットのため、2^53 を超える整数は値が壊れ、`cJSON_Print()` の出力も指数表記に変わります。

この制約はすでに実害を出しています。  
`app/struct-meta` は `prod/libsrc/struct_meta/json/json.h` で `STRUCT_META_JSON_INTEGER_LIMIT` を `999999999999999` (10^15 - 1) と定義し、`decode.c` と `encode.c` の双方でこれを超える整数を `CPLAT_ERR_OUT_OF_RANGE` として拒否しています。  
C の構造体を JSON へ往復させる用途で、64 ビット整数のフィールドが扱えていません。

## 制約

cJSON 本体は `packages/` の配布アーカイブから機械的に展開する生成物です。  
改変は `patches/` のパッチを通してのみ行い、上流バージョンの更新へ追随できる大きさに保つ必要があります。

`struct cJSON` は公開構造体です。  
フィールド、順序、サイズ、アラインメントを変えると、既存アプリケーションのソース互換性だけでなく ABI 互換性も壊れます。

## 採用しなかった方式

### 外部テーブルによる sidecar

`cJSON *` を鍵とする外部テーブルに整数を保持する案です。

採用できません。  
`app/c-platform/appdeps.mk` がすでに `APP_DEPS := cjson` を宣言しており (cplat の `sym_loader` が JSON 設定の解析に cJSON を使用)、`app/cjson` から cplat の hashtable へ依存すると循環します。  
`framework/makefw/bin/resolve_app_deps.sh` は循環依存を検出してビルドを停止します。

自前のテーブルを持つ案も、解放済みアドレスの再利用による誤対応と、全参照でのロックという設計負債を抱えます。

### 割り当て拡張 (トレーラー)

`cJSON_New_Item()` の確保サイズを広げ、`cJSON` の直後に隠し領域を置く案です。  
`sizeof(cJSON)` も構造体定義も変わらないため、当初はこれを採用する予定でした。

採用できません。  
上流に `sizeof(cJSON)` の浅いコピーが 3 箇所あり、隠し領域が同期しないためです。

| 箇所 | 影響 |
|---|---|
| `cJSON.c` の `create_reference()` | 参照項目が整数値を失います。 |
| `cJSON_Utils.c` の `overwrite_item()` | 差し替え後も古い整数値が残り、表示上の値と食い違います。 |
| `cJSON_Utils.c` の `apply_patch()` | 上流自身が、`cJSON_New_Item()` を経ずに定義した静的記憶域の `cJSON` を値渡ししています。 |

3 番目が決定的です。  
`cJSON_Utils.c` の 843 行は `static const cJSON invalid = { ... };` と書いており、`cJSON_New_Item()` を経ない `cJSON` を値渡しで `overwrite_item()` へ渡します。  
「API 経由で生成した項目しか渡されない」という前提が、利用側ではなく上流 cJSON 自身の中ですでに破られています。  
ワークスペース内にも `test/src/libcjsonTest/mockCjsonTest/mockCjsonTest.cc` の 71 行にスタック上の `cJSON item = {};` があります。

この方式ではパッチ箇所が `cJSON_Utils.c` にまで広がり、追随範囲が 2 ファイルになります。

## 採用した方式

正規化した十進表記を `item->valuestring` に保持します。

数値の項目の `valuestring` は上流の cJSON では常に NULL です。  
そしてライフサイクルに関わる処理が、いずれも上流の既存コードのまま正しく働きます。

| 上流の処理 | 挙動 |
|---|---|
| `cJSON_Delete()` | 参照でなければ `valuestring` を解放します。 |
| `cJSON_Duplicate_rec()` | `valuestring` を複製します。 |
| `create_reference()` | ポインターを共有し `cJSON_IsReference` で二重解放を避けます。 |
| `overwrite_item()` | 旧 `valuestring` を解放してから `memcpy` します。 |
| `cJSON_SetValuestring()` | `cJSON_String` 以外を弾くため、数値の項目を書き換えません。 |

解放、複製、参照、差し替えのいずれにもパッチが不要です。

### 正規化

トークンをそのまま複写せず、`strtoll` または `strtoull` で解釈してから `snprintf` で作り直します。

- `007` は `7` に、`-0` は `0` になり、出力が常に妥当な JSON になります。
- 正規形どうしの一致が値の一致と等価になり、`cJSON_Compare` を文字列比較で実装できます。
- 符号の区別は先頭の `-` の有無だけで判別でき、別途のフラグが不要です。

### 整合性ガード

`cJSON_SetIntValue` は `cJSON.h` のマクロで、`valueint` と `valuedouble` を直接書き換えます。  
関数側からは検知できないため、保持した表記が古い値のまま出力される危険があります。

そこで、保持した表記を採用するのは `(double)復元値 == item->valuedouble` が成り立つときだけとします。  
パース時と生成時は `valuedouble` へ同じ変換結果を書くため常に一致し、利用側が `valuedouble` を直接書き換えた場合だけ従来の double 経路へ落ちます。  
公開マクロを一切変更せずに済みます。

## パッチの範囲

`cJSON.c` への差分は 5 ハンクです。  
`cJSON_Utils.c` は無変更です。

| 箇所 | 変更 |
|---|---|
| ヘッダー取り込み | `cJSON_Integer_internal.h` を追加します。 |
| `parse_number()` | 消費済みトークンを `cjson_internal_integer_store_text()` へ渡します。 |
| `print_number()` | 保持している表記があれば、それをそのまま出力する分岐を足します。 |
| `cJSON_Compare()` | 双方が正確な整数を保持していれば正規形の一致で比較します。 |
| `cJSON_SetNumberHelper()` | double での上書きなので保持している表記を破棄します。 |

トークンの妥当性は保存関数が `-?[0-9]+` として自前で検証します。  
そのため `parse_number()` のトークン走査に指数の判定を足す必要がなく、上流の走査ロジックには一切触れません。

実装本体は `prod/libsrc/cjson/integer/` に置き、パッチ側は呼び出しだけを追加します。  
上流バージョン更新時の追随範囲を小さく保つための分割です。

## 公開 API

型を保持しません。

JSON の線上に整数の幅は残らないためです。  
`int32_t` の 42 も `uint64_t` の 42 も、同じ `42` として出力されます。  
そのため型は取得側が指定し、値が収まらなければ取得関数が偽を返します。

- 生成 8 種: `cJSON_CreateInt8` から `cJSON_CreateUInt64`
- 取得 8 種: `cJSON_GetInt8Value` から `cJSON_GetUInt64Value`
- 判定 1 種: `cJSON_HasExactInteger`

取得関数は 2 段構えです。  
正確な整数を保持していればそこから復元し、保持していない通常の数値でも、値が整数で目的の型に収まれば取得できます。

公開ヘッダーは `prod/include/cJSON_Integer.h`、ライブラリ内共有の宣言は `prod/include_internal/cJSON_Integer_internal.h` です。

## 既知の制約

- 数値の項目の `valuestring` が NULL でなくなります。文字列の取得には、型を検査する `cJSON_GetStringValue` を使ってください。
- 2^53 を超える整数の `cJSON_Print()` 出力が、指数表記から正確な整数表記へ変わります。これは修正ですが、出力文字列を固定値と比較している利用側があれば影響します。
- 利用側が `valuedouble` を、保持している整数と同じ値の double へ書き換えた場合、整合性ガードは一致と判定します。この場合は元の正確な整数が出力されます。
- `cJSON_AddInt64ToObject()` のような利便関数は今回の対象外です。

## 進捗

| 段階 | 内容 | 状態 |
|---|---|---|
| 1 | `framework/makefw/bin/apply_patches.py` (共有パッチ適用器) | 完了 |
| 2 | 公開ヘッダーと内部ヘッダーの確定 | 完了 |
| 3 | `patches/0001` と `patches/0002` の作成 | 完了 |
| 4 | `prod/libsrc/cjson/integer/` の実装とビルド配線 | 完了 |
| 5 | `bin/extract_package.py` のパッチ方式への移行 | 完了 |
| 6 | ビルド署名へ `patches/` と `packages/` を含める修正 (makefw) | 完了 |
| 7 | 往復テスト `test/src/cmd/cjsonIntegerTest/` の追加 | 完了 |
| 8 | `app/struct-meta` を新 API へ移行し 10^15 の上限を撤廃 | 完了 |
| 9 | `app/sqlite` と `app/lua` のパッチ方式への移行 | 完了 |
| 10 | 規範の除外パターンの見直し | 完了 |

段階 9 では、移行前の変換規則をアーカイブ原本へ適用した結果と移行後の展開結果がバイト一致することを、3 app すべてで確認しています。

段階 10 では `app/general/docs/coding-guideline.md` の除外パターンを、展開された OSS 本体のパスだけを除く形へ狭めました。
これにより 3 app の `test/` 配下と `app/cjson/prod/libsrc/cjson/integer/` の手書きコードが規範検査の対象に入ります。
あわせて、mock の API 表に使う X マクロを関数形式マクロの用途ホワイト リストへ追加しました。
[共有ライブラリの mock 化](../../general/docs/shared-library-mock-guideline.md) が定める正規の方式でありながら、ホワイト リストに載っていなかったためです。

段階 6 は、実装中に見つかった既存の欠陥への対処です。  
`framework/makefw/bin/resolve_app_deps.sh` が計算するビルド署名に `patches/` と `packages/` が含まれておらず、パッチや配布アーカイブを変更しても `make clean` を挟まない限り再展開が走りませんでした。  
パッチ方式では日常的な編集がこの欠陥を踏むため、根因を修正します。
