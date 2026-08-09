#include <testfw.h>
#include <mock_cjson.h>

// Mock_cjson を注入しない呼び出しが cJSON と cJSON_Utils の実関数へ委譲されることの確認
TEST(mockCjsonTest, delegates_to_real_without_mock)
{
    // Arrange
    const char *json = "{\"items\":[1,2]}"; // [状態] - items 配列を持つ JSON 文字列を用意する。

    // Pre-Assert

    // Act
    cJSON *root = cJSON_Parse(json);                      // [手順] - Mock_cjson を注入せず cJSON_Parse を呼び出す。
    cJSON *items = cJSONUtils_GetPointer(root, "/items"); // [手順] - cJSONUtils_GetPointer で items を取得する。
    int size = cJSON_GetArraySize(items);                 // [手順] - items の配列要素数を取得する。

    // Assert
    ASSERT_NE(nullptr, root);  // [確認_正常系] - cJSON_Parse の戻り値が NULL でないこと。
    ASSERT_NE(nullptr, items); // [確認_正常系] - cJSONUtils_GetPointer の戻り値が NULL でないこと。
    EXPECT_EQ(2, size);        // [確認_正常系] - cJSON_GetArraySize の戻り値が 2 であること。

    // Cleanup
    cJSON_Delete(root);
}

// 注入済み Mock_cjson の未設定呼び出しが実関数へ委譲されることの確認
TEST(mockCjsonTest, delegates_to_real_with_default_action)
{
    // Arrange
    NiceMock<Mock_cjson> mock_cjson;
    const char *json = "{\"name\":\"cjson\"}"; // [状態] - name が cjson の JSON 文字列を用意する。

    // Pre-Assert

    // Act
    cJSON *root = cJSON_Parse(json); // [手順] - 既定動作の Mock_cjson を介して cJSON_Parse を呼び出す。

    // Assert
    ASSERT_NE(nullptr, root); // [確認_正常系] - cJSON_Parse の戻り値が NULL でないこと。

    // Cleanup
    cJSON_Delete(root);
}

// EXPECT_CALL により cJSON の戻り値を上書きできることの確認
TEST(mockCjsonTest, overrides_result)
{
    // Arrange
    NiceMock<Mock_cjson> mock_cjson;
    cJSON expected = {};
    const char *json = "{}"; // [状態] - cJSON_Parse の戻り値として使用する cJSON オブジェクトを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_cjson, cJSON_Parse(StrEq(json)))
        .WillOnce(
            Return(&expected)); // [Pre-Assert確認_正常系] - cJSON_Parse が JSON 文字列を指定して 1 回呼び出されること。
                                // [Pre-Assert手順] - cJSON_Parse から expected のアドレスを返却する。

    // Act
    cJSON *actual = cJSON_Parse(json); // [手順] - 戻り値を設定した Mock_cjson を介して cJSON_Parse を呼び出す。

    // Assert
    EXPECT_EQ(&expected, actual); // [確認_正常系] - cJSON_Parse の戻り値が expected のアドレスであること。
}
