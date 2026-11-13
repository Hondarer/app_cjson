#ifndef MOCK_OVERRIDE_CJSON_CJSON_JSONC_H
#define MOCK_OVERRIDE_CJSON_CJSON_JSONC_H

#if !defined(MOCK_CJSON_ENABLE_CALLER) || defined(MOCK_CJSON_IN_OVERRIDE_REAL_HEADERS)
    #if defined(_WIN32)
        /* see: https://learn.microsoft.com/en-us/cpp/preprocessor/warning */
        #pragma warning(push, 0)
        #include "../../prod/include/cJSON_JSONC.h"
        #pragma warning(pop)
    #else
        #include_next <cJSON_JSONC.h>
    #endif
#else
    #define MOCK_CJSON_IN_OVERRIDE_REAL_HEADERS
    #if defined(_WIN32)
        /* see: https://learn.microsoft.com/en-us/cpp/preprocessor/warning */
        #pragma warning(push, 0)
        #include "../../prod/include/cJSON.h"
        #include "../../prod/include/cJSON_Integer.h"
        #include "../../prod/include/cJSON_JSONC.h"
        #include "../../prod/include/cJSON_Utils.h"
        #pragma warning(pop)
    #else
        #include_next <cJSON.h>
        #include_next <cJSON_Integer.h>
        #include_next <cJSON_JSONC.h>
        #include_next <cJSON_Utils.h>
    #endif
    #undef MOCK_CJSON_IN_OVERRIDE_REAL_HEADERS
    #define MOCK_CJSON_INJECT_CALLER
    #include <mock_cjson_overrides.h>
    #undef MOCK_CJSON_INJECT_CALLER
#endif

#endif /* MOCK_OVERRIDE_CJSON_CJSON_JSONC_H */
