#include <testfw.h>
#include <mock_cjson.h>
#include <cstring>

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

// 実オブジェクトを生成せずに Parse / GetStringValue / Delete を単体隔離できることの確認
TEST(mockCjsonTest, isolates_parse_and_string_value_without_real_object)
{
    // Arrange
    NiceMock<Mock_cjson> mock_cjson;
    cJSON item = {};
    item.valuestring = const_cast<char *>("name"); // [状態] - valuestring が name の cJSON オブジェクトを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_cjson, cJSON_Parse(StrEq("{}")))
        .WillOnce(Return(&item)); // [Pre-Assert確認_正常系] - cJSON_Parse が {} を指定して 1 回呼び出されること。
                                  // [Pre-Assert手順] - cJSON_Parse から item のアドレスを返却する。
    EXPECT_CALL(mock_cjson, cJSON_GetStringValue(&item))
        .WillOnce(Return(
            item.valuestring)); // [Pre-Assert確認_正常系] - cJSON_GetStringValue が item を指定して 1 回呼び出されること。
                                // [Pre-Assert手順] - cJSON_GetStringValue から name を返却する。
    EXPECT_CALL(mock_cjson, cJSON_Delete(&item))
        .WillOnce(Return()); // [Pre-Assert確認_正常系] - cJSON_Delete が item を指定して 1 回呼び出されること。
                             // [Pre-Assert手順] - cJSON_Delete から直ちに戻る。

    // Act
    cJSON *actual = cJSON_Parse("{}");          // [手順] - cJSON_Parse を呼び出す。
    char *value = cJSON_GetStringValue(actual); // [手順] - cJSON_GetStringValue で文字列を取得する。
    cJSON_Delete(actual);                       // [手順] - cJSON_Delete でオブジェクトを解放する。

    // Assert
    EXPECT_EQ(&item, actual);    // [確認_正常系] - cJSON_Parse の戻り値が item のアドレスであること。
    EXPECT_STREQ("name", value); // [確認_正常系] - cJSON_GetStringValue の戻り値が name であること。
}

// JSONC 拡張の実委譲がコメントと末尾カンマを解析し、文字列を保持することの確認
TEST(mockCjsonTest, delegates_jsonc_parser)
{
    const char *source = "/* header */ {\"name\":\"a,}\",\"items\":[1, /"
                         "/ item\n],}";

    cJSON *root = cJSON_ParseJSONCWithLength(source, std::strlen(source)); // [手順] - JSONC を実関数で解析する。

    ASSERT_NE(nullptr, root); // [確認_正常系] - 末尾カンマを含む JSONC が解析できること。
    EXPECT_STREQ("a,}", cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(root, "name")));
    // [確認_正常系] - `cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(root, "name"))` の戻り値が `"a,}"` であること。
    EXPECT_EQ(1, cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(root, "items")));
    // [確認_正常系] - `cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(root, "items"))` の戻り値が `1` であること。
    cJSON_Delete(root);
}

// JSONC 拡張の不正なカンマと閉じていないコメントが拒否されることの確認
TEST(mockCjsonTest, rejects_invalid_jsonc)
{
    const char *empty_member = "{,}";
    const char *missing_value = "{\"value\":,}";
    const char *unclosed_comment = "{/" "* comment";

    EXPECT_EQ(nullptr, cJSON_ParseJSONCWithLength(empty_member, std::strlen(empty_member)));
    // [確認_異常系] - `cJSON_ParseJSONCWithLength(empty_member, std::strlen(empty_member))` の戻り値が `nullptr` であること。
    EXPECT_EQ(nullptr, cJSON_ParseJSONCWithLength(missing_value, std::strlen(missing_value)));
    // [確認_異常系] - `cJSON_ParseJSONCWithLength(missing_value, std::strlen(missing_value))` の戻り値が `nullptr` であること。
    EXPECT_EQ(nullptr, cJSON_ParseJSONCWithLength(unclosed_comment, std::strlen(unclosed_comment)));
    // [確認_異常系] - `cJSON_ParseJSONCWithLength(unclosed_comment, std::strlen(unclosed_comment))` の戻り値が `nullptr` であること。
    EXPECT_EQ(nullptr, cJSON_ParseJSONCWithLength(nullptr, 0U));
    // [確認_異常系] - `cJSON_ParseJSONCWithLength(nullptr, 0U)` の戻り値が `nullptr` であること。
}

// JSONC 拡張の呼び出し結果を mock で差し替えられることの確認
TEST(mockCjsonTest, overrides_jsonc_parser)
{
    NiceMock<Mock_cjson> mock_cjson;
    cJSON expected = {};
    const char *source = "{}";
    EXPECT_CALL(mock_cjson, cJSON_ParseJSONCWithLength(StrEq(source), std::strlen(source)))
        .WillOnce(Return(&expected));
    // [Pre-Assert確認_正常系] - mock_cjson の cJSON_ParseJSONCWithLength(StrEq(source), std::strlen(source)) が登録した呼び出し期待を満たすこと。

    cJSON *actual = cJSON_ParseJSONCWithLength(source, std::strlen(source)); // [手順] - mock を介して JSONC を解析する。

    EXPECT_EQ(&expected, actual); // [確認_正常系] - JSONC 解析結果を差し替えられること。
}
