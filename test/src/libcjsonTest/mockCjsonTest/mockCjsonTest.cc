#include <testfw.h>
#include <mock_cjson.h>
#include <cstring>

extern "C"
{
const char *namespace_test_caller_file(void);
int namespace_test_caller_line(void);
cJSON *namespace_test_direct_call(const char *json);
cJSON *namespace_test_pointer_call(const char *json);
}
const char *namespace_test_cpp_caller_file();
int namespace_test_cpp_caller_line();
cJSON *namespace_test_cpp_direct_call(const char *json);

// Mock_cjson を注入しない呼び出しが cJSON と cJSON_Utils の処理へ委譲されることの確認
TEST(mockCjsonTest, delegates_to_real_without_mock)
{
    // Arrange
    const char *json = "{\"items\":[1,2]}"; // [状態] - items 配列を持つ JSON 文字列を用意する。

    // Pre-Assert

    // Act
    cJSON *root = cjson_app_cJSON_Parse(json); // [手順] - Mock_cjson を注入せず json を指定して cjson_app_cJSON_Parse() を呼び出す。
    cJSON *items = cjson_app_cJSONUtils_GetPointer(
        root, "/items");                  // [手順] - root と "/items" を指定して cjson_app_cJSONUtils_GetPointer() を呼び出す。
    int size = cjson_app_cJSON_GetArraySize(items); // [手順] - items を指定して cjson_app_cJSON_GetArraySize() を呼び出す。

    // Assert
    ASSERT_NE(nullptr, root);  // [確認_正常系] - cjson_app_cJSON_Parse() の戻り値が nullptr 以外であること。
    ASSERT_NE(nullptr, items); // [確認_正常系] - cjson_app_cJSONUtils_GetPointer() の戻り値が nullptr 以外であること。
    EXPECT_EQ(2, size);        // [確認_正常系] - cjson_app_cJSON_GetArraySize() の戻り値が 2 であること。

    // Cleanup
    cjson_app_cJSON_Delete(root);
}

// 注入済み Mock_cjson の未設定呼び出しが cJSON の処理へ委譲されることの確認
TEST(mockCjsonTest, delegates_to_real_with_default_action)
{
    // Arrange
    NiceMock<Mock_cjson> mock_cjson;
    const char *json = "{\"name\":\"cjson\"}"; // [状態] - name が cjson の JSON 文字列を用意する。

    // Pre-Assert

    // Act
    cJSON *root =
        cjson_app_cJSON_Parse(json); // [手順] - 既定動作の Mock_cjson を介して json を指定して cjson_app_cJSON_Parse() を呼び出す。

    // Assert
    ASSERT_NE(nullptr, root); // [確認_正常系] - cjson_app_cJSON_Parse() の戻り値が nullptr 以外であること。

    // Cleanup
    cjson_app_cJSON_Delete(root);
}

// EXPECT_CALL により cJSON の戻り値を上書きできることの確認
TEST(mockCjsonTest, overrides_result)
{
    // Arrange
    NiceMock<Mock_cjson> mock_cjson;
    cJSON expected = {};
    const char *json = "{}"; // [状態] - cjson_app_cJSON_Parse() の戻り値として使用する cJSON オブジェクトを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_cjson, cjson_app_cJSON_Parse(_, _, _, StrEq(json)))
        .WillOnce(Return(
            &expected)); // [Pre-Assert確認_正常系] - cjson_app_cJSON_Parse() が json と等しい文字列を指定して 1 回呼び出されること。
                         // [Pre-Assert手順] - cjson_app_cJSON_Parse() から expected のアドレスを返す。

    // Act
    cJSON *actual =
        cjson_app_cJSON_Parse(json); // [手順] - 戻り値を設定した Mock_cjson を介して json を指定して cjson_app_cJSON_Parse() を呼び出す。

    // Assert
    EXPECT_EQ(&expected, actual); // [確認_正常系] - cjson_app_cJSON_Parse() の戻り値が &expected と一致すること。
}

