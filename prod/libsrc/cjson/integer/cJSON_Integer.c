/**
 *******************************************************************************
 *  @file           cJSON_Integer.c
 *  @brief          JSON の整数を double を経由せずに扱う拡張 API の実装です。
 *  @author         Tetsuo Honda
 *  @date           2026/08/30
 *  @version        1.0.0
 *
 *  仕様の正本は @ref cJSON_Integer.h (公開 API) と
 *  @ref cJSON_Integer_internal.h (ライブラリ内共有 API) です。本ファイルには
 *  実装の意図を説明するコメントだけを記載します。
 *
 *  保持方式の要点は次のとおりです。
 *  - 正確な整数は、正規化した十進表記 (`-?[0-9]+`) として `item->valuestring`
 *    に保持します。
 *  - 取得側は `valuestring` を再解釈した値と `valuedouble` の一致で整合性を
 *    確認し、一致しない場合 (`cJSON_SetIntValue` による書き換えなど) は
 *    従来の `valuedouble` 経路へ落とします。
 *******************************************************************************
 */

#include <cJSON_Integer.h>
#include <cJSON_Integer_internal.h>

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* int64_t / uint64_t の十進表記 (符号 + 20 桁 + 終端 NUL) を余裕をもって収める大きさ。
   配列境界には整数定数式が必要なため、static const ではなくマクロで表す。 */
#define DECIMAL_TEXT_BUFFER_SIZE 32

/* int64_t の表現範囲の上限を 1 超えた値 (2^63)。double として正確に表現できるため、
   これを「含まない上限」として使うと、直後の (int64_t) キャストが未定義動作にならない
   ことを保証できる。 */
static const double s_int64_upper_exclusive = 9223372036854775808.0;

/* uint64_t の表現範囲の上限を 1 超えた値 (2^64)。考え方は s_int64_upper_exclusive と同じ。 */
static const double s_uint64_upper_exclusive = 18446744073709551616.0;

/**
 *  @brief          トークンが `-?[0-9]+` に完全一致するかを判定します。
 *
 *  小数点、指数、符号の `+`、空白を含むトークン、`-` だけのトークン、空のトークンは
 *  すべて偽になります。呼び出し元は @p negative_out に非 NULL を渡します。
 */
static cJSON_bool is_plain_integer_token(const char *token, const size_t length, cJSON_bool *negative_out)
{
    if ((token == NULL) || (length == 0U))
    {
        return false;
    }

    cJSON_bool negative = false;
    size_t pos = 0U;

    if (token[0] == '-')
    {
        negative = true;
        pos = 1U;
    }

    if (pos >= length)
    {
        /* "-" だけ、または空のトークン */
        return false;
    }

    for (; pos < length; pos++)
    {
        if ((token[pos] < '0') || (token[pos] > '9'))
        {
            return false;
        }
    }

    *negative_out = negative;
    return true;
}

cJSON_bool cjson_internal_integer_store_text(cJSON *item, const char *token, const size_t length)
{
    if ((!cJSON_IsNumber(item)) || ((item->type & cJSON_IsReference) != 0))
    {
        return false;
    }

    cJSON_bool negative = false;
    if (!is_plain_integer_token(token, length, &negative))
    {
        return false;
    }

    /* strtoll / strtoull は NUL 終端文字列を要求する。token の length バイト目以降は
       この数値トークンの一部ではない可能性があるため、length バイトだけを切り出した
       一時バッファーへコピーしてから解釈する。 */
    char *buffer = (char *)cJSON_malloc(length + 1U);
    if (buffer == NULL)
    {
        return false;
    }
    memcpy(buffer, token, length);
    buffer[length] = '\0';

    char *endptr = NULL;
    char text[DECIMAL_TEXT_BUFFER_SIZE];
    int written;

    errno = 0;
    if (negative)
    {
        long long value = strtoll(buffer, &endptr, 10);
        if ((errno == ERANGE) || (endptr != (buffer + length)))
        {
            cJSON_free(buffer);
            return false;
        }
        /* strtoll / strtoull で解釈し直した値から表記を作り直すことで、
           先頭のゼロや "-0" を正規化する ("007" -> "7"、"-0" -> "0")。 */
        written = snprintf(text, sizeof(text), "%" PRId64, (int64_t)value);
    }
    else
    {
        unsigned long long value = strtoull(buffer, &endptr, 10);
        if ((errno == ERANGE) || (endptr != (buffer + length)))
        {
            cJSON_free(buffer);
            return false;
        }
        written = snprintf(text, sizeof(text), "%" PRIu64, (uint64_t)value);
    }

    cJSON_free(buffer);

    if ((written < 0) || ((size_t)written >= sizeof(text)))
    {
        /* DECIMAL_TEXT_BUFFER_SIZE は int64_t / uint64_t の最大表記を収める大きさであり、
           通常はここに到達しない。防御的な検査として残す。 */
        return false;
    }

    char *stored = (char *)cJSON_malloc((size_t)written + 1U);
    if (stored == NULL)
    {
        return false;
    }
    memcpy(stored, text, (size_t)written + 1U);

    if (item->valuestring != NULL)
    {
        cJSON_free(item->valuestring);
    }
    item->valuestring = stored;

    return true;
}

