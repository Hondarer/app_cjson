#include <testfw.h>

#include <cJSON.h>
#include <cJSON_Integer.h>
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

// libcjson の期待シンボルと実ライブラリの全エクスポートが一致することの確認
TEST(exportTest, cjson_symbols_match_api_table)
{
    // Arrange
    std::set<std::string> expected(
        std::begin(kExpectedExportNames),
        std::end(kExpectedExportNames)); // [状態] - mock_cjson の API 表から期待する公開関数名を構築する。
#if defined(PLATFORM_WINDOWS)
    expected.insert(testing::identManifestSymbolName(
        "libcjson" TESTFW_SHARED_LIBRARY_EXTENSION)); // [状態] - IDENT manifest シンボル名を期待値へ追加する。
#endif                                                /* PLATFORM_WINDOWS */
    std::string path = findWorkspaceRoot() + "/app/cjson/prod/lib/libcjson" +
                       TESTFW_SHARED_LIBRARY_EXTENSION; // [状態] - 検査対象を libcjson の動的ライブラリとする。

    // Pre-Assert

    // Act
    std::set<std::string> actual =
        testing::getActualExportNames(path); // [手順] - libcjson のエクスポート名を取得する。

    // Assert
    testing::expectExportNamesMatch(expected,
                                    actual); // [確認_正常系] - libcjson のエクスポートに不足や想定外がないこと。
}
