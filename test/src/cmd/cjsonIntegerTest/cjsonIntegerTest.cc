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
    EXPECT_EQ(std::string(input),
              output); // [確認_正常系] - parseAndPrint() の戻り値が std::string(input) と一致すること。
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
    // [確認_正常系] - parseAndPrint() の戻り値が "[1.5,1000,-2.25]" であること。
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
    EXPECT_EQ(std::string("[7,0]"), output); // [確認_正常系] - parseAndPrint() の戻り値が "[7,0]" であること。
}

// 8 種類の生成関数が期待する表記を出力することの確認
TEST_F(cjsonIntegerTest, create_functions_emit_expected_text)
{
    // Arrange

    // Pre-Assert

    // Act
    // [手順] - 各生成関数で項目を作成し、printAndDelete で文字列化する。

    // Assert
    EXPECT_EQ(std::string("-128"), printAndDelete(cJSON_CreateInt8(INT8_MIN)));
    // [確認_正常系] - INT8_MIN の項目に対する printAndDelete() の戻り値が "-128" であること。
    EXPECT_EQ(std::string("255"), printAndDelete(cJSON_CreateUInt8(UINT8_MAX)));
    // [確認_正常系] - UINT8_MAX の項目に対する printAndDelete() の戻り値が "255" であること。
    EXPECT_EQ(std::string("-32768"), printAndDelete(cJSON_CreateInt16(INT16_MIN)));
    // [確認_正常系] - INT16_MIN の項目に対する printAndDelete() の戻り値が "-32768" であること。
    EXPECT_EQ(std::string("65535"), printAndDelete(cJSON_CreateUInt16(UINT16_MAX)));
    // [確認_正常系] - UINT16_MAX の項目に対する printAndDelete() の戻り値が "65535" であること。
    EXPECT_EQ(std::string("-2147483648"), printAndDelete(cJSON_CreateInt32(INT32_MIN)));
    // [確認_正常系] - INT32_MIN の項目に対する printAndDelete() の戻り値が "-2147483648" であること。
    EXPECT_EQ(std::string("4294967295"), printAndDelete(cJSON_CreateUInt32(UINT32_MAX)));
    // [確認_正常系] - UINT32_MAX の項目に対する printAndDelete() の戻り値が "4294967295" であること。
    EXPECT_EQ(std::string("-9223372036854775808"), printAndDelete(cJSON_CreateInt64(INT64_MIN)));
    // [確認_正常系] - INT64_MIN の項目に対する printAndDelete() の戻り値が "-9223372036854775808" であること。
    EXPECT_EQ(std::string("18446744073709551615"), printAndDelete(cJSON_CreateUInt64(UINT64_MAX)));
    // [確認_正常系] - UINT64_MAX の項目に対する printAndDelete() の戻り値が "18446744073709551615" であること。
}

// 取得関数が境界値で成功し、範囲外で失敗して格納先を変えないことの確認
TEST_F(cjsonIntegerTest, get_value_checks_range_boundaries)
{
    // Arrange
    cJSON *inRange = cJSON_CreateInt32(127);   // [状態] - int8_t の上限に収まる項目を用意する。
    cJSON *outOfRange = cJSON_CreateInt32(128); // [状態] - int8_t の上限を 1 超える項目を用意する。
    int8_t value = 42;                          // [状態] - 範囲外時に変更されないことを見るため 42 で初期化する。

    ASSERT_NE(nullptr, inRange); // [状態確認] - 127 を指定した cJSON_CreateInt32() の戻り値が nullptr 以外であること。
    ASSERT_NE(nullptr,
              outOfRange); // [状態確認] - 128 を指定した cJSON_CreateInt32() の戻り値が nullptr 以外であること。

    // Pre-Assert

    // Act
    cJSON_bool okInRange = cJSON_GetInt8Value(inRange, &value); // [手順] - 収まる値を int8_t として取得する。

    // Assert
    EXPECT_TRUE(okInRange); // [確認_正常系] - 範囲内の値に対する cJSON_GetInt8Value() の戻り値が true であること。
    EXPECT_EQ(127, value);  // [確認_正常系] - value が 127 であること。

    // Act_2
    cJSON_bool okOutOfRange = cJSON_GetInt8Value(outOfRange, &value); // [手順] - 範囲外の値を int8_t として取得する。

    // Assert_2
    EXPECT_FALSE(okOutOfRange); // [確認_異常系] - 範囲外の値に対する cJSON_GetInt8Value() の戻り値が false であること。
    EXPECT_EQ(127, value);      // [確認_異常系] - value が 127 であること。(取得失敗時に格納先が変更されないことの確認)

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

    ASSERT_NE(nullptr, item); // [状態確認] - cJSON_CreateUInt64() の戻り値が nullptr 以外であること。

    // Pre-Assert

    // Act
    cJSON_bool okSigned = cJSON_GetInt64Value(item, &signedValue);       // [手順] - int64_t として取得を試みる。
    cJSON_bool okUnsigned = cJSON_GetUInt64Value(item, &unsignedValue);  // [手順] - uint64_t として取得を試みる。

    // Assert
    EXPECT_FALSE(okSigned);               // [確認_異常系] - cJSON_GetInt64Value() の戻り値が false であること。
    EXPECT_TRUE(okUnsigned);              // [確認_正常系] - cJSON_GetUInt64Value() の戻り値が true であること。
    EXPECT_EQ(UINT64_MAX, unsignedValue); // [確認_正常系] - unsignedValue が UINT64_MAX であること。

    // Cleanup
    cJSON_Delete(item);
}

