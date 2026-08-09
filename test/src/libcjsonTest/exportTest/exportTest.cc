#include <testfw.h>

#include <cJSON.h>
#include <cJSON_Utils.h>

#include <set>
#include <string>
#include <type_traits>

#define MOCK_CJSON_RET(return_type, name, parameters, arguments, matchers) \
    using expected_##name##_fn = return_type(CJSON_STDCALL *) parameters; \
    static_assert(std::is_same<decltype(&name), expected_##name##_fn>::value, #name " のシグネチャが不一致です");
#define MOCK_CJSON_VOID(return_type, name, parameters, arguments, matchers) \
    using expected_##name##_fn = return_type(CJSON_STDCALL *) parameters; \
    static_assert(std::is_same<decltype(&name), expected_##name##_fn>::value, #name " のシグネチャが不一致です");
#include <mock_cjson_api_table.h>
#undef MOCK_CJSON_VOID
#undef MOCK_CJSON_RET

static const char *const kExpectedExportNames[] = {
#define MOCK_CJSON_RET(return_type, name, parameters, arguments, matchers)  #name,
#define MOCK_CJSON_VOID(return_type, name, parameters, arguments, matchers) #name,
#include <mock_cjson_api_table.h>
#undef MOCK_CJSON_VOID
#undef MOCK_CJSON_RET
};

// libcjson の公開関数と mock_cjson の API 表が一致することの確認
TEST(exportTest, cjson_symbols_match_api_table)
{
    // Arrange
    std::set<std::string> expected(
        std::begin(kExpectedExportNames),
        std::end(kExpectedExportNames)); // [状態] - mock_cjson の API 表から期待する公開関数名を構築する。
    std::string path = findWorkspaceRoot() + "/app/cjson/prod/lib/libcjson" +
                       TESTFW_SHARED_LIBRARY_EXTENSION; // [状態] - 検査対象を libcjson の動的ライブラリとする。

    // Pre-Assert

    // Act
    std::set<std::string> all_actual =
        testing::getActualExportNames(path); // [手順] - libcjson のエクスポート名を取得する。
    std::set<std::string> actual;
    for (const std::string &name : all_actual)
    {
        if (name.rfind("cJSON", 0u) == 0u)
        {
            actual.insert(name);
        }
    }

    // Assert
    EXPECT_EQ(expected, actual);   // [確認_正常系] - libcjson の全公開関数名が mock_cjson の API 表と一致すること。
    EXPECT_EQ(92u, actual.size()); // [確認_正常系] - libcjson の公開関数数が 92 であること。
}