const char *cjson_internal_integer_text(const cJSON *item)
{
    if ((!cJSON_IsNumber(item)) || (item->valuestring == NULL))
    {
        return NULL;
    }

    size_t length = strlen(item->valuestring);
    cJSON_bool negative = false;
    if (!is_plain_integer_token(item->valuestring, length, &negative))
    {
        return NULL;
    }

    /* 整合性ガード: valuestring を解釈し直した値を double 化したものが valuedouble と
       一致する場合だけ、保持している表記を正とする。cJSON_SetIntValue はマクロで
       valueint / valuedouble を直接書き換えるため、この一致判定でしか改変を検知できない。 */
    char *endptr = NULL;
    double restored;

    errno = 0;
    if (negative)
    {
        long long value = strtoll(item->valuestring, &endptr, 10);
        if ((errno == ERANGE) || (endptr != (item->valuestring + length)))
        {
            return NULL;
        }
        restored = (double)value;
    }
    else
    {
        unsigned long long value = strtoull(item->valuestring, &endptr, 10);
        if ((errno == ERANGE) || (endptr != (item->valuestring + length)))
        {
            return NULL;
        }
        restored = (double)value;
    }

    if (restored != item->valuedouble)
    {
        return NULL;
    }

    return item->valuestring;
}

void cjson_internal_integer_clear(cJSON *item)
{
    if (!cJSON_IsNumber(item))
    {
        return;
    }

    if ((item->type & cJSON_IsReference) != 0)
    {
        /* 参照項目は表記を他の項目と共有しているため解放しない。 */
        return;
    }

    if (item->valuestring != NULL)
    {
        cJSON_free(item->valuestring);
        item->valuestring = NULL;
    }
}

/**
 *  @brief          符号付き整数の項目を生成する共通処理です。
 *
 *  cJSON_CreateNumber((double)value) で valuedouble / valueint を上流と同じ規則で
 *  設定したうえで、正規化した十進表記を valuestring へ設定します。value を double へ
 *  変換する時点で 2^53 を超える値の精度は失われますが、これは既存の cJSON_CreateNumber
 *  と同じ挙動であり意図的です。正確な値は valuestring 側で保持します。
 */
static cJSON *create_signed_integer(const int64_t value)
{
    cJSON *item = cJSON_CreateNumber((double)value);
    if (item == NULL)
    {
        return NULL;
    }

    char text[DECIMAL_TEXT_BUFFER_SIZE];
    int written = snprintf(text, sizeof(text), "%" PRId64, value);
    if ((written < 0) || ((size_t)written >= sizeof(text)) ||
        (!cjson_internal_integer_store_text(item, text, (size_t)written)))
    {
        cJSON_Delete(item);
        return NULL;
    }

    return item;
}

/** @copydoc create_signed_integer */
static cJSON *create_unsigned_integer(const uint64_t value)
{
    cJSON *item = cJSON_CreateNumber((double)value);
    if (item == NULL)
    {
        return NULL;
    }

    char text[DECIMAL_TEXT_BUFFER_SIZE];
    int written = snprintf(text, sizeof(text), "%" PRIu64, value);
    if ((written < 0) || ((size_t)written >= sizeof(text)) ||
        (!cjson_internal_integer_store_text(item, text, (size_t)written)))
    {
        cJSON_Delete(item);
        return NULL;
    }

    return item;
}