// 正確な整数を保持しない通常の数値からも取得できることの確認
TEST_F(cjsonIntegerTest, get_value_works_for_plain_number)
{
    // Arrange
    cJSON *item = cJSON_CreateNumber(42); // [状態] - 従来の cJSON_CreateNumber で数値項目を用意する。
    int8_t value = 0;

    ASSERT_NE(nullptr, item);                  // [状態確認] - cJSON_CreateNumber() の戻り値が nullptr 以外であること。
    EXPECT_FALSE(cJSON_HasExactInteger(item)); // [状態確認] - cJSON_HasExactInteger() の戻り値が false であること。

    // Pre-Assert

    // Act
    cJSON_bool ok = cJSON_GetInt8Value(item, &value); // [手順] - int8_t として取得する。

    // Assert
    EXPECT_TRUE(ok);      // [確認_正常系] - cJSON_GetInt8Value() の戻り値が true であること。
    EXPECT_EQ(42, value); // [確認_正常系] - value が 42 であること。

    // Cleanup
    cJSON_Delete(item);
}

// 既存フィールドの挙動が従来どおり保たれることの確認
TEST_F(cjsonIntegerTest, legacy_fields_keep_previous_behavior)
{
    // Arrange
    cJSON *root = cJSON_Parse("{\"v\":9007199254740993}"); // [状態] - 2^53+1 を値に持つ JSON を用意する。

    ASSERT_NE(nullptr, root); // [状態確認] - cJSON_Parse() の戻り値が nullptr 以外であること。

    // Pre-Assert

    // Act
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, "v"); // [手順] - 値の項目を取得する。

    // Assert
    ASSERT_NE(nullptr, item); // [確認_正常系] - cJSON_GetObjectItemCaseSensitive() の戻り値が nullptr 以外であること。
    EXPECT_EQ(INT_MAX, item->valueint); // [確認_正常系] - item->valueint が INT_MAX であること。
    EXPECT_DOUBLE_EQ(9007199254740992.0, item->valuedouble);
    // [確認_正常系] - item->valuedouble が 9007199254740992.0 であること。(9007199254740993 が double の精度で丸められたことの確認)
    EXPECT_DOUBLE_EQ(9007199254740992.0, cJSON_GetNumberValue(item));
    // [確認_正常系] - cJSON_GetNumberValue() の戻り値が 9007199254740992.0 であること。

    // Cleanup
    cJSON_Delete(root);
}

// 複製しても正確な整数が保たれることの確認
TEST_F(cjsonIntegerTest, duplicate_keeps_exact_integer)
{
    // Arrange
    cJSON *root = cJSON_Parse("[9007199254740993]"); // [状態] - 2^53+1 を含む配列を用意する。

    ASSERT_NE(nullptr, root); // [状態確認] - cJSON_Parse() の戻り値が nullptr 以外であること。

    // Pre-Assert

    // Act
    cJSON *copy = cJSON_Duplicate(root, 1); // [手順] - 再帰指定で複製する。

    // Assert
    ASSERT_NE(nullptr, copy); // [確認_正常系] - cJSON_Duplicate() の戻り値が nullptr 以外であること。
    EXPECT_EQ(std::string("[9007199254740993]"), printAndDelete(copy));
    // [確認_正常系] - 複製した項目に対する printAndDelete() の戻り値が "[9007199254740993]" であること。

    // Cleanup
    cJSON_Delete(root);
}

