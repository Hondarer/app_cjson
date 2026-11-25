/**
 *******************************************************************************
 *  @file           cjson_app_thread_local.h
 *  @brief          libcjson_app の内部で使うスレッド ローカル記憶域の指定子を定義します。
 *  @author         Tetsuo Honda
 *  @date           2026/11/26
 *  @version        1.0.0
 *
 *  本ヘッダーは libcjson_app の内部だけで使用します。\n
 *  パッチを当てた `cJSON.c` の誤りの位置 (`global_error`) と、手書きの
 *  `prod/libsrc/cjson/jsonc/` 配下の実装が、この定義を共有します。
 *
 *  対象の処理系は MSVC と GCC だけです。ほかの処理系では
 *  コンパイルを停止します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef CJSON_APP_THREAD_LOCAL_H
#define CJSON_APP_THREAD_LOCAL_H

/**
 *  @brief          静的記憶域の変数をスレッド ローカルにする指定子です。
 *
 *  `static` と組み合わせて使います。各スレッドは、初期値から始まる独立した
 *  変数を持ちます。\n
 *  MSVC の `__declspec(thread)` は、`LoadLibrary` で動的にロードした DLL でも
 *  使えます (Windows Vista 以降)。
 *  see: https://learn.microsoft.com/en-us/cpp/cpp/thread
 */
#if defined(_MSC_VER)
    #define CJSON_APP_THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__)
    #define CJSON_APP_THREAD_LOCAL __thread
#else
    #error libcjson_app のスレッド ローカル記憶域は、MSVC と GCC だけを対象にします。
#endif

#endif /* CJSON_APP_THREAD_LOCAL_H */
