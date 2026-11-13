#include <cJSON.h>

const char *namespace_test_cpp_caller_file()
{
    return __FILE__;
}
/* 呼び出し行の照合に使うため、次の 2 定義を隣接させます。 */
// clang-format off
int namespace_test_cpp_caller_line() { return __LINE__ + 1; }
cJSON *namespace_test_cpp_direct_call(const char *json) { return cjson_app_cJSON_Parse(json); }
// clang-format on