TEST(mockCjsonTest, injects_c_callsite_and_marks_function_pointer_as_unknown)
{
    // Arrange
    NiceMock<Mock_cjson> mock_cjson;
    cJSON expected = {};
    const char *json = "{}";

    // Pre-Assert
    EXPECT_CALL(mock_cjson, cjson_app_cJSON_Parse(StrEq(namespace_test_caller_file()), namespace_test_caller_line(),
                                                 StrEq("namespace_test_direct_call"), StrEq(json)))
        .WillOnce(Return(&expected)); // [Pre-Assert確認_正常系] - C 呼び出し元の情報を伴って cjson_app_cJSON_Parse を呼び出す。
    EXPECT_CALL(mock_cjson, cjson_app_cJSON_Parse(StrEq(namespace_test_cpp_caller_file()), namespace_test_cpp_caller_line(),
                                                 StrEq("namespace_test_cpp_direct_call"), StrEq(json)))
        .WillOnce(Return(&expected)); // [Pre-Assert確認_正常系] - C++ 呼び出し元の情報を伴って cjson_app_cJSON_Parse を呼び出す。
    EXPECT_CALL(mock_cjson, cjson_app_cJSON_Parse(StrEq("<unknown>"), 0, StrEq("<unknown>"), StrEq(json)))
        .WillOnce(Return(&expected)); // [Pre-Assert確認_正常系] - 関数ポインター経由の取得不能値を受け取る。

    // Act
    cJSON *direct_result = namespace_test_direct_call(json);
    cJSON *cpp_direct_result = namespace_test_cpp_direct_call(json);
    cJSON *pointer_result = namespace_test_pointer_call(json);

    // Assert
    EXPECT_EQ(&expected, direct_result); // [確認_正常系] - C 直接呼び出しの mock 戻り値を受け取る。
    EXPECT_EQ(&expected, cpp_direct_result); // [確認_正常系] - C++ 直接呼び出しの mock 戻り値を受け取る。
    EXPECT_EQ(&expected, pointer_result); // [確認_正常系] - 関数ポインター呼び出しの mock 戻り値を受け取る。
}

// 実オブジェクトを生成せずに Parse / GetStringValue / Delete を単体隔離できることの確認
TEST(mockCjsonTest, isolates_parse_and_string_value_without_real_object)
{
    // Arrange
    NiceMock<Mock_cjson> mock_cjson;
    cJSON item = {};
    item.valuestring = const_cast<char *>("name"); // [状態] - valuestring が name の cJSON オブジェクトを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_cjson, cjson_app_cJSON_Parse(_, _, _, StrEq("{}")))
        .WillOnce(Return(
            &item)); // [Pre-Assert確認_正常系] - cjson_app_cJSON_Parse() が "{}" と等しい文字列を指定して 1 回呼び出されること。
                     // [Pre-Assert手順] - cjson_app_cJSON_Parse() から item のアドレスを返す。
    EXPECT_CALL(mock_cjson, cjson_app_cJSON_GetStringValue(_, _, _, &item))
        .WillOnce(Return(
            item.valuestring)); // [Pre-Assert確認_正常系] - cjson_app_cJSON_GetStringValue() が &item を指定して 1 回呼び出されること。
                                // [Pre-Assert手順] - cjson_app_cJSON_GetStringValue() から name を返す。
    EXPECT_CALL(mock_cjson, cjson_app_cJSON_Delete(_, _, _, &item))
        .WillOnce(Return()); // [Pre-Assert確認_正常系] - cjson_app_cJSON_Delete() が &item を指定して 1 回呼び出されること。
                             // [Pre-Assert手順] - cjson_app_cJSON_Delete() から戻る。

    // Act
    cJSON *actual = cjson_app_cJSON_Parse("{}");          // [手順] - "{}" を指定して cjson_app_cJSON_Parse() を呼び出す。
    char *value = cjson_app_cJSON_GetStringValue(actual); // [手順] - actual を指定して cjson_app_cJSON_GetStringValue() を呼び出す。
    cjson_app_cJSON_Delete(actual);                       // [手順] - actual を指定して cjson_app_cJSON_Delete() を呼び出す。

    // Assert
    EXPECT_EQ(&item, actual);    // [確認_正常系] - cjson_app_cJSON_Parse() の戻り値が &item と一致すること。
    EXPECT_STREQ("name", value); // [確認_正常系] - cjson_app_cJSON_GetStringValue() の戻り値が "name" であること。
}

// JSONC 拡張の実委譲がコメントと末尾カンマを解析し、文字列を保持することの確認
TEST(mockCjsonTest, delegates_jsonc_parser)
{
    // Arrange
    const char *source = "/* header */ {\"name\":\"a,}\",\"items\":[1, /"
                         "/ item\n],}"; // [状態] - コメントと末尾カンマを含む JSONC 文字列を用意する。

    // Pre-Assert

    // Act
    cJSON *root = cjson_app_cJSON_ParseJSONCWithLength(
        source,
        std::strlen(source)); // [手順] - JSONC 文字列 source を指定して cjson_app_cJSON_ParseJSONCWithLength() を呼び出す。

    // Assert
    ASSERT_NE(nullptr, root); // [確認_正常系] - cjson_app_cJSON_ParseJSONCWithLength() の戻り値が nullptr 以外であること。
    EXPECT_STREQ(
        "a,}",
        cjson_app_cJSON_GetStringValue(cjson_app_cJSON_GetObjectItemCaseSensitive(
            root,
            "name"))); // [確認_正常系] - "name" の項目に対する cjson_app_cJSON_GetStringValue() の戻り値が "a,}" であること。
    EXPECT_EQ(
        1,
        cjson_app_cJSON_GetArraySize(cjson_app_cJSON_GetObjectItemCaseSensitive(
            root, "items"))); // [確認_正常系] - "items" の項目に対する cjson_app_cJSON_GetArraySize() の戻り値が 1 であること。

    // Cleanup
    cjson_app_cJSON_Delete(root);
}

