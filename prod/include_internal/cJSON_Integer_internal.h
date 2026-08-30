/**
 *******************************************************************************
 *  @file           cJSON_Integer_internal.h
 *  @brief          整数拡張のライブラリ内共有 API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/08/30
 *  @version        1.0.0
 *
 *  公開 API は @ref cJSON_Integer.h です。本ヘッダーは libcjson の内部だけで
 *  使用します。\n
 *  パッチを当てた `cJSON.c` と、手書きの `prod/libsrc/cjson/integer/` 配下の
 *  実装が、この宣言を共有します。
 *
 *  本ヘッダーが宣言する関数は @ref CJSON_PUBLIC を付けないため、共有ライブラリ
 *  から公開されません。Linux では `-fvisibility=hidden` により隠蔽され、
 *  Windows では `__declspec(dllexport)` が付かないため公開されません。
 *
 *  @par            保持方式
 *  正確な整数は、正規化した十進表記として `item->valuestring` に保持します。\n
 *  数値の項目の `valuestring` は上流の cJSON では常に NULL であり、かつ
 *  `cJSON_Delete` による解放、`cJSON_Duplicate` による複製、
 *  `create_reference` の二重解放回避、`cJSON_Utils` の `overwrite_item` による
 *  差し替えが、いずれも上流の既存処理のまま正しく働きます。
 *******************************************************************************
 */

#ifndef CJSON_INTEGER_INTERNAL_H
#define CJSON_INTEGER_INTERNAL_H

#include <stddef.h>

#include <cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief          数値トークンが正確な整数なら、正規化した表記を保持します。
 *  @param[in,out]  item    対象の数値項目。NULL を渡せます。
 *  @param[in]      token   数値トークンの先頭。NULL を渡せます。
 *  @param[in]      length  @p token のうち数値として消費された長さ。
 *  @return         保持したら真、保持しなかったら偽。
 *
 *  @p token の先頭 @p length バイトが `-?[0-9]+` に完全一致し、かつ値が
 *  `int64_t` または `uint64_t` に収まる場合だけ保持します。\n
 *  小数点、指数、符号の `+`、前後の空白を含むトークンは保持しません。呼び出し側
 *  で小数点や指数の有無を判定する必要はありません。\n
 *  保持する表記は `strtoll` または `strtoull` で解釈し直してから再生成するため、
 *  先頭のゼロや `-0` は正規化されます。\n
 *  保持に成功した場合、`item->valuestring` に確保した文字列を設定します。
 *  既存の `valuestring` があれば解放します。
 *
 *  @par            スレッド セーフ
 *  条件付きスレッド セーフです。\n
 *  異なる項目への同時呼び出しはできます。\n
 *  同一項目への他の操作と同時に呼んではなりません。
 */
cJSON_bool cjson_internal_integer_store_text(cJSON *item, const char *token, size_t length);

/**
 *  @brief          保持している正確な整数の十進表記を返します。
 *  @param[in]      item    対象。NULL を渡せます。
 *  @return         十進表記。保持していない場合は NULL。
 *
 *  次のすべてを満たす場合だけ表記を返します。\n
 *  - @p item が数値の項目である。\n
 *  - `valuestring` が NULL でない。\n
 *  - 保持している整数を double へ変換した値が `valuedouble` と一致する。
 *
 *  3 番目は整合性ガードです。`cJSON_SetIntValue` はマクロで `valueint` と
 *  `valuedouble` を直接書き換えるため関数側から検知できません。書き換えられた
 *  項目では一致しなくなり、従来の double 経路へ落ちます。
 *
 *  @par            スレッド セーフ
 *  条件付きスレッド セーフです。\n
 *  異なる項目への同時呼び出しはできます。\n
 *  同一項目への書き込みと同時に呼んではなりません。
 */
const char *cjson_internal_integer_text(const cJSON *item);

/**
 *  @brief          保持している正確な整数の表記を破棄します。
 *  @param[in,out]  item    対象。NULL を渡せます。
 *
 *  数値の項目の `valuestring` を解放して NULL に戻します。\n
 *  参照項目 (`cJSON_IsReference`) では、表記を他の項目と共有しているため
 *  解放しません。
 *
 *  @par            スレッド セーフ
 *  条件付きスレッド セーフです。\n
 *  異なる項目への同時呼び出しはできます。\n
 *  同一項目への他の操作と同時に呼んではなりません。
 */
void cjson_internal_integer_clear(cJSON *item);

#ifdef __cplusplus
}
#endif

#endif /* CJSON_INTEGER_INTERNAL_H */
