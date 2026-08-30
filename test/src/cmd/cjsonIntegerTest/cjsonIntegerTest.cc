#include <testfw.h>

#include <cJSON.h>
#include <cJSON_Integer.h>
#include <cJSON_Utils.h>

#include <cstdint>

class cjsonIntegerTest : public Test
{
};

namespace
{

// パースした JSON を無整形で出力し、std::string で返す補助関数。
std::string parseAndPrint(const char *input)
{
    cJSON *root = cJSON_Parse(input);
    if (root == nullptr)
    {
        return std::string("<parse failed>");
    }
    char *text = cJSON_PrintUnformatted(root);
    std::string result = (text != nullptr) ? std::string(text) : std::string("<print failed>");
    cJSON_free(text);
    cJSON_Delete(root);
    return result;
}

// 項目を無整形で出力し、std::string で返す補助関数。項目は解放する。
std::string printAndDelete(cJSON *item)
{
    if (item == nullptr)
    {
        return std::string("<null item>");
    }
    char *text = cJSON_PrintUnformatted(item);
    std::string result = (text != nullptr) ? std::string(text) : std::string("<print failed>");
    cJSON_free(text);
    cJSON_Delete(item);
    return result;
}

} // namespace

// double では表現できない整数がパースと出力を往復しても壊れないことの確認
TEST_F(cjsonIntegerTest, roundtrip_keeps_integers_beyond_double_precision)
{
    // Arrange
    const char *input = "[9007199254740993,-9223372036854775808,9223372036854775807,18446744073709551615]";
    // [状態] - 2^53+1、INT64_MIN、INT64_MAX、UINT64_MAX を含む JSON 配列を用意する。

    // Pre-Assert

    // Act
    std::string output = parseAndPrint(input); // [手順] - パースした結果を cJSON_PrintUnformatted で出力する。

    // Assert
    EXPECT_EQ(std::string(input), output); // [確認_正常系] - 出力が入力と完全に一致すること。
}

// 小数と指数を含む数値が従来の double 経路のまま扱われることの確認
TEST_F(cjsonIntegerTest, fraction_and_exponent_keep_double_path)
{
    // Arrange
    const char *input = "[1.5,1e3,-2.25]"; // [状態] - 小数と指数を含む JSON 配列を用意する。

    // Pre-Assert

    // Act
    std::string output = parseAndPrint(input); // [手順] - パースした結果を出力する。

    // Assert
    EXPECT_EQ(std::string("[1.5,1000,-2.25]"), output);
    // [確認_正常系] - 小数はそのまま、指数は従来どおり整数表記へ畳まれること。
}

// 先頭のゼロと負のゼロが正規化されることの確認
TEST_F(cjsonIntegerTest, normalizes_leading_zero_and_negative_zero)
{
    // Arrange
    const char *input = "[007,-0]"; // [状態] - 先頭ゼロと負のゼロを含む JSON 配列を用意する。

    // Pre-Assert

    // Act
    std::string output = parseAndPrint(input); // [手順] - パースした結果を出力する。

    // Assert
    EXPECT_EQ(std::string("[7,0]"), output); // [確認_正常系] - 妥当な JSON となるよう正規化されること。
}