// 比較が double では区別できない 2 値を区別することの確認
TEST_F(cjsonIntegerTest, compare_distinguishes_values_beyond_double_precision)
{
    // Arrange
    cJSON *left = cJSON_CreateUInt64(18446744073709551615ULL);  // [状態] - UINT64_MAX の項目を用意する。
    cJSON *right = cJSON_CreateUInt64(18446744073709551614ULL); // [状態] - その 1 つ小さい値の項目を用意する。

    ASSERT_NE(nullptr,
              left); // [状態確認] - UINT64_MAX を指定した cJSON_CreateUInt64() の戻り値が nullptr 以外であること。
    ASSERT_NE(
        nullptr,
        right); // [状態確認] - UINT64_MAX より 1 小さい値を指定した cJSON_CreateUInt64() の戻り値が nullptr 以外であること。
    EXPECT_DOUBLE_EQ(left->valuedouble, right->valuedouble);
    // [状態確認] - right->valuedouble が left->valuedouble と一致すること。

    // Pre-Assert

    // Act
    cJSON_bool equal = cJSON_Compare(left, right, 1); // [手順] - 大文字小文字を区別して比較する。

    // Assert
    EXPECT_FALSE(equal); // [確認_正常系] - cJSON_Compare() の戻り値が false であること。

    // Cleanup
    cJSON_Delete(left);
    cJSON_Delete(right);
}

// cJSON.h の cJSON_SetIntValue は valueint と valuedouble を直接書き換えるマクロであり、
// 関数側から検知できない。ここではそのマクロが行うのと同じ直接代入を行い、
// 整合性ガードが働くことを確認する。
// マクロ自体を呼ぶと、上流の定義が valueint へ double を代入するため利用側で
// -Wfloat-conversion が出る。抑止は OS ごとの書き分けが要るうえ、検証したいのは
// 「フィールドを直接書き換えた項目の扱い」であってマクロの展開形ではないため、
// 同等の代入を直接書く。

// フィールドを直接書き換えた項目が従来の double 経路へ落ちることの確認
TEST_F(cjsonIntegerTest, direct_field_write_falls_back_to_double_path)
{
    // Arrange
    cJSON *item = cJSON_CreateInt64(9007199254740993LL); // [状態] - 2^53+1 を保持する項目を用意する。

    ASSERT_NE(nullptr, item); // [状態確認] - cJSON_CreateInt64() の戻り値が nullptr 以外であること。
    EXPECT_TRUE(
        cJSON_HasExactInteger(item)); // [状態確認] - 書き換え前の cJSON_HasExactInteger() の戻り値が true であること。

    // Pre-Assert

    // Act
    item->valuedouble = 7.0; // [手順] - cJSON_SetIntValue マクロと同じく valuedouble を直接書き換える。
    item->valueint = 7;      // [手順] - あわせて valueint も直接書き換える。

    // Assert
    EXPECT_FALSE(cJSON_HasExactInteger(item));
    // [確認_正常系] - 書き換え後の cJSON_HasExactInteger() の戻り値が false であること。
    EXPECT_EQ(std::string("7"), printAndDelete(item));
    // [確認_正常系] - 書き換え後の printAndDelete() の戻り値が "7" であること。
}

// JSON Patch による差し替え後に新しい値が出力されることの確認
TEST_F(cjsonIntegerTest, apply_patches_replaces_exact_integer)
{
    // Arrange
    cJSON *object = cJSON_Parse("{\"v\":9007199254740993}"); // [状態] - 2^53+1 を値に持つ対象を用意する。
    cJSON *patches = cJSON_Parse("[{\"op\":\"replace\",\"path\":\"/v\",\"value\":123}]");
    // [状態] - v を 123 へ差し替える JSON Patch を用意する。

    ASSERT_NE(nullptr, object);  // [状態確認] - 対象を解析した cJSON_Parse() の戻り値が nullptr 以外であること。
    ASSERT_NE(nullptr, patches); // [状態確認] - パッチを解析した cJSON_Parse() の戻り値が nullptr 以外であること。

    // Pre-Assert

    // Act
    int status = cJSONUtils_ApplyPatches(object, patches); // [手順] - JSON Patch を適用する。

    // Assert
    EXPECT_EQ(0, status); // [確認_正常系] - cJSONUtils_ApplyPatches() の戻り値が 0 であること。
    EXPECT_EQ(std::string("{\"v\":123}"), printAndDelete(object));
    // [確認_正常系] - パッチ適用後の printAndDelete() の戻り値が "{\"v\":123}" であること。

    // Cleanup
    cJSON_Delete(patches);
}
