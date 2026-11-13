#ifndef MOCK_CJSON_H
#define MOCK_CJSON_H

#include <cJSON.h>
#include <cJSON_Integer.h>
#include <cJSON_JSONC.h>
#include <cJSON_Utils.h>
#include <testfw.h>
#include <mock_cjson_overrides.h>

inline constexpr char kLibCjsonName[] = "libcjson_app" TESTFW_SHARED_LIBRARY_EXTENSION;

#define MOCK_CJSON_RET(return_type, original_name, api_name, parameters, arguments, mock_parameters, mock_arguments, matchers) \
    extern return_type delegate_real_##api_name mock_parameters;
#define MOCK_CJSON_VOID(return_type, original_name, api_name, parameters, arguments, mock_parameters, mock_arguments, matchers) \
    extern return_type delegate_real_##api_name mock_parameters;
#include <mock_cjson_api_table.h>
#undef MOCK_CJSON_VOID
#undef MOCK_CJSON_RET

class Mock_cjson
{
  public:
#define MOCK_CJSON_RET(return_type, original_name, api_name, parameters, arguments, mock_parameters, mock_arguments, matchers)  MOCK_METHOD(return_type, api_name, mock_parameters);
#define MOCK_CJSON_VOID(return_type, original_name, api_name, parameters, arguments, mock_parameters, mock_arguments, matchers) MOCK_METHOD(return_type, api_name, mock_parameters);
#include <mock_cjson_api_table.h>
#undef MOCK_CJSON_VOID
#undef MOCK_CJSON_RET

    Mock_cjson();
    virtual ~Mock_cjson();
};

extern Mock_cjson *_mock_cjson;

#endif /* MOCK_CJSON_H */
