/**
 *******************************************************************************
 *  @file           cJSON_Integer.h
 *  @brief          JSON の整数を double を経由せずに扱う拡張 API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/08/30
 *  @version        1.0.0
 *
 *  cJSON は数値を `valuedouble` (double) と `valueint` (int) に保持するため、
 *  2^53 を超える整数は値が壊れ、出力も指数表記になります。\n
 *  本拡張は、小数点も指数も含まない数値トークンを、正規化した十進表記として
 *  `valuestring` に保持し、出力時にその表記をそのまま書き出します。\n
 *  これにより `int64_t` と `uint64_t` の全域を正確に往復できます。
 *
 *  本拡張は既存の cJSON API の挙動を変えません。\n
 *  `valuedouble` と `valueint` は従来どおり設定され、`cJSON_GetNumberValue` の
 *  戻り値も変わりません。\n
 *  変わるのは 2^53 を超える整数の出力表記だけであり、これは指数表記から
 *  正確な整数表記への修正です。
 *
 *  @par            型の扱い
 *  JSON の線上に整数の幅は残りません。`int32_t` の 42 も `uint64_t` の 42 も
 *  同じ `42` です。\n
 *  そのため本 API は型を保持せず、取得側が目的の型を指定します。\n
 *  値が指定した型に収まらない場合、取得関数は偽を返し、格納先を変更しません。
 *
 *  @par            前提
 *  `valuestring` は上流の cJSON では文字列と生値の項目にだけ使われ、
 *  数値の項目では常に NULL です。本拡張は数値の項目でこれを使います。\n
 *  数値の項目で `valuestring` を参照する利用側のコードは、本拡張の導入により
 *  NULL 以外を受け取ります。文字列の取得には型を検査する
 *  `cJSON_GetStringValue` を使ってください。
 *******************************************************************************
 */

#ifndef CJSON_INTEGER_H
#define CJSON_INTEGER_H

#include <stdint.h>

#include <cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief          項目が正確な整数を保持しているかを調べます。
 *  @param[in]      item    対象。NULL を渡せます。
 *  @return         正確な整数を保持していれば真、そうでなければ偽。
 *
 *  数値の項目であっても、小数や指数を含む値、`int64_t` にも `uint64_t` にも
 *  収まらない値では偽を返します。\n
 *  利用側が `valuedouble` を直接書き換えた項目でも偽を返します。保持している
 *  整数と `valuedouble` が食い違う項目を、正確な整数として扱わないためです。
 *
 *  @par            スレッド セーフ
 *  条件付きスレッド セーフです。\n
 *  異なる項目への同時呼び出しはできます。\n
 *  同一項目への書き込みと同時に呼んではなりません。
 */
CJSON_PUBLIC(cJSON_bool) cJSON_HasExactInteger(const cJSON *item);

/**
 *  @brief          8 ビット符号付き整数の項目を生成します。
 *  @param[in]      value   保持する値。
 *  @return         生成した項目。確保に失敗した場合は NULL。
 *
 *  生成した項目は @ref cJSON_Delete で解放してください。
 *
 *  @par            スレッド セーフ
 *  スレッド セーフです。\n
 *  内部に共有状態を持たず、呼び出しごとに独立した項目を生成します。
 */
CJSON_PUBLIC(cJSON *) cJSON_CreateInt8(int8_t value);

/**
 *  @brief          8 ビット符号なし整数の項目を生成します。
 *  @param[in]      value   保持する値。
 *  @return         生成した項目。確保に失敗した場合は NULL。
 *
 *  生成した項目は @ref cJSON_Delete で解放してください。
 *
 *  @par            スレッド セーフ
 *  スレッド セーフです。\n
 *  内部に共有状態を持たず、呼び出しごとに独立した項目を生成します。
 */
CJSON_PUBLIC(cJSON *) cJSON_CreateUInt8(uint8_t value);

/**
 *  @brief          16 ビット符号付き整数の項目を生成します。
 *  @param[in]      value   保持する値。
 *  @return         生成した項目。確保に失敗した場合は NULL。
 *
 *  生成した項目は @ref cJSON_Delete で解放してください。
 *
 *  @par            スレッド セーフ
 *  スレッド セーフです。\n
 *  内部に共有状態を持たず、呼び出しごとに独立した項目を生成します。
 */
CJSON_PUBLIC(cJSON *) cJSON_CreateInt16(int16_t value);

/**
 *  @brief          16 ビット符号なし整数の項目を生成します。
 *  @param[in]      value   保持する値。
 *  @return         生成した項目。確保に失敗した場合は NULL。
 *
 *  生成した項目は @ref cJSON_Delete で解放してください。
 *
 *  @par            スレッド セーフ
 *  スレッド セーフです。\n
 *  内部に共有状態を持たず、呼び出しごとに独立した項目を生成します。
 */
CJSON_PUBLIC(cJSON *) cJSON_CreateUInt16(uint16_t value);