/**
 *  @brief          正確な整数、または整数値の valuedouble から符号付き 64bit 値を取り出します。
 *
 *  1. cjson_internal_integer_text が非 NULL なら、その表記を再解釈して返します。
 *  2. NULL なら、valuedouble が整数であり int64_t の範囲に収まる場合だけ変換します。
 */
static cJSON_bool try_get_signed(const cJSON *item, int64_t *value_out)
{
    if (!cJSON_IsNumber(item))
    {
        return false;
    }

    const char *text = cjson_internal_integer_text(item);
    if (text != NULL)
    {
        char *endptr = NULL;

        if (text[0] == '-')
        {
            /* 負の正確な整数は、格納時点で int64_t の範囲に収まることが
               保証されている (cjson_internal_integer_store_text 参照)。 */
            errno = 0;
            long long value = strtoll(text, &endptr, 10);
            if (errno == ERANGE)
            {
                return false;
            }
            *value_out = (int64_t)value;
            return true;
        }

        /* 非負の正確な整数は uint64_t の範囲までありうるため、int64_t の
           上限を超えていないかをここで検査する。 */
        errno = 0;
        unsigned long long value = strtoull(text, &endptr, 10);
        if ((errno == ERANGE) || (value > (unsigned long long)INT64_MAX))
        {
            return false;
        }
        *value_out = (int64_t)value;
        return true;
    }

    double raw = item->valuedouble;

    /* 範囲検査を先に行う。範囲外や NaN のまま int64_t へ変換すると未定義動作になる。
       否定形で書くことで、比較がすべて偽になる NaN もここで弾く。
       上限は 2^63 を含まない形にし、境界値のキャストが未定義動作にならないようにする。 */
    if (!((raw >= (double)INT64_MIN) && (raw < s_int64_upper_exclusive)))
    {
        return false;
    }
    if (raw != (double)(int64_t)raw)
    {
        return false;
    }

    *value_out = (int64_t)raw;
    return true;
}

/** @copydoc try_get_signed */
static cJSON_bool try_get_unsigned(const cJSON *item, uint64_t *value_out)
{
    if (!cJSON_IsNumber(item))
    {
        return false;
    }

    const char *text = cjson_internal_integer_text(item);
    if (text != NULL)
    {
        if (text[0] == '-')
        {
            /* 負の正確な整数は符号なし型では取得できない。 */
            return false;
        }

        char *endptr = NULL;
        errno = 0;
        unsigned long long value = strtoull(text, &endptr, 10);
        if (errno == ERANGE)
        {
            return false;
        }
        *value_out = (uint64_t)value;
        return true;
    }

    double raw = item->valuedouble;

    if (!((raw >= 0.0) && (raw < s_uint64_upper_exclusive)))
    {
        return false;
    }
    if (raw != (double)(uint64_t)raw)
    {
        return false;
    }

    *value_out = (uint64_t)raw;
    return true;
}

/**
 *  @brief          try_get_signed の結果を、目的の型の表現範囲で検査します。
 */
static cJSON_bool get_signed_ranged(const cJSON *item, const int64_t min_value, const int64_t max_value,
                                    int64_t *value_out)
{
    int64_t value;

    if (!try_get_signed(item, &value))
    {
        return false;
    }
    if ((value < min_value) || (value > max_value))
    {
        return false;
    }

    *value_out = value;
    return true;
}

/**
 *  @brief          try_get_unsigned の結果を、目的の型の表現範囲で検査します。
 */
static cJSON_bool get_unsigned_ranged(const cJSON *item, const uint64_t max_value, uint64_t *value_out)
{
    uint64_t value;

    if (!try_get_unsigned(item, &value))
    {
        return false;
    }
    if (value > max_value)
    {
        return false;
    }

    *value_out = value;
    return true;
}

CJSON_PUBLIC(cJSON_bool) cJSON_HasExactInteger(const cJSON *item)
{
    return (cJSON_bool)(cjson_internal_integer_text(item) != NULL);
}