// 8 種類の生成関数が期待する表記を出力することの確認
TEST_F(cjsonIntegerTest, create_functions_emit_expected_text)
{
    // Arrange

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(std::string("-128"), printAndDelete(cJSON_CreateInt8(INT8_MIN)));
    // [確認_正常系] - cJSON_CreateInt8(INT8_MIN) が -128 を出力すること。
    EXPECT_EQ(std::string("255"), printAndDelete(cJSON_CreateUInt8(UINT8_MAX)));
    // [確認_正常系] - cJSON_CreateUInt8(UINT8_MAX) が 255 を出力すること。
    EXPECT_EQ(std::string("-32768"), printAndDelete(cJSON_CreateInt16(INT16_MIN)));
    // [確認_正常系] - cJSON_CreateInt16(INT16_MIN) が -32768 を出力すること。
    EXPECT_EQ(std::string("65535"), printAndDelete(cJSON_CreateUInt16(UINT16_MAX)));
    // [確認_正常系] - cJSON_CreateUInt16(UINT16_MAX) が 65535 を出力すること。
    EXPECT_EQ(std::string("-2147483648"), printAndDelete(cJSON_CreateInt32(INT32_MIN)));
    // [確認_正常系] - cJSON_CreateInt32(INT32_MIN) が -2147483648 を出力すること。
    EXPECT_EQ(std::string("4294967295"), printAndDelete(cJSON_CreateUInt32(UINT32_MAX)));
    // [確認_正常系] - cJSON_CreateUInt32(UINT32_MAX) が 4294967295 を出力すること。
    EXPECT_EQ(std::string("-9223372036854775808"), printAndDelete(cJSON_CreateInt64(INT64_MIN)));
    // [確認_正常系] - cJSON_CreateInt64(INT64_MIN) が -9223372036854775808 を出力すること。
    EXPECT_EQ(std::string("18446744073709551615"), printAndDelete(cJSON_CreateUInt64(UINT64_MAX)));
    // [確認_正常系] - cJSON_CreateUInt64(UINT64_MAX) が 18446744073709551615 を出力すること。
}

// 取得関数が境界値で成功し、範囲外で失敗して格納先を変えないことの確認
TEST_F(cjsonIntegerTest, get_value_checks_range_boundaries)
{
    // Arrange
    cJSON *inRange = cJSON_CreateInt32(127);   // [状態] - int8_t の上限に収まる項目を用意する。
    cJSON *outOfRange = cJSON_CreateInt32(128); // [状態] - int8_t の上限を 1 超える項目を用意する。
    int8_t value = 42;                          // [状態] - 範囲外時に変更されないことを見るため 42 で初期化する。

    // Pre-Assert
    ASSERT_NE(nullptr, inRange);    // [Pre-Assert確認_正常系] - 収まる側の項目が生成できること。
    ASSERT_NE(nullptr, outOfRange); // [Pre-Assert確認_正常系] - 超える側の項目が生成できること。

    // Act
    cJSON_bool okInRange = cJSON_GetInt8Value(inRange, &value); // [手順] - 収まる値を int8_t として取得する。

    // Assert
    EXPECT_TRUE(okInRange);  // [確認_正常系] - 境界値の取得が成功すること。
    EXPECT_EQ(127, value);   // [確認_正常系] - 取得した値が 127 であること。

    // Act
    cJSON_bool okOutOfRange = cJSON_GetInt8Value(outOfRange, &value); // [手順] - 範囲外の値を int8_t として取得する。

    // Assert
    EXPECT_FALSE(okOutOfRange); // [確認_異常系] - 範囲外の取得が失敗すること。
    EXPECT_EQ(127, value);      // [確認_異常系] - 失敗時に格納先が変更されないこと。

    // Cleanup
    cJSON_Delete(inRange);
    cJSON_Delete(outOfRange);
}

// int64_t を超える値が符号なしの取得関数でのみ得られることの確認
TEST_F(cjsonIntegerTest, value_beyond_int64_requires_unsigned_getter)
{
    // Arrange
    cJSON *item = cJSON_CreateUInt64(UINT64_MAX); // [状態] - int64_t に収まらない値の項目を用意する。
    int64_t signedValue = 0;
    uint64_t unsignedValue = 0;

    // Pre-Assert
    ASSERT_NE(nullptr, item); // [Pre-Assert確認_正常系] - 項目が生成できること。

    // Act
    cJSON_bool okSigned = cJSON_GetInt64Value(item, &signedValue);       // [手順] - int64_t として取得を試みる。
    cJSON_bool okUnsigned = cJSON_GetUInt64Value(item, &unsignedValue);  // [手順] - uint64_t として取得を試みる。

    // Assert
    EXPECT_FALSE(okSigned);                    // [確認_異常系] - int64_t での取得が失敗すること。
    EXPECT_TRUE(okUnsigned);                   // [確認_正常系] - uint64_t での取得が成功すること。
    EXPECT_EQ(UINT64_MAX, unsignedValue);      // [確認_正常系] - 取得した値が UINT64_MAX であること。

    // Cleanup
    cJSON_Delete(item);
}