/**
 *  @brief          32 ビット符号付き整数の項目を生成します。
 *  @param[in]      value   保持する値。
 *  @return         生成した項目。確保に失敗した場合は NULL。
 *
 *  生成した項目は @ref cJSON_Delete で解放してください。
 *
 *  @par            スレッド セーフ
 *  スレッド セーフです。\n
 *  内部に共有状態を持たず、呼び出しごとに独立した項目を生成します。
 */
CJSON_PUBLIC(cJSON *) cJSON_CreateInt32(int32_t value);

/**
 *  @brief          32 ビット符号なし整数の項目を生成します。
 *  @param[in]      value   保持する値。
 *  @return         生成した項目。確保に失敗した場合は NULL。
 *
 *  生成した項目は @ref cJSON_Delete で解放してください。
 *
 *  @par            スレッド セーフ
 *  スレッド セーフです。\n
 *  内部に共有状態を持たず、呼び出しごとに独立した項目を生成します。
 */
CJSON_PUBLIC(cJSON *) cJSON_CreateUInt32(uint32_t value);

/**
 *  @brief          64 ビット符号付き整数の項目を生成します。
 *  @param[in]      value   保持する値。
 *  @return         生成した項目。確保に失敗した場合は NULL。
 *
 *  生成した項目は @ref cJSON_Delete で解放してください。
 *
 *  @par            スレッド セーフ
 *  スレッド セーフです。\n
 *  内部に共有状態を持たず、呼び出しごとに独立した項目を生成します。
 */
CJSON_PUBLIC(cJSON *) cJSON_CreateInt64(int64_t value);

/**
 *  @brief          64 ビット符号なし整数の項目を生成します。
 *  @param[in]      value   保持する値。
 *  @return         生成した項目。確保に失敗した場合は NULL。
 *
 *  生成した項目は @ref cJSON_Delete で解放してください。
 *
 *  @par            スレッド セーフ
 *  スレッド セーフです。\n
 *  内部に共有状態を持たず、呼び出しごとに独立した項目を生成します。
 */
CJSON_PUBLIC(cJSON *) cJSON_CreateUInt64(uint64_t value);

/**
 *  @brief          項目の値を 8 ビット符号付き整数として取得します。
 *  @param[in]      item        対象。NULL を渡せます。
 *  @param[out]     value_out   値の格納先。NULL を渡せます。
 *  @return         取得できれば真、そうでなければ偽。
 *
 *  次のいずれかに該当する場合は偽を返し、@p value_out を変更しません。\n
 *  - @p item が NULL である、または数値の項目でない。\n
 *  - 値が整数でない。\n
 *  - 値が `int8_t` の表現範囲に収まらない。
 *
 *  正確な整数を保持していない通常の数値の項目でも、値が整数で `int8_t` に
 *  収まれば取得できます。
 *
 *  @par            スレッド セーフ
 *  条件付きスレッド セーフです。\n
 *  異なる項目への同時呼び出しはできます。\n
 *  同一項目への書き込みと同時に呼んではなりません。
 */
CJSON_PUBLIC(cJSON_bool) cJSON_GetInt8Value(const cJSON *item, int8_t *value_out);

/**
 *  @brief          項目の値を 8 ビット符号なし整数として取得します。
 *  @param[in]      item        対象。NULL を渡せます。
 *  @param[out]     value_out   値の格納先。NULL を渡せます。
 *  @return         取得できれば真、そうでなければ偽。
 *
 *  次のいずれかに該当する場合は偽を返し、@p value_out を変更しません。\n
 *  - @p item が NULL である、または数値の項目でない。\n
 *  - 値が整数でない。\n
 *  - 値が `uint8_t` の表現範囲に収まらない。
 *
 *  正確な整数を保持していない通常の数値の項目でも、値が整数で `uint8_t` に
 *  収まれば取得できます。
 *
 *  @par            スレッド セーフ
 *  条件付きスレッド セーフです。\n
 *  異なる項目への同時呼び出しはできます。\n
 *  同一項目への書き込みと同時に呼んではなりません。
 */
CJSON_PUBLIC(cJSON_bool) cJSON_GetUInt8Value(const cJSON *item, uint8_t *value_out);

/**
 *  @brief          項目の値を 16 ビット符号付き整数として取得します。
 *  @param[in]      item        対象。NULL を渡せます。
 *  @param[out]     value_out   値の格納先。NULL を渡せます。
 *  @return         取得できれば真、そうでなければ偽。
 *
 *  次のいずれかに該当する場合は偽を返し、@p value_out を変更しません。\n
 *  - @p item が NULL である、または数値の項目でない。\n
 *  - 値が整数でない。\n
 *  - 値が `int16_t` の表現範囲に収まらない。
 *
 *  正確な整数を保持していない通常の数値の項目でも、値が整数で `int16_t` に
 *  収まれば取得できます。
 *
 *  @par            スレッド セーフ
 *  条件付きスレッド セーフです。\n
 *  異なる項目への同時呼び出しはできます。\n
 *  同一項目への書き込みと同時に呼んではなりません。
 */
CJSON_PUBLIC(cJSON_bool) cJSON_GetInt16Value(const cJSON *item, int16_t *value_out);

