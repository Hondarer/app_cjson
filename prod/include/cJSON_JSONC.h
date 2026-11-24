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

    /** JSONC の解析の結果の種別です。 */
    typedef enum cjson_app_jsonc_status
    {
        CJSON_APP_JSONC_OK = 0,            /**< 解析に成功しました。 */
        CJSON_APP_JSONC_SYNTAX_ERROR = 1,  /**< 構文が不正です。error_offset に位置を格納します。 */
        CJSON_APP_JSONC_OUT_OF_MEMORY = 2, /**< 解析の間にメモリを確保できませんでした。 */
        CJSON_APP_JSONC_INVALID_ARGUMENT =
            3 /**< value が NULL、buffer_length が SIZE_MAX、または入力に NUL が含まれます。 */
    } cjson_app_jsonc_status;

    /**
     *  @brief          コメントと末尾カンマを含む JSONC 文字列を解析します。
     *  @param[in]      value         入力文字列。NULL、入力内の NUL は指定できません。
     *  @param[in]      buffer_length 終端 NUL を含まない入力のバイト数。SIZE_MAX は指定できません。
     *  @return         成功時は解析結果を返します。構文が不正、引数が不正、または
     *                  メモリを確保できない場合は NULL を返します。
     *
     *  スラッシュ 2 個の行コメントと、スラッシュ・アスタリスクで囲むブロック
     *  コメント、およびオブジェクトと配列の末尾カンマを受け付けます。
     *  入力文字列は変更しません。戻り値は @ref cjson_app_cJSON_Delete で解放してください。
     *  value が NULL、buffer_length が SIZE_MAX、または入力に NUL が含まれる場合は NULL を返します。
     *  解析の間に、このスレッドで cJSON の確保が失敗した場合は、cJSON の解析が成功しても
     *  結果を解放して NULL を返します。整数の付加情報の確保の失敗も対象です。\n
     *  ロード時にサイズを記録する確保と消去する解放のフックを設定します。\n
     *  フックは libcjson_app の cJSON API 全体、cJSON_Utils、整数拡張に適用します。確保ごとにヘッダー分の容量が増え、解放時に要求サイズ全体を消去します。\n
     *  cJSON_InitHooks() による解除・再差し替えは禁止します。確保領域と出力バッファーは cjson_app_cJSON_free() で解放します。\n
     *  通常の free() と realloc() は使わず、通常の malloc() で確保した領域を cJSON の値やキーへ代入しません。
     *
     *  @par            スレッド セーフ
     *  本関数は条件付きスレッド セーフです。\n
     *  新旧の JSONC のラッパーを通る解析は、プロセス全体で 1 つのロックにより直列に実行します。\n
     *  本関数自身はロックを取らず、cjson_app_cJSON_ParseJSONCWithLengthOpts() が 1 回だけ取ります。\n
     *  cJSON を直接呼ぶ解析および cJSON_GetErrorPtr() の参照と同時に実行せず、
     *  入力文字列を同時に変更せず、ロード後に cJSON のアロケーター フックを変更しない場合に、同時に呼び出せます。\n
     *  struct-meta の struct_meta_json_file_load() が呼ぶ cjson_app_cJSON_Parse() は、このロックを通りません。
     */
    CJSON_PUBLIC(cJSON *) cjson_app_cJSON_ParseJSONCWithLength(const char *value, size_t buffer_length);

    /**
     *  @brief          コメントと末尾カンマを含む JSONC 文字列を解析し、失敗の種別と位置を返します。
     *  @param[in]      value         入力文字列。NULL、入力内の NUL は指定できません。
     *  @param[in]      buffer_length 終端 NUL を含まない入力のバイト数。SIZE_MAX は指定できません。
     *  @param[out]     status        結果の種別を格納するポインター。NULL は指定できません。
     *  @param[out]     error_offset  構文の誤りの場合に、value の先頭からの位置 [byte] を格納するポインター。
     *                                NULL を指定できます。構文の誤り以外では 0 を格納します。
     *  @return         成功時は解析結果を返します。失敗時は NULL を返します。
     *
     *  status が NULL の場合は NULL を返し、種別は格納しません。\n
     *  value が NULL、buffer_length が SIZE_MAX、または入力に NUL が含まれる場合は、
     *  NULL と CJSON_APP_JSONC_INVALID_ARGUMENT を返します。\n
     *  解析の間に、このスレッドで cJSON の確保の関数が失敗した場合は、解析が成功しても結果を解放し、
     *  NULL と CJSON_APP_JSONC_OUT_OF_MEMORY を返します。\n
     *  error_offset は、cJSON が解析を止めた位置です。誤りのある字句より後ろを指す場合があります。
     *  たとえば `{,}` では `}`、文字列の不正なエスケープではバックスラッシュを指します。\n
     *  閉じていないブロック コメントでは、そのコメントの開始の位置です。\n
     *  コメントと末尾カンマの除去は、同じ位置の文字を空白へ置き換えるため、位置は入力の位置と一致します。\n
     *  ロード時にサイズを記録する確保と消去する解放のフックを設定します。\n
     *  フックは libcjson_app の cJSON API 全体、cJSON_Utils、整数拡張に適用します。確保ごとにヘッダー分の容量が増え、解放時に要求サイズ全体を消去します。\n
     *  cJSON_InitHooks() による解除・再差し替えは禁止します。確保領域と出力バッファーは cjson_app_cJSON_free() で解放します。\n
     *  通常の free() と realloc() は使わず、通常の malloc() で確保した領域を cJSON の値やキーへ代入しません。\n
     *  入力文字列は変更しません。戻り値は @ref cjson_app_cJSON_Delete で解放してください。
     *
     *  @par            スレッド セーフ
     *  本関数は条件付きスレッド セーフです。\n
     *  新旧の JSONC のラッパーを通る解析は、プロセス全体で 1 つのロックにより直列に実行します。\n
     *  cJSON を直接呼ぶ解析および cJSON_GetErrorPtr() の参照と同時に実行せず、
     *  入力文字列を同時に変更せず、ロード後に cJSON のアロケーター フックを変更しない場合に、同時に呼び出せます。\n
     *  struct-meta の struct_meta_json_file_load() が呼ぶ cjson_app_cJSON_Parse() は、このロックを通りません。\n
     *  status と error_offset の出力先を、ほかの呼び出しと同時に書き換えないでください。
     */
    CJSON_PUBLIC(cJSON *)
    cjson_app_cJSON_ParseJSONCWithLengthOpts(const char *value, size_t buffer_length, cjson_app_jsonc_status *status,
                                             size_t *error_offset);

#ifdef __cplusplus
}
#endif

#endif /* CJSON_JSONC_H */
