#include <testfw.h>

#include <cJSON.h>
#include <cJSON_Utils.h>

class cjsonTest : public Test
{
};

// JSON のパース結果が元の値と一致することの確認
TEST_F(cjsonTest, parse_and_print_roundtrip)
{
    // Arrange
    const char *input =
        "{\"name\":\"cjson\",\"version\":1}"; // [状態] - name が "cjson"、version が 1 の JSON 文字列を用意する。

    // Pre-Assert

    // Act
    cJSON *root = cJSON_Parse(input); // [手順] - cJSON_Parse(input) で JSON 文字列をパースする。

    // Assert
    ASSERT_NE(nullptr, root); // [確認_正常系] - cJSON_Parse の戻り値が NULL でないこと。
    cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
    ASSERT_NE(nullptr, name);                          // [確認_正常系] - name フィールドが存在すること。
    EXPECT_STREQ("cjson", cJSON_GetStringValue(name)); // [確認_正常系] - name フィールドの値が "cjson" であること。
    cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "version");
    ASSERT_NE(nullptr, version);     // [確認_正常系] - version フィールドが存在すること。
    EXPECT_EQ(1, version->valueint); // [確認_正常系] - version フィールドの値が 1 であること。

    // Cleanup
    cJSON_Delete(root);
}

// 不正な JSON のパースが NULL を返すことの確認
TEST_F(cjsonTest, parse_invalid_json_returns_null)
{
    // Arrange
    const char *input = "{invalid"; // [状態] - 不正な JSON 文字列 "{invalid" を用意する。

    // Pre-Assert

    // Act
    cJSON *root = cJSON_Parse(input); // [手順] - cJSON_Parse(input) に不正な JSON 文字列を渡す。

    // Assert
    EXPECT_EQ(nullptr, root); // [確認_異常系] - cJSON_Parse の戻り値が NULL であること。
}

// MergePatch が patch を適用することの確認
TEST_F(cjsonTest, utils_merge_patch_applies_patch)
{
    // Arrange
    cJSON *target = cJSON_Parse(
        "{\"name\":\"cjson\",\"version\":1}");     // [状態] - name が "cjson"、version が 1 の target を用意する。
    cJSON *patch = cJSON_Parse("{\"version\":2}"); // [状態] - version が 2 の patch を用意する。

    // Pre-Assert

    // Act
    cJSON *merged = cJSONUtils_MergePatch(
        target, patch); // [手順] - cJSONUtils_MergePatch(target, patch) を呼び出す (RFC 7396 準拠のマージ)。

    // Assert
    ASSERT_NE(nullptr, merged); // [確認_正常系] - cJSONUtils_MergePatch の戻り値が NULL でないこと。
    cJSON *version = cJSON_GetObjectItemCaseSensitive(merged, "version");
    ASSERT_NE(nullptr, version);     // [確認_正常系] - マージ後の version フィールドが存在すること。
    EXPECT_EQ(2, version->valueint); // [確認_正常系] - マージ後の version フィールドの値が 2 であること。

    // Cleanup
    cJSON_Delete(merged);
    cJSON_Delete(patch);
}