// JSONC 拡張の不正なカンマと閉じていないコメントが拒否されることの確認
TEST(mockCjsonTest, rejects_invalid_jsonc)
{
    // Arrange
    const char *empty_member = "{,}";            // [状態] - 空のメンバーを持つ不正な JSONC を用意する。
    const char *missing_value = "{\"value\":,}"; // [状態] - 値が欠落した不正な JSONC を用意する。
    const char *unclosed_comment = "{/"
                                   "* comment"; // [状態] - 閉じていないコメントを持つ不正な JSONC を用意する。

    // Pre-Assert

    // Act
    cJSON *actual_empty = cjson_app_cJSON_ParseJSONCWithLength(
        empty_member,
        std::strlen(empty_member)); // [手順] - 空メンバーの JSONC を指定して cjson_app_cJSON_ParseJSONCWithLength() を呼び出す。
    cJSON *actual_missing = cjson_app_cJSON_ParseJSONCWithLength(
        missing_value,
        std::strlen(missing_value)); // [手順] - 値欠落の JSONC を指定して cjson_app_cJSON_ParseJSONCWithLength() を呼び出す。
    cJSON *actual_unclosed = cjson_app_cJSON_ParseJSONCWithLength(
        unclosed_comment,
        std::strlen(
            unclosed_comment)); // [手順] - 閉じていないコメントの JSONC を指定して cjson_app_cJSON_ParseJSONCWithLength() を呼び出す。
    cJSON *actual_null = cjson_app_cJSON_ParseJSONCWithLength(
        nullptr, 0U); // [手順] - nullptr の入力を指定して cjson_app_cJSON_ParseJSONCWithLength() を呼び出す。

    // Assert
    EXPECT_EQ(
        nullptr,
        actual_empty); // [確認_異常系] - 空メンバーの入力に対する cjson_app_cJSON_ParseJSONCWithLength() の戻り値が nullptr であること。
    EXPECT_EQ(
        nullptr,
        actual_missing); // [確認_異常系] - 値欠落の入力に対する cjson_app_cJSON_ParseJSONCWithLength() の戻り値が nullptr であること。
    EXPECT_EQ(
        nullptr,
        actual_unclosed); // [確認_異常系] - 閉じていないコメントの入力に対する cjson_app_cJSON_ParseJSONCWithLength() の戻り値が nullptr であること。
    EXPECT_EQ(
        nullptr,
        actual_null); // [確認_異常系] - nullptr の入力に対する cjson_app_cJSON_ParseJSONCWithLength() の戻り値が nullptr であること。
}

// JSONC 拡張の呼び出し結果を mock で差し替えられることの確認
TEST(mockCjsonTest, overrides_jsonc_parser)
{
    // Arrange
    NiceMock<Mock_cjson> mock_cjson;
    cJSON expected = {};
    const char *source = "{}"; // [状態] - 解析対象文字列と期待値を準備する。

    // Pre-Assert
    EXPECT_CALL(mock_cjson, cjson_app_cJSON_ParseJSONCWithLength(_, _, _, StrEq(source), std::strlen(source)))
        .WillOnce(Return(
            &expected)); // [Pre-Assert確認_正常系] - cjson_app_cJSON_ParseJSONCWithLength() が source と等しい文字列、std::strlen() の戻り値を指定して 1 回呼び出されること。
                         // [Pre-Assert手順] - cjson_app_cJSON_ParseJSONCWithLength() から expected のアドレスを返す。

    // Act
    cJSON *actual = cjson_app_cJSON_ParseJSONCWithLength(
        source,
        std::strlen(
            source)); // [手順] - mock を介して JSONC 文字列 source を指定して cjson_app_cJSON_ParseJSONCWithLength() を呼び出す。

    // Assert
    EXPECT_EQ(&expected, actual); // [確認_正常系] - cjson_app_cJSON_ParseJSONCWithLength() の戻り値が &expected と一致すること。
}
