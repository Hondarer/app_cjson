/**
 *******************************************************************************
 *  @file           cJSON_JSONC.c
 *  @brief          JSONC 文字列の解析を実装します。
 *  @author         Tetsuo Honda
 *  @date           2026/09/01
 *  @version        1.0.0
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <cJSON_JSONC.h>
#include <cjson_app_thread_local.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
    #include <windows.h>
#endif

/*
 * 要求サイズをヘッダーに記録し、返すポインターを max_align_t 以上に揃える。
 * 解放のとき、このヘッダーを含めて要求サイズ全体を消去する。
 *
 * MSVC の C コンパイルでは stddef.h が max_align_t を定義しない。
 * 同じ処理系の C++ では max_align_t は double であり、整列はそれに合わせる。
 * see: https://learn.microsoft.com/en-us/cpp/standard-library/cstddef
 */
#if defined(_MSC_VER) && !defined(__cplusplus)
typedef double max_align_t;
#endif

typedef union jsonc_block_header
{
    struct
    {
        size_t requested_size;
    } fields;
    max_align_t alignment;
} jsonc_block_header;

static_assert(_Alignof(jsonc_block_header) >= _Alignof(max_align_t), "JSONC の確保ヘッダーの整列が不足しています");
static_assert(sizeof(jsonc_block_header) % _Alignof(max_align_t) == 0U,
              "JSONC の確保ヘッダーの大きさが整列の倍数ではありません");
static_assert(sizeof(jsonc_block_header) > 0U, "JSONC の確保ヘッダーが空です");

/* このスレッドで、フックが確保に失敗した回数です。解析の開始時の値と比べます。 */
static CJSON_APP_THREAD_LOCAL size_t jsonc_allocation_failure_count = 0U;

static void *CJSON_CDECL jsonc_hook_malloc(size_t size);
static void CJSON_CDECL jsonc_hook_free(void *pointer);
static void jsonc_install_allocation_hooks(void);
#if defined(_WIN32)
#else
__attribute__((constructor)) static void jsonc_on_load(void);
#endif
static size_t config_strip_comments(char *text);
static void config_strip_trailing_commas(char *text);
static size_t jsonc_syntax_offset(const char *normalized, const char *end, size_t buffer_length);

#if defined(_WIN32)
/*
 * DllMain では cJSON_InitHooks() の代入だけを行う。
 * 確保、同期、LoadLibrary は呼ばない。
 * see: https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices
 *
 * DLL_PROCESS_ATTACH では、フック関数の設定だけを行う。
 * see: https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-entry-point-function
 */
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void)instance;
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH)
    {
        jsonc_install_allocation_hooks();
    }
    return TRUE;
}
#endif

