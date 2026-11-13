#include <cJSON.h>

const char *namespace_test_caller_file(void);
int namespace_test_caller_line(void);
cJSON *namespace_test_direct_call(const char *json);
cJSON *namespace_test_pointer_call(const char *json);

const char *namespace_test_caller_file(void)
{
    return __FILE__;
}
/* 呼び出し行の照合に使うため、次の 2 定義を隣接させます。 */
// clang-format off
int namespace_test_caller_line(void) { return __LINE__ + 1; }
cJSON *namespace_test_direct_call(const char *json) { return cjson_app_cJSON_Parse(json); }
// clang-format on
cJSON *namespace_test_pointer_call(const char *json)
{
    cJSON *(*api)(const char *) = &cjson_app_cJSON_Parse;
    return api(json);
}
