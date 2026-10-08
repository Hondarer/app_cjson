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

#include <stdint.h>
#include <string.h>

/* 文字列リテラルを保持し、JSONC コメントだけを空白へ置き換える。 */
static int config_strip_comments(char *text)
{
    size_t i;
    int in_string = 0;
    int escaped = 0;
    int in_line_comment = 0;
    int in_block_comment = 0;

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
            text[i++] = ' ';
            text[i] = ' ';
            in_block_comment = 1;
        }
    }
    return !in_block_comment;
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

/* Doxygen コメントは、ヘッダーに記載 */
CJSON_PUBLIC(cJSON *) cJSON_ParseJSONCWithLength(const char *value, size_t buffer_length)
{
    char *normalized;
    cJSON *result;
    volatile unsigned char *bytes;
    size_t i;

    if (value == NULL || buffer_length == SIZE_MAX || memchr(value, '\0', buffer_length) != NULL)
    {
        return NULL;
    }
    normalized = (char *)cJSON_malloc(buffer_length + 1U);
    if (normalized == NULL)
    {
        return NULL;
    }
    memcpy(normalized, value, buffer_length);
    normalized[buffer_length] = '\0';
    if (!config_strip_comments(normalized))
    {
        result = NULL;
    }
    else
    {
        config_strip_trailing_commas(normalized);
        result = cJSON_ParseWithLengthOpts(normalized, buffer_length + 1U, NULL, 1);
    }
    bytes = (volatile unsigned char *)normalized;
    for (i = 0U; i < buffer_length + 1U; i++)
    {
        bytes[i] = 0U;
    }
    cJSON_free(normalized);
    return result;
}
