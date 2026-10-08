/**
 *******************************************************************************
 *  @file           cJSON_JSONC.h
 *  @brief          JSONC 文字列を解析する拡張 API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/09/01
 *  @version        1.0.0
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef CJSON_JSONC_H
#define CJSON_JSONC_H

#include <cJSON.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     *  @brief          コメントと末尾カンマを含む JSONC 文字列を解析します。
     *  @param[in]      value         入力文字列。NULL は指定できません。
     *  @param[in]      buffer_length 終端 NUL を含まない入力のバイト数。
     *  @return         成功時は解析結果を返します。構文が不正、引数が不正、または
     *                  メモリを確保できない場合は NULL を返します。
     *
     *  スラッシュ 2 個の行コメントと、スラッシュ・アスタリスクで囲むブロック
     *  コメント、およびオブジェクトと配列の末尾カンマを受け付けます。
     *  入力文字列は変更しません。戻り値は @ref cJSON_Delete で解放してください。
     *
     *  @par            スレッド セーフ
     *  本関数は条件付きスレッド セーフです。\n
     *  cJSON のアロケーター フックおよび入力文字列を同時に変更しない場合は、同時に実行できます。\n
     *  他スレッドがそれらを同時に変更する場合は、呼び出し側で同期してください。
     */
    CJSON_PUBLIC(cJSON *) cJSON_ParseJSONCWithLength(const char *value, size_t buffer_length);

#ifdef __cplusplus
}
#endif

#endif /* CJSON_JSONC_H */
