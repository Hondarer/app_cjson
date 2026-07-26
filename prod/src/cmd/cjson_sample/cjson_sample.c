/**
 *******************************************************************************
 *  @file           src/cmd/cjson_sample/cjson_sample.c
 *  @brief          cJSON を使用して JSON オブジェクトを組み立て、文字列化するコマンドを実装します。
 *  @author         Tetsuo Honda
 *  @date           2026/07/26
 *  @version        1.0.0
 *
 *  オブジェクトへ name / version フィールドを追加し、cJSON_Print で
 *  シリアライズした JSON 文字列を標準出力に出力します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <stdio.h>

#include <cJSON.h>

/**
 *  @brief          プログラムのエントリ ポイント。
 *  @return         常に 0 を返します。
 */
int main(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "name", "cjson_sample");
    cJSON_AddNumberToObject(root, "version", 1);

    char *text = cJSON_Print(root);
    if (text != NULL)
    {
        printf("%s\n", text);
        cJSON_free(text);
    }

    cJSON_Delete(root);

    return 0;
}