// 正確な整数を保持しない通常の数値からも取得できることの確認
TEST_F(cjsonIntegerTest, get_value_works_for_plain_number)
{
    // Arrange
    cJSON *item = cJSON_CreateNumber(42); // [状態] - 従来の cJSON_CreateNumber で数値項目を用意する。
    int8_t value = 0;

    // Pre-Assert
    ASSERT_NE(nullptr, item);                       // [Pre-Assert確認_正常系] - 項目が生成できること。
    EXPECT_FALSE(cJSON_HasExactInteger(item));      // [Pre-Assert確認_正常系] - 正確な整数を保持していないこと。

    // Act
    cJSON_bool ok = cJSON_GetInt8Value(item, &value); // [手順] - int8_t として取得する。

    // Assert
    EXPECT_TRUE(ok);       // [確認_正常系] - 通常の数値でも取得できること。
    EXPECT_EQ(42, value);  // [確認_正常系] - 取得した値が 42 であること。

    // Cleanup
    cJSON_Delete(item);
}

// 既存フィールドの挙動が従来どおり保たれることの確認
TEST_F(cjsonIntegerTest, legacy_fields_keep_previous_behavior)
{
    // Arrange
    cJSON *root = cJSON_Parse("{\"v\":9007199254740993}"); // [状態] - 2^53+1 を値に持つ JSON を用意する。

    // Pre-Assert
    ASSERT_NE(nullptr, root); // [Pre-Assert確認_正常系] - パースが成功すること。

    // Act
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, "v"); // [手順] - 値の項目を取得する。

    // Assert
    ASSERT_NE(nullptr, item);                          // [確認_正常系] - 項目が存在すること。
    EXPECT_EQ(INT_MAX, item->valueint);                // [確認_正常系] - valueint が従来どおり INT_MAX に飽和すること。
    EXPECT_DOUBLE_EQ(9007199254740992.0, item->valuedouble);
    // [確認_正常系] - valuedouble が従来どおり double へ丸めた値であること。
    EXPECT_DOUBLE_EQ(9007199254740992.0, cJSON_GetNumberValue(item));
    // [確認_正常系] - cJSON_GetNumberValue の戻り値が変わらないこと。

    // Cleanup
    cJSON_Delete(root);
}

// 複製しても正確な整数が保たれることの確認
TEST_F(cjsonIntegerTest, duplicate_keeps_exact_integer)
{
    // Arrange
    cJSON *root = cJSON_Parse("[9007199254740993]"); // [状態] - 2^53+1 を含む配列を用意する。

    // Pre-Assert
    ASSERT_NE(nullptr, root); // [Pre-Assert確認_正常系] - パースが成功すること。

    // Act
    cJSON *copy = cJSON_Duplicate(root, 1); // [手順] - 再帰指定で複製する。

    // Assert
    ASSERT_NE(nullptr, copy);                                       // [確認_正常系] - 複製が成功すること。
    EXPECT_EQ(std::string("[9007199254740993]"), printAndDelete(copy));
    // [確認_正常系] - 複製した側も正確な整数を出力すること。

    // Cleanup
    cJSON_Delete(root);
}

