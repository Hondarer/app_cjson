/**
 *******************************************************************************
 *  @file           cJSON_PatchCheck.h
 *  @brief          cJSON パッチ 0003 の適用をビルド時に検査します。
 *  @author         Tetsuo Honda
 *  @date           2026/11/28
 *  @version        1.0.0
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef CJSON_PATCH_CHECK_H
#define CJSON_PATCH_CHECK_H

#include <assert.h>
#include <cJSON.h>

#ifndef CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR
    #error "cJSON にパッチ 0003 (global_error のスレッド ローカル化) が当たっていません"
#else
static_assert(CJSON_APP_PATCH_THREAD_LOCAL_GLOBAL_ERROR == 1,
              "cJSON のパッチ 0003 (global_error のスレッド ローカル化) の版が一致しません");
#endif

#endif /* CJSON_PATCH_CHECK_H */
