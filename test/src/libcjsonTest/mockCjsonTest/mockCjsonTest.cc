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
    // Arrange
    const char *source = "/* header */ {\"name\":\"a,}\",\"items\":[1, /"
                         "/ item\n],}"; // [状態] - コメントと末尾カンマを含む JSONC 文字列を用意する。

    // Pre-Assert

    // Act
    cJSON *root = cJSON_ParseJSONCWithLength(source, std::strlen(source)); // [手順] - JSONC を実関数で解析する。

    // Assert
    ASSERT_NE(nullptr, root); // [確認_正常系] - 末尾カンマを含む JSONC が解析できること。
    EXPECT_STREQ("a,}", cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(root, "name"))); // [確認_正常系] - 文字列が正しく取得できること。
    EXPECT_EQ(1, cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(root, "items"))); // [確認_正常系] - 配列要素数が 1 であること。

    // Cleanup
    cJSON_Delete(root);
}

// JSONC 拡張の不正なカンマと閉じていないコメントが拒否されることの確認
TEST(mockCjsonTest, rejects_invalid_jsonc)
{
    // Arrange
    const char *empty_member = "{,}"; // [状態] - 空のメンバーを持つ不正な JSONC を用意する。
    const char *missing_value = "{\"value\":,}"; // [状態] - 値が欠落した不正な JSONC を用意する。
    const char *unclosed_comment = "{/" "* comment"; // [状態] - 閉じていないコメントを持つ不正な JSONC を用意する。

    // Pre-Assert

    // Act
    cJSON *actual_empty = cJSON_ParseJSONCWithLength(empty_member, std::strlen(empty_member)); // [手順] - 空メンバーの JSONC を解析する。
    cJSON *actual_missing = cJSON_ParseJSONCWithLength(missing_value, std::strlen(missing_value)); // [手順] - 値欠落の JSONC を解析する。
    cJSON *actual_unclosed = cJSON_ParseJSONCWithLength(unclosed_comment, std::strlen(unclosed_comment)); // [手順] - 閉じていないコメントの JSONC を解析する。
    cJSON *actual_null = cJSON_ParseJSONCWithLength(nullptr, 0U); // [手順] - NULL 入力を解析する。

    // Assert
    EXPECT_EQ(nullptr, actual_empty); // [確認_異常系] - 空メンバーが拒否されること。
    EXPECT_EQ(nullptr, actual_missing); // [確認_異常系] - 値欠落が拒否されること。
    EXPECT_EQ(nullptr, actual_unclosed); // [確認_異常系] - 閉じていないコメントが拒否されること。
    EXPECT_EQ(nullptr, actual_null); // [確認_異常系] - NULL 入力が拒否されること。
}

// JSONC 拡張の呼び出し結果を mock で差し替えられることの確認
TEST(mockCjsonTest, overrides_jsonc_parser)
{
    // Arrange
    NiceMock<Mock_cjson> mock_cjson;
    cJSON expected = {};
    const char *source = "{}"; // [状態] - 解析対象文字列と期待値を準備する。

    // Pre-Assert
    EXPECT_CALL(mock_cjson, cJSON_ParseJSONCWithLength(StrEq(source), std::strlen(source)))
        .WillOnce(Return(&expected)); // [Pre-Assert確認_正常系] - mock の cJSON_ParseJSONCWithLength が 1 回呼び出されること。
                                      // [Pre-Assert手順] - expected のアドレスを返却する。

    // Act
    cJSON *actual = cJSON_ParseJSONCWithLength(source, std::strlen(source)); // [手順] - mock を介して JSONC を解析する。

    // Assert
    EXPECT_EQ(&expected, actual); // [確認_正常系] - JSONC 解析結果を差し替えられること。
}