/**
 *  @brief          項目の値を 16 ビット符号なし整数として取得します。
 *  @param[in]      item        対象。NULL を渡せます。
 *  @param[out]     value_out   値の格納先。NULL を渡せます。
 *  @return         取得できれば真、そうでなければ偽。
 *
 *  次のいずれかに該当する場合は偽を返し、@p value_out を変更しません。\n
 *  - @p item が NULL である、または数値の項目でない。\n
 *  - 値が整数でない。\n
 *  - 値が `uint16_t` の表現範囲に収まらない。
 *
 *  正確な整数を保持していない通常の数値の項目でも、値が整数で `uint16_t` に
 *  収まれば取得できます。
 *
 *  @par            スレッド セーフ
 *  条件付きスレッド セーフです。\n
 *  異なる項目への同時呼び出しはできます。\n
 *  同一項目への書き込みと同時に呼んではなりません。
 */
CJSON_PUBLIC(cJSON_bool) cJSON_GetUInt16Value(const cJSON *item, uint16_t *value_out);

/**
 *  @brief          項目の値を 32 ビット符号付き整数として取得します。
 *  @param[in]      item        対象。NULL を渡せます。
 *  @param[out]     value_out   値の格納先。NULL を渡せます。
 *  @return         取得できれば真、そうでなければ偽。
 *
 *  次のいずれかに該当する場合は偽を返し、@p value_out を変更しません。\n
 *  - @p item が NULL である、または数値の項目でない。\n
 *  - 値が整数でない。\n
 *  - 値が `int32_t` の表現範囲に収まらない。
 *
 *  正確な整数を保持していない通常の数値の項目でも、値が整数で `int32_t` に
 *  収まれば取得できます。
 *
 *  @par            スレッド セーフ
 *  条件付きスレッド セーフです。\n
 *  異なる項目への同時呼び出しはできます。\n
 *  同一項目への書き込みと同時に呼んではなりません。
 */
CJSON_PUBLIC(cJSON_bool) cJSON_GetInt32Value(const cJSON *item, int32_t *value_out);

/**
 *  @brief          項目の値を 32 ビット符号なし整数として取得します。
 *  @param[in]      item        対象。NULL を渡せます。
 *  @param[out]     value_out   値の格納先。NULL を渡せます。
 *  @return         取得できれば真、そうでなければ偽。
 *
 *  次のいずれかに該当する場合は偽を返し、@p value_out を変更しません。\n
 *  - @p item が NULL である、または数値の項目でない。\n
 *  - 値が整数でない。\n
 *  - 値が `uint32_t` の表現範囲に収まらない。
 *
 *  正確な整数を保持していない通常の数値の項目でも、値が整数で `uint32_t` に
 *  収まれば取得できます。
 *
 *  @par            スレッド セーフ
 *  条件付きスレッド セーフです。\n
 *  異なる項目への同時呼び出しはできます。\n
 *  同一項目への書き込みと同時に呼んではなりません。
 */
CJSON_PUBLIC(cJSON_bool) cJSON_GetUInt32Value(const cJSON *item, uint32_t *value_out);

/**
 *  @brief          項目の値を 64 ビット符号付き整数として取得します。
 *  @param[in]      item        対象。NULL を渡せます。
 *  @param[out]     value_out   値の格納先。NULL を渡せます。
 *  @return         取得できれば真、そうでなければ偽。
 *
 *  次のいずれかに該当する場合は偽を返し、@p value_out を変更しません。\n
 *  - @p item が NULL である、または数値の項目でない。\n
 *  - 値が整数でない。\n
 *  - 値が `int64_t` の表現範囲に収まらない。
 *
 *  正確な整数を保持していない通常の数値の項目でも、値が整数で `int64_t` に
 *  収まれば取得できます。
 *
 *  @par            スレッド セーフ
 *  条件付きスレッド セーフです。\n
 *  異なる項目への同時呼び出しはできます。\n
 *  同一項目への書き込みと同時に呼んではなりません。
 */
CJSON_PUBLIC(cJSON_bool) cJSON_GetInt64Value(const cJSON *item, int64_t *value_out);

/**
 *  @brief          項目の値を 64 ビット符号なし整数として取得します。
 *  @param[in]      item        対象。NULL を渡せます。
 *  @param[out]     value_out   値の格納先。NULL を渡せます。
 *  @return         取得できれば真、そうでなければ偽。
 *
 *  次のいずれかに該当する場合は偽を返し、@p value_out を変更しません。\n
 *  - @p item が NULL である、または数値の項目でない。\n
 *  - 値が整数でない。\n
 *  - 値が `uint64_t` の表現範囲に収まらない。
 *
 *  正確な整数を保持していない通常の数値の項目でも、値が整数で `uint64_t` に
 *  収まれば取得できます。
 *
 *  @par            スレッド セーフ
 *  条件付きスレッド セーフです。\n
 *  異なる項目への同時呼び出しはできます。\n
 *  同一項目への書き込みと同時に呼んではなりません。
 */
CJSON_PUBLIC(cJSON_bool) cJSON_GetUInt64Value(const cJSON *item, uint64_t *value_out);

#ifdef __cplusplus
}
#endif

#endif /* CJSON_INTEGER_H */