// 比較が double では区別できない 2 値を区別することの確認
TEST_F(cjsonIntegerTest, compare_distinguishes_values_beyond_double_precision)
{
    // Arrange
    cJSON *left = cJSON_CreateUInt64(18446744073709551615ULL);  // [状態] - UINT64_MAX の項目を用意する。
    cJSON *right = cJSON_CreateUInt64(18446744073709551614ULL); // [状態] - その 1 つ小さい値の項目を用意する。

    // Pre-Assert
    ASSERT_NE(nullptr, left);  // [Pre-Assert確認_正常系] - 左辺の項目が生成できること。
    ASSERT_NE(nullptr, right); // [Pre-Assert確認_正常系] - 右辺の項目が生成できること。
    EXPECT_DOUBLE_EQ(left->valuedouble, right->valuedouble);
    // [Pre-Assert確認_正常系] - double へ丸めると両者が区別できないこと。

    // Act
    cJSON_bool equal = cJSON_Compare(left, right, 1); // [手順] - 大文字小文字を区別して比較する。

    // Assert
    EXPECT_FALSE(equal); // [確認_正常系] - 異なる値として判定されること。

    // Cleanup
    cJSON_Delete(left);
    cJSON_Delete(right);
}

// フィールドを直接書き換えた項目が従来の double 経路へ落ちることの確認
//
// cJSON.h の cJSON_SetIntValue は valueint と valuedouble を直接書き換えるマクロであり、
// 関数側から検知できない。ここではそのマクロが行うのと同じ直接代入を行い、
// 整合性ガードが働くことを確認する。
// マクロ自体を呼ぶと、上流の定義が valueint へ double を代入するため利用側で
// -Wfloat-conversion が出る。抑止は OS ごとの書き分けが要るうえ、検証したいのは
// 「フィールドを直接書き換えた項目の扱い」であってマクロの展開形ではないため、
// 同等の代入を直接書く。
TEST_F(cjsonIntegerTest, direct_field_write_falls_back_to_double_path)
{
    // Arrange
    cJSON *item = cJSON_CreateInt64(9007199254740993LL); // [状態] - 2^53+1 を保持する項目を用意する。

    // Pre-Assert
    ASSERT_NE(nullptr, item);                 // [Pre-Assert確認_正常系] - 項目が生成できること。
    EXPECT_TRUE(cJSON_HasExactInteger(item)); // [Pre-Assert確認_正常系] - 正確な整数を保持していること。

    // Act
    item->valuedouble = 7.0; // [手順] - cJSON_SetIntValue マクロと同じく valuedouble を直接書き換える。
    item->valueint = 7;      // [手順] - あわせて valueint も直接書き換える。

    // Assert
    EXPECT_FALSE(cJSON_HasExactInteger(item));
    // [確認_正常系] - 整合性ガードが働き、保持していた表記が採用されなくなること。
    EXPECT_EQ(std::string("7"), printAndDelete(item));
    // [確認_正常系] - 書き換え後の値が出力されること。
}

// JSON Patch による差し替え後に新しい値が出力されることの確認
TEST_F(cjsonIntegerTest, apply_patches_replaces_exact_integer)
{
    // Arrange
    cJSON *object = cJSON_Parse("{\"v\":9007199254740993}"); // [状態] - 2^53+1 を値に持つ対象を用意する。
    cJSON *patches = cJSON_Parse("[{\"op\":\"replace\",\"path\":\"/v\",\"value\":123}]");
    // [状態] - v を 123 へ差し替える JSON Patch を用意する。

    // Pre-Assert
    ASSERT_NE(nullptr, object);  // [Pre-Assert確認_正常系] - 対象がパースできること。
    ASSERT_NE(nullptr, patches); // [Pre-Assert確認_正常系] - パッチがパースできること。

    // Act
    int status = cJSONUtils_ApplyPatches(object, patches); // [手順] - JSON Patch を適用する。

    // Assert
    EXPECT_EQ(0, status); // [確認_正常系] - 適用が成功すること。
    EXPECT_EQ(std::string("{\"v\":123}"), printAndDelete(object));
    // [確認_正常系] - 差し替え後の値が出力され、古い整数が残らないこと。

    // Cleanup
    cJSON_Delete(patches);
}