/* Doxygen コメントは、ヘッダーに記載 */
CJSON_PUBLIC(cJSON *)
cjson_app_cJSON_ParseJSONCWithLengthOpts(const char *value, size_t buffer_length, cjson_app_jsonc_status *status,
                                         size_t *error_offset)
{
    char *normalized;
    cJSON *result;
    const char *end;
    size_t failure_count_at_entry;
    size_t unclosed_offset;
    size_t offset;

    if (error_offset != NULL)
    {
        *error_offset = 0U;
    }
    if (status == NULL)
    {
        return NULL;
    }
    if (value == NULL || buffer_length == SIZE_MAX || memchr(value, '\0', buffer_length) != NULL)
    {
        *status = CJSON_APP_JSONC_INVALID_ARGUMENT;
        return NULL;
    }

    failure_count_at_entry = jsonc_allocation_failure_count;
    normalized = (char *)cjson_app_cJSON_malloc(buffer_length + 1U);
    if (normalized == NULL)
    {
        *status = CJSON_APP_JSONC_OUT_OF_MEMORY;
        return NULL;
    }
    memcpy(normalized, value, buffer_length);
    normalized[buffer_length] = '\0';

    unclosed_offset = config_strip_comments(normalized);
    if (unclosed_offset != SIZE_MAX)
    {
        if (error_offset != NULL)
        {
            *error_offset = unclosed_offset;
        }
        cjson_app_cJSON_free(normalized);
        *status = CJSON_APP_JSONC_SYNTAX_ERROR;
        return NULL;
    }
    config_strip_trailing_commas(normalized);

    /*
     * cJSON の誤りの位置 (global_error) はパッチ 0003 でスレッド ローカルにしてあるため、
     * ほかのスレッドの解析と並行に呼び出せる。
     */
    end = NULL;
    result = cjson_app_cJSON_ParseWithLengthOpts(normalized, buffer_length + 1U, &end, 1);

    if (jsonc_allocation_failure_count != failure_count_at_entry)
    {
        if (result != NULL)
        {
            cjson_app_cJSON_Delete(result);
        }
        cjson_app_cJSON_free(normalized);
        *status = CJSON_APP_JSONC_OUT_OF_MEMORY;
        return NULL;
    }
    if (result == NULL)
    {
        offset = jsonc_syntax_offset(normalized, end, buffer_length);
        cjson_app_cJSON_free(normalized);
        if (error_offset != NULL)
        {
            *error_offset = offset;
        }
        *status = CJSON_APP_JSONC_SYNTAX_ERROR;
        return NULL;
    }

    cjson_app_cJSON_free(normalized);
    *status = CJSON_APP_JSONC_OK;
    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */
CJSON_PUBLIC(cJSON *) cjson_app_cJSON_ParseJSONCWithLength(const char *value, size_t buffer_length)
{
    cjson_app_jsonc_status status;

    status = CJSON_APP_JSONC_INVALID_ARGUMENT;
    return cjson_app_cJSON_ParseJSONCWithLengthOpts(value, buffer_length, &status, NULL);
}

/*
 * 確保と解放は、同じモジュールの malloc と free を呼ぶ。
 * cJSON の確保関数は呼ばない。呼ぶとフックが再入する。
 * cJSON の処理の途中から呼ばれるため、JSONC の解析と利用側のコールバックは行わない。
 */
static void *CJSON_CDECL jsonc_hook_malloc(size_t size)
{
    jsonc_block_header *header;
    unsigned char *block;
    size_t total;

    if (size > (SIZE_MAX - sizeof(*header)))
    {
        jsonc_allocation_failure_count++;
        return NULL;
    }
    if (size == 0U)
    {
        total = sizeof(*header);
    }
    else
    {
        total = sizeof(*header) + size;
    }
    block = (unsigned char *)malloc(total);
    if (block == NULL)
    {
        jsonc_allocation_failure_count++;
        return NULL;
    }
    header = (jsonc_block_header *)block;
    memset(header, 0, sizeof(*header));
    header->fields.requested_size = size;
    return block + sizeof(*header);
}

static void CJSON_CDECL jsonc_hook_free(void *pointer)
{
    jsonc_block_header *header;
    unsigned char *block;
    volatile unsigned char *bytes;
    size_t requested;
    size_t total;
    size_t index;

    if (pointer == NULL)
    {
        return;
    }
    block = (unsigned char *)pointer - sizeof(*header);
    header = (jsonc_block_header *)block;
    requested = header->fields.requested_size;
    total = sizeof(*header) + requested;
    /*
     * 要求サイズとヘッダーの全バイトを 0 にする。
     * volatile は、コンパイラが不要なストアとして除去しないようにするためであり、スレッド同期には使わない。
     * strlen は使わない。NUL より後ろのバイトも消す。
     * see: https://wiki.sei.cmu.edu/confluence/display/c/MSC06-C.+Beware+of+compiler+optimizations
     */
    bytes = (volatile unsigned char *)block;
    for (index = 0U; index < total; index++)
    {
        bytes[index] = 0U;
    }
    free(block);
}

/*
 * ロード時に 1 回だけ確保関数を差し替える。
 * 実行中の差し替えは、ほかのスレッドの解析とデータ競合になり、
 * Windows の MSVC_CRT=static ではモジュールごとにヒープが分かれて解放が壊れる。
 * 以後の解除と再差し替えは、全スレッドを止めた場合も禁止する。
 */
static void jsonc_install_allocation_hooks(void)
{
    cJSON_Hooks hooks;

    hooks.malloc_fn = jsonc_hook_malloc;
    hooks.free_fn = jsonc_hook_free;
    cjson_app_cJSON_InitHooks(&hooks);
}

#if defined(_WIN32)
#else
/*
 * 依存される共有ライブラリの初期化は、依存する側より前に動く。
 * see: https://refspecs.linuxfoundation.org/elf/gabi4+/ch5.dynamic.html
 */
__attribute__((constructor)) static void jsonc_on_load(void)
{
    jsonc_install_allocation_hooks();
}
#endif

/* 文字列リテラルを保持し、JSONC コメントだけを空白へ置き換える。
   閉じていないブロック コメントがある場合は、最後に開いた位置を返す。
   成功時は SIZE_MAX を返す。入力長は SIZE_MAX 未満であるため、位置と重ならない。 */
static size_t config_strip_comments(char *text)
{
    size_t i;
    size_t block_comment_start;
    int in_string = 0;
    int escaped = 0;
    int in_line_comment = 0;
    int in_block_comment = 0;

    block_comment_start = 0U;
    for (i = 0U; text[i] != '\0'; i++)
    {
        if (in_line_comment)
        {
            if (text[i] == '\n' || text[i] == '\r')
            {
                in_line_comment = 0;
            }
            else
            {
                text[i] = ' ';
            }
            continue;
        }
        if (in_block_comment)
        {
            if (text[i] == '*' && text[i + 1U] == '/')
            {
                text[i] = ' ';
                text[++i] = ' ';
                in_block_comment = 0;
            }
            else if (text[i] != '\n' && text[i] != '\r')
            {
                text[i] = ' ';
            }
            continue;
        }
        if (in_string)
        {
            if (escaped)
            {
                escaped = 0;
            }
            else if (text[i] == '\\')
            {
                escaped = 1;
            }
            else if (text[i] == '"')
            {
                in_string = 0;
            }
            continue;
        }
        if (text[i] == '"')
        {
            in_string = 1;
        }
        else if (text[i] == '/' && text[i + 1U] == '/')
        {
            text[i++] = ' ';
            text[i] = ' ';
            in_line_comment = 1;
        }
        else if (text[i] == '/' && text[i + 1U] == '*')
        {
            block_comment_start = i;
            text[i++] = ' ';
            text[i] = ' ';
            in_block_comment = 1;
        }
    }
    if (in_block_comment)
    {
        return block_comment_start;
    }
    return SIZE_MAX;
}

/* 閉じ括弧の直前にあるカンマを空白に置き換える。 */
static void config_strip_trailing_commas(char *text)
{
    size_t i;
    int in_string = 0;
    int escaped = 0;

    for (i = 0U; text[i] != '\0'; i++)
    {
        if (in_string)
        {
            if (escaped)
            {
                escaped = 0;
            }
            else if (text[i] == '\\')
            {
                escaped = 1;
            }
            else if (text[i] == '"')
            {
                in_string = 0;
            }
            continue;
        }
        if (text[i] == '"')
        {
            in_string = 1;
        }
        else if (text[i] == ',')
        {
            size_t next = i + 1U;
            size_t previous = i;
            while (text[next] == ' ' || text[next] == '\t' || text[next] == '\r' || text[next] == '\n')
            {
                next++;
            }
            while (previous > 0U && (text[previous - 1U] == ' ' || text[previous - 1U] == '\t' ||
                                     text[previous - 1U] == '\r' || text[previous - 1U] == '\n'))
            {
                previous--;
            }
            if ((text[next] == '}' || text[next] == ']') && previous > 0U && text[previous - 1U] != '{' &&
                text[previous - 1U] != '[')
            {
                text[i] = ' ';
            }
        }
    }
}

static size_t jsonc_syntax_offset(const char *normalized, const char *end, size_t buffer_length)
{
    size_t raw;

    if (end == NULL || end < normalized)
    {
        return buffer_length;
    }
    raw = (size_t)(end - normalized);
    if (raw >= buffer_length)
    {
        return buffer_length;
    }
    return raw;
}

