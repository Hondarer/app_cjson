#ifndef MOCK_CJSON_H
#define MOCK_CJSON_H

#include <cJSON.h>
#include <cJSON_Utils.h>
#include <testfw.h>

inline constexpr char kLibCjsonName[] = "libcjson" TESTFW_SHARED_LIBRARY_EXTENSION;

#define MOCK_CJSON_RET(return_type, name, parameters, arguments, matchers) \
    extern return_type delegate_real_##name parameters;
#define MOCK_CJSON_VOID(return_type, name, parameters, arguments, matchers) \
    extern return_type delegate_real_##name parameters;
#include <mock_cjson_api_table.h>
#undef MOCK_CJSON_VOID
#undef MOCK_CJSON_RET

class Mock_cjson
{
  public:
#define MOCK_CJSON_RET(return_type, name, parameters, arguments, matchers)  MOCK_METHOD(return_type, name, parameters);
#define MOCK_CJSON_VOID(return_type, name, parameters, arguments, matchers) MOCK_METHOD(return_type, name, parameters);
#include <mock_cjson_api_table.h>
#undef MOCK_CJSON_VOID
#undef MOCK_CJSON_RET

    Mock_cjson();
    ~Mock_cjson();
};

extern Mock_cjson *_mock_cjson;

#endif /* MOCK_CJSON_H */