CJSON_PUBLIC(cJSON *) cJSON_CreateInt8(const int8_t value)
{
    return create_signed_integer((int64_t)value);
}

CJSON_PUBLIC(cJSON *) cJSON_CreateUInt8(const uint8_t value)
{
    return create_unsigned_integer((uint64_t)value);
}

CJSON_PUBLIC(cJSON *) cJSON_CreateInt16(const int16_t value)
{
    return create_signed_integer((int64_t)value);
}

CJSON_PUBLIC(cJSON *) cJSON_CreateUInt16(const uint16_t value)
{
    return create_unsigned_integer((uint64_t)value);
}

CJSON_PUBLIC(cJSON *) cJSON_CreateInt32(const int32_t value)
{
    return create_signed_integer((int64_t)value);
}

CJSON_PUBLIC(cJSON *) cJSON_CreateUInt32(const uint32_t value)
{
    return create_unsigned_integer((uint64_t)value);
}

CJSON_PUBLIC(cJSON *) cJSON_CreateInt64(const int64_t value)
{
    return create_signed_integer(value);
}

CJSON_PUBLIC(cJSON *) cJSON_CreateUInt64(const uint64_t value)
{
    return create_unsigned_integer(value);
}

CJSON_PUBLIC(cJSON_bool) cJSON_GetInt8Value(const cJSON *item, int8_t *value_out)
{
    int64_t value;

    if (!get_signed_ranged(item, INT8_MIN, INT8_MAX, &value))
    {
        return false;
    }
    if (value_out != NULL)
    {
        *value_out = (int8_t)value;
    }
    return true;
}

CJSON_PUBLIC(cJSON_bool) cJSON_GetUInt8Value(const cJSON *item, uint8_t *value_out)
{
    uint64_t value;

    if (!get_unsigned_ranged(item, UINT8_MAX, &value))
    {
        return false;
    }
    if (value_out != NULL)
    {
        *value_out = (uint8_t)value;
    }
    return true;
}

CJSON_PUBLIC(cJSON_bool) cJSON_GetInt16Value(const cJSON *item, int16_t *value_out)
{
    int64_t value;

    if (!get_signed_ranged(item, INT16_MIN, INT16_MAX, &value))
    {
        return false;
    }
    if (value_out != NULL)
    {
        *value_out = (int16_t)value;
    }
    return true;
}

CJSON_PUBLIC(cJSON_bool) cJSON_GetUInt16Value(const cJSON *item, uint16_t *value_out)
{
    uint64_t value;

    if (!get_unsigned_ranged(item, UINT16_MAX, &value))
    {
        return false;
    }
    if (value_out != NULL)
    {
        *value_out = (uint16_t)value;
    }
    return true;
}

CJSON_PUBLIC(cJSON_bool) cJSON_GetInt32Value(const cJSON *item, int32_t *value_out)
{
    int64_t value;

    if (!get_signed_ranged(item, INT32_MIN, INT32_MAX, &value))
    {
        return false;
    }
    if (value_out != NULL)
    {
        *value_out = (int32_t)value;
    }
    return true;
}

CJSON_PUBLIC(cJSON_bool) cJSON_GetUInt32Value(const cJSON *item, uint32_t *value_out)
{
    uint64_t value;

    if (!get_unsigned_ranged(item, UINT32_MAX, &value))
    {
        return false;
    }
    if (value_out != NULL)
    {
        *value_out = (uint32_t)value;
    }
    return true;
}

CJSON_PUBLIC(cJSON_bool) cJSON_GetInt64Value(const cJSON *item, int64_t *value_out)
{
    int64_t value;

    if (!get_signed_ranged(item, INT64_MIN, INT64_MAX, &value))
    {
        return false;
    }
    if (value_out != NULL)
    {
        *value_out = value;
    }
    return true;
}

CJSON_PUBLIC(cJSON_bool) cJSON_GetUInt64Value(const cJSON *item, uint64_t *value_out)
{
    uint64_t value;

    if (!get_unsigned_ranged(item, UINT64_MAX, &value))
    {
        return false;
    }
    if (value_out != NULL)
    {
        *value_out = value;
    }
    return true;
}
