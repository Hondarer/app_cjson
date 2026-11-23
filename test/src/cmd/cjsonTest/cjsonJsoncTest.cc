#include <testfw.h>

#include <cJSON.h>
#include <cJSON_Integer.h>
#include <cJSON_JSONC.h>
#include <mock_stdlib.h>

#include "cJSON_JSONC.inject.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace
{

struct AllocationPlan
{
    size_t call_count = 0U;
    size_t fail_call = 0U;
    size_t fail_size = 0U;
    size_t fail_size_count = 0U;
    int barrier_role = 0;
    std::vector<size_t> requested_sizes;
};

thread_local AllocationPlan allocation_plan;

struct ParseBarrier
{
    std::mutex mutex;
    std::condition_variable condition;
    bool first_blocked = false;
    bool second_normalized = false;
    bool second_parser_allocation = false;
    bool release_first = false;
    int active_parser_allocations = 0;
    int maximum_parser_allocations = 0;
};

std::atomic<ParseBarrier *> parse_barrier(nullptr);
std::mutex allocation_map_mutex;
std::map<void *, size_t> allocation_sizes;
std::atomic<size_t> wipe_count(0U);
std::atomic<size_t> wipe_failure_count(0U);
std::atomic<size_t> unknown_free_count(0U);

void update_maximum_parser_allocations(ParseBarrier *barrier)
{
    barrier->active_parser_allocations++;
    if (barrier->active_parser_allocations > barrier->maximum_parser_allocations)
    {
        barrier->maximum_parser_allocations = barrier->active_parser_allocations;
    }
}

void *controlled_malloc(const char *file, const int line, const char *func, size_t size)
{
    allocation_plan.call_count++;
    allocation_plan.requested_sizes.push_back(size);
    if (allocation_plan.call_count == allocation_plan.fail_call)
    {
        return nullptr;
    }
    if ((allocation_plan.fail_size_count != 0U) && (size == allocation_plan.fail_size))
    {
        allocation_plan.fail_size_count--;
        return nullptr;
    }

    void *allocated = delegate_real_malloc(file, line, func, size);
    if (allocated == nullptr)
    {
        return nullptr;
    }
    {
        std::lock_guard<std::mutex> guard(allocation_map_mutex);
        allocation_sizes[allocated] = size;
    }

    ParseBarrier *barrier = parse_barrier.load();
    if ((barrier != nullptr) && (allocation_plan.call_count == 1U) && (allocation_plan.barrier_role == 2))
    {
        std::lock_guard<std::mutex> guard(barrier->mutex);
        barrier->second_normalized = true;
        barrier->condition.notify_all();
    }
    if ((barrier != nullptr) && (allocation_plan.call_count == 2U) && (allocation_plan.barrier_role != 0))
    {
        std::unique_lock<std::mutex> guard(barrier->mutex);
        update_maximum_parser_allocations(barrier);
        if (allocation_plan.barrier_role == 1)
        {
            barrier->first_blocked = true;
            barrier->condition.notify_all();
            barrier->condition.wait(guard, [barrier]() { return barrier->release_first; });
        }
        else
        {
            barrier->second_parser_allocation = true;
            barrier->condition.notify_all();
        }
        barrier->active_parser_allocations--;
    }

    return allocated;
}

void controlled_free(const char *file, const int line, const char *func, void *pointer)
{
    size_t size = 0U;
    bool found = false;
    {
        std::lock_guard<std::mutex> guard(allocation_map_mutex);
        auto allocation = allocation_sizes.find(pointer);
        if (allocation != allocation_sizes.end())
        {
            size = allocation->second;
            allocation_sizes.erase(allocation);
            found = true;
        }
    }

    if (found)
    {
        const unsigned char *bytes = static_cast<const unsigned char *>(pointer);
        bool wiped = true;
        for (size_t index = 0U; index < size; index++)
        {
            if (bytes[index] != 0U)
            {
                wiped = false;
                break;
            }
        }
        wipe_count.fetch_add(1U);
        if (!wiped)
        {
            wipe_failure_count.fetch_add(1U);
        }
    }
    else
    {
        unknown_free_count.fetch_add(1U);
    }

    delegate_real_free(file, line, func, pointer);
}

void reset_allocation_plan()
{
    allocation_plan = {};
}

struct ParseOutcome
{
    cJSON *root = nullptr;
    cjson_app_jsonc_status status = CJSON_APP_JSONC_INVALID_ARGUMENT;
    size_t offset = 0U;
};

struct ThreadOutcome
{
    cjson_app_jsonc_status status = CJSON_APP_JSONC_INVALID_ARGUMENT;
    int value = -1;
    size_t allocation_failures = 0U;
};

ParseOutcome parse_with_status(const char *text, bool store_offset = true)
{
    ParseOutcome outcome;
    size_t *offset = store_offset ? &outcome.offset : nullptr;
    outcome.root = cjson_app_cJSON_ParseJSONCWithLengthOpts(text, std::strlen(text), &outcome.status, offset);
    return outcome;
}

ThreadOutcome parse_value(bool use_new_api, const char *text)
{
    ThreadOutcome outcome;
    cJSON *root = nullptr;
    if (use_new_api)
    {
        cjson_app_jsonc_status status = CJSON_APP_JSONC_INVALID_ARGUMENT;
        root = cjson_app_cJSON_ParseJSONCWithLengthOpts(text, std::strlen(text), &status, nullptr);
        outcome.status = status;
    }
    else
    {
        root = cjson_app_cJSON_ParseJSONCWithLength(text, std::strlen(text));
        outcome.status = (root == nullptr) ? CJSON_APP_JSONC_SYNTAX_ERROR : CJSON_APP_JSONC_OK;
    }

    outcome.allocation_failures = cjson_jsonc_test_allocation_failure_count();
    if (root != nullptr)
    {
        cJSON *value = cjson_app_cJSON_GetObjectItemCaseSensitive(root, "v");
        if (value != nullptr)
        {
            outcome.value = value->valueint;
        }
        cjson_app_cJSON_Delete(root);
    }
    return outcome;
}

size_t text_index(const char *text, const char *found)
{
    return static_cast<size_t>(found - text);
}

std::vector<size_t> record_successful_allocations(const char *text)
{
    reset_allocation_plan();
    ParseOutcome outcome = parse_with_status(text);
    std::vector<size_t> sizes = allocation_plan.requested_sizes;
    if (outcome.root != nullptr)
    {
        cjson_app_cJSON_Delete(outcome.root);
    }
    return sizes;
}

struct SerialResult
{
    bool first_blocked = false;
    bool second_normalized = false;
    bool mutex_busy = false;
    bool second_did_not_enter = false;
    int maximum_parser_allocations = 0;
    ThreadOutcome first;
    ThreadOutcome second;
};

SerialResult run_serial_pair(bool first_new_api, bool second_new_api)
{
    ParseBarrier barrier;
    SerialResult result;
    parse_barrier.store(&barrier);
    std::thread first([&]() {
        reset_allocation_plan();
        allocation_plan.barrier_role = 1;
        result.first = parse_value(first_new_api, "{\"v\":1}");
    });

    {
        std::unique_lock<std::mutex> guard(barrier.mutex);
        result.first_blocked = barrier.condition.wait_for(guard, std::chrono::seconds(3), [&barrier]() {
            return barrier.first_blocked;
        });
    }
    if (result.first_blocked)
    {
        std::thread second([&]() {
            reset_allocation_plan();
            allocation_plan.barrier_role = 2;
            result.second = parse_value(second_new_api, "{\"v\":2}");
        });
        {
            std::unique_lock<std::mutex> guard(barrier.mutex);
            result.second_normalized = barrier.condition.wait_for(guard, std::chrono::seconds(3), [&barrier]() {
                return barrier.second_normalized;
            });
        }
        result.mutex_busy = (cjson_jsonc_test_parse_lock_is_available() == 0);
        {
            std::unique_lock<std::mutex> guard(barrier.mutex);
            barrier.condition.wait_for(guard, std::chrono::milliseconds(100), [&barrier]() {
                return barrier.second_parser_allocation;
            });
            result.second_did_not_enter = !barrier.second_parser_allocation;
            result.maximum_parser_allocations = barrier.maximum_parser_allocations;
            barrier.release_first = true;
            barrier.condition.notify_all();
        }
        second.join();
    }
    else
    {
        std::lock_guard<std::mutex> guard(barrier.mutex);
        barrier.release_first = true;
        barrier.condition.notify_all();
    }
    first.join();
    if (!result.first_blocked)
    {
        result.maximum_parser_allocations = barrier.maximum_parser_allocations;
    }
    parse_barrier.store(nullptr);
    return result;
}

} // namespace

class cjsonJsoncTest : public Test
{
  protected:
    void SetUp() override
    {
        static std::once_flag hooks_installed;
        std::call_once(hooks_installed, cjson_jsonc_test_install_hooks);
        cjson_jsonc_test_reset_allocation_failure_count();
        reset_allocation_plan();
        wipe_count.store(0U);
        wipe_failure_count.store(0U);
        unknown_free_count.store(0U);
        ON_CALL(mock_stdlib, malloc(_, _, _, _)).WillByDefault(Invoke(controlled_malloc));
        ON_CALL(mock_stdlib, free(_, _, _, _)).WillByDefault(Invoke(controlled_free));
    }

  private:
    NiceMock<Mock_stdlib> mock_stdlib;
};

// JSONC の成功と、構文エラーの種別および位置を返すことの確認
TEST_F(cjsonJsoncTest, reports_success_and_syntax_positions)
{
    // Arrange
    const char valid[] = "/* c */ {\"name\":\"a,\",}";
    const char first_line[] = "{,}";
    const char after_comment[] = "/* c */ {,}";
    const char after_comma[] = "{\"a\":1,}x";
    const char unclosed[] = "{\"a\":1} /* tail";
    const char missing_brace[] = "{\"a\":1";
    const char *comment_comma = std::strchr(after_comment, ',');
    const char *trailing_bad = std::strchr(after_comma, 'x');
    const char *comment_start = std::strstr(unclosed, "/*");

    // Pre-Assert
    ASSERT_NE(nullptr, comment_comma); // [状態確認] - コメントの後ろの誤りに使うカンマが見つかったこと。
    ASSERT_NE(nullptr, trailing_bad);  // [状態確認] - 末尾カンマの後ろの誤りに使う文字が見つかったこと。
    ASSERT_NE(nullptr, comment_start); // [状態確認] - 閉じていないコメントの開始位置が見つかったこと。

    // Act
    ParseOutcome success = parse_with_status(valid);
    ParseOutcome line = parse_with_status(first_line);
    ParseOutcome commented = parse_with_status(after_comment);
    ParseOutcome comma = parse_with_status(after_comma);
    ParseOutcome block = parse_with_status(unclosed);
    ParseOutcome brace = parse_with_status(missing_brace);

    // Assert
    ASSERT_NE(nullptr, success.root); // [確認_正常系] - 正当な JSONC の解析結果が存在すること。
    EXPECT_EQ(CJSON_APP_JSONC_OK, success.status); // [確認_正常系] - 成功時の状態が OK であること。
    EXPECT_EQ(0U, success.offset); // [確認_正常系] - 成功時の位置が 0 であること。
    EXPECT_EQ(nullptr, line.root); // [確認_異常系] - 不正な入力の解析結果が NULL であること。
    EXPECT_EQ(CJSON_APP_JSONC_SYNTAX_ERROR, line.status); // [確認_異常系] - 構文エラーを返すこと。
    EXPECT_EQ(2U, line.offset); // [確認_異常系] - `{,}` の解析を止めた位置が 2 であること。
    EXPECT_EQ('}', first_line[line.offset]); // [確認_異常系] - 停止位置が閉じ括弧を指すこと。
    EXPECT_EQ('}', after_comment[commented.offset]); // [確認_異常系] - コメントの後ろの解析停止位置が閉じ括弧であること。
    EXPECT_EQ(text_index(after_comment, comment_comma) + 1U, commented.offset); // [確認_異常系] - コメントを含む入力でも位置が元の入力と一致すること。
    EXPECT_EQ(text_index(after_comma, trailing_bad), comma.offset); // [確認_異常系] - 末尾カンマ後の誤り位置が元の入力と一致すること。
    EXPECT_EQ(CJSON_APP_JSONC_SYNTAX_ERROR, block.status); // [確認_異常系] - 未閉鎖コメントを構文エラーとして返すこと。
    EXPECT_EQ(text_index(unclosed, comment_start), block.offset); // [確認_異常系] - 未閉鎖コメントの開始位置を返すこと。
    EXPECT_EQ(std::strlen(missing_brace), brace.offset); // [確認_異常系] - 閉じ括弧不足の位置が入力の終端であること。

    // Cleanup
    cjson_app_cJSON_Delete(success.root);
}

// 入れ子の構文エラーで cJSON が止めた位置を返すことの確認
TEST_F(cjsonJsoncTest, reports_nested_stop_position)
{
    // Arrange
    const char object_value[] = "{\"a\":}";
    const char array_value[] = "[1,x]";
    const char bad_escape[] = "{\"a\":\"\\q\"}";
    const char *object_bad = std::strchr(object_value, '}');
    const char *array_bad = std::strchr(array_value, 'x');
    const char *escape_quote = std::strchr(std::strchr(bad_escape, ':'), '"');

    // Pre-Assert
    ASSERT_NE(nullptr, object_bad); // [状態確認] - オブジェクトの停止位置が見つかったこと。
    ASSERT_NE(nullptr, array_bad); // [状態確認] - 配列の不正な要素が見つかったこと。
    ASSERT_NE(nullptr, escape_quote); // [状態確認] - エスケープを含む文字列の開始位置が見つかったこと。

    // Act
    ParseOutcome object = parse_with_status(object_value);
    ParseOutcome array = parse_with_status(array_value);
    ParseOutcome escape = parse_with_status(bad_escape);

    // Assert
    EXPECT_EQ(CJSON_APP_JSONC_SYNTAX_ERROR, object.status); // [確認_異常系] - オブジェクトの構文エラーを返すこと。
    EXPECT_EQ(text_index(object_value, object_bad), object.offset); // [確認_異常系] - オブジェクトを解析停止位置で報告すること。
    EXPECT_EQ(text_index(array_value, array_bad), array.offset); // [確認_異常系] - 配列の不正な要素の位置を報告すること。
    EXPECT_EQ(text_index(bad_escape, escape_quote) + 1U, escape.offset); // [確認_異常系] - 不正なエスケープの解析停止位置を報告すること。
    EXPECT_EQ('\\', bad_escape[escape.offset]); // [確認_異常系] - 不正なエスケープの位置がバックスラッシュであること。
}

// 不正な引数を INVALID_ARGUMENT として返すことの確認
TEST_F(cjsonJsoncTest, rejects_invalid_arguments)
{
    // Arrange
    const char embedded[] = {'a', '\0', 'b'};
    const char *text = "{}";
    size_t offset = 7U;
    cjson_app_jsonc_status status = CJSON_APP_JSONC_OK;

    // Act
    cJSON *missing_status = cjson_app_cJSON_ParseJSONCWithLengthOpts(text, std::strlen(text), nullptr, &offset);
    size_t offset_after_missing_status = offset;
    offset = 7U;
    cJSON *missing_value = cjson_app_cJSON_ParseJSONCWithLengthOpts(nullptr, 0U, &status, &offset);
    cjson_app_jsonc_status missing_value_status = status;
    status = CJSON_APP_JSONC_OK;
    cJSON *too_long = cjson_app_cJSON_ParseJSONCWithLengthOpts(text, SIZE_MAX, &status, nullptr);
    cjson_app_jsonc_status too_long_status = status;
    status = CJSON_APP_JSONC_OK;
    cJSON *embedded_nul = cjson_app_cJSON_ParseJSONCWithLengthOpts(embedded, sizeof(embedded), &status, nullptr);
    cjson_app_jsonc_status embedded_status = status;

    // Assert
    EXPECT_EQ(nullptr, missing_status); // [確認_異常系] - status が NULL の場合に NULL を返すこと。
    EXPECT_EQ(0U, offset_after_missing_status); // [確認_異常系] - status が NULL でも位置を 0 にすること。
    EXPECT_EQ(nullptr, missing_value); // [確認_異常系] - value が NULL の場合に NULL を返すこと。
    EXPECT_EQ(CJSON_APP_JSONC_INVALID_ARGUMENT, missing_value_status); // [確認_異常系] - value が NULL の場合の種別を返すこと。
    EXPECT_EQ(0U, offset); // [確認_異常系] - 引数エラーの位置を 0 にすること。
    EXPECT_EQ(nullptr, too_long); // [確認_異常系] - SIZE_MAX の長さを拒否すること。
    EXPECT_EQ(CJSON_APP_JSONC_INVALID_ARGUMENT, too_long_status); // [確認_異常系] - SIZE_MAX の長さを引数エラーとして返すこと。
    EXPECT_EQ(nullptr, embedded_nul); // [確認_異常系] - 入力途中の NUL を拒否すること。
    EXPECT_EQ(CJSON_APP_JSONC_INVALID_ARGUMENT, embedded_status); // [確認_異常系] - 入力途中の NUL を引数エラーとして返すこと。
}

// 要素および正確な整数の文字列の確保失敗をメモリ不足として返すことの確認
TEST_F(cjsonJsoncTest, returns_out_of_memory_for_element_and_exact_integer)
{
    // Arrange
    const char object_text[] = "{\"v\":1}";
    const char integer_text[] = "{\"n\":9007199254740993}";
    const size_t header_size = cjson_jsonc_test_allocation_header_size();
    std::vector<size_t> object_allocations = record_successful_allocations(object_text);
    std::vector<size_t> integer_allocations = record_successful_allocations(integer_text);
    std::vector<size_t> exact_integer_ordinals;
    const size_t exact_integer_allocation_size = header_size + std::strlen("9007199254740993") + 1U;
    for (size_t index = 0U; index < integer_allocations.size(); index++)
    {
        if (integer_allocations[index] == exact_integer_allocation_size)
        {
            exact_integer_ordinals.push_back(index + 1U);
        }
    }

    // Pre-Assert
    ASSERT_GT(object_allocations.size(), 1U); // [状態確認] - 正規化後に cJSON の要素を確保すること。
    ASSERT_EQ(header_size + sizeof(cJSON), object_allocations[1U]); // [状態確認] - 2 回目の確保が cJSON 要素のサイズであること。
    ASSERT_EQ(3U, exact_integer_ordinals.size()); // [状態確認] - 整数トークン用一時領域、整数変換用一時文字列、保存文字列を確保すること。

    // Act
    reset_allocation_plan();
    allocation_plan.fail_call = 2U;
    ParseOutcome element = parse_with_status(object_text);
    reset_allocation_plan();
    allocation_plan.fail_call = exact_integer_ordinals[0U];
    ParseOutcome integer_token = parse_with_status(integer_text);
    reset_allocation_plan();
    allocation_plan.fail_call = exact_integer_ordinals[1U];
    ParseOutcome integer_temporary = parse_with_status(integer_text);
    reset_allocation_plan();
    allocation_plan.fail_call = exact_integer_ordinals[2U];
    ParseOutcome integer_stored = parse_with_status(integer_text);
    reset_allocation_plan();
    allocation_plan.fail_call = 2U;
    cJSON *legacy_result = cjson_app_cJSON_ParseJSONCWithLength(object_text, std::strlen(object_text));
    reset_allocation_plan();
    ParseOutcome recovered = parse_with_status(object_text);

    // Assert
    EXPECT_EQ(nullptr, element.root); // [確認_異常系] - 要素確保失敗時に解析結果を返さないこと。
    EXPECT_EQ(CJSON_APP_JSONC_OUT_OF_MEMORY, element.status); // [確認_異常系] - 要素確保失敗をメモリ不足として返すこと。
    EXPECT_EQ(nullptr, integer_token.root); // [確認_異常系] - 整数トークン用一時領域の確保失敗時に結果を返さないこと。
    EXPECT_EQ(CJSON_APP_JSONC_OUT_OF_MEMORY, integer_token.status); // [確認_異常系] - 整数トークン用一時領域の確保失敗をメモリ不足として返すこと。
    EXPECT_EQ(nullptr, integer_temporary.root); // [確認_異常系] - 整数一時文字列の確保失敗時に結果を返さないこと。
    EXPECT_EQ(CJSON_APP_JSONC_OUT_OF_MEMORY, integer_temporary.status); // [確認_異常系] - 一時文字列の確保失敗をメモリ不足として返すこと。
    EXPECT_EQ(nullptr, integer_stored.root); // [確認_異常系] - 整数保存文字列の確保失敗時に結果を返さないこと。
    EXPECT_EQ(CJSON_APP_JSONC_OUT_OF_MEMORY, integer_stored.status); // [確認_異常系] - 保存文字列の確保失敗をメモリ不足として返すこと。
    EXPECT_EQ(nullptr, legacy_result); // [確認_異常系] - 旧 API も解析中の確保失敗を NULL で返すこと。
    ASSERT_NE(nullptr, recovered.root); // [確認_正常系] - 確保失敗後に次の解析が成功すること。
    EXPECT_EQ(CJSON_APP_JSONC_OK, recovered.status); // [確認_正常系] - 回復後の解析状態が OK であること。

    // Cleanup
    cjson_app_cJSON_Delete(recovered.root);
}

// 新旧の解析 API の組み合わせを同時に呼び出して排他することの確認
TEST_F(cjsonJsoncTest, serializes_new_and_existing_parser_calls)
{
    // Arrange
    const SerialResult new_and_new = run_serial_pair(true, true);
    const SerialResult old_and_old = run_serial_pair(false, false);
    const SerialResult new_and_old = run_serial_pair(true, false);

    // Assert
    EXPECT_TRUE(new_and_new.first_blocked); // [確認_正常系] - 新 API の先行解析が内部確保で停止したこと。
    EXPECT_TRUE(new_and_new.second_normalized); // [確認_正常系] - 2 つ目の新 API がロック前の正規化を終えたこと。
    EXPECT_TRUE(new_and_new.mutex_busy); // [確認_正常系] - 先行解析の間に共通ロックを取得できないこと。
    EXPECT_TRUE(new_and_new.second_did_not_enter); // [確認_正常系] - 先行解析中に 2 つ目が cJSON 本体へ入らないこと。
    EXPECT_EQ(1, new_and_new.maximum_parser_allocations); // [確認_正常系] - 新 API の解析区間内の同時数が 1 であること。
    EXPECT_EQ(CJSON_APP_JSONC_OK, new_and_new.first.status); // [確認_正常系] - 先行する新 API の解析が成功すること。
    EXPECT_EQ(CJSON_APP_JSONC_OK, new_and_new.second.status); // [確認_正常系] - 後続の新 API の解析が成功すること。
    EXPECT_TRUE(old_and_old.first_blocked); // [確認_正常系] - 先行する旧 API の解析が内部確保で停止したこと。
    EXPECT_TRUE(old_and_old.mutex_busy); // [確認_正常系] - 旧 API の解析も共通ロックを取得すること。
    EXPECT_TRUE(old_and_old.second_did_not_enter); // [確認_正常系] - 先行解析中に 2 つ目が cJSON 本体へ入らないこと。
    EXPECT_EQ(1, old_and_old.maximum_parser_allocations); // [確認_正常系] - 旧 API の解析区間内の同時数が 1 であること。
    EXPECT_TRUE(new_and_old.first_blocked); // [確認_正常系] - 新 API の解析が内部確保で停止したこと。
    EXPECT_TRUE(new_and_old.mutex_busy); // [確認_正常系] - 新旧 API が同じロックを使うこと。
    EXPECT_TRUE(new_and_old.second_did_not_enter); // [確認_正常系] - 先行解析中に旧 API が cJSON 本体へ入らないこと。
    EXPECT_EQ(1, new_and_old.maximum_parser_allocations); // [確認_正常系] - 新旧混在時の解析区間内の同時数が 1 であること。
    EXPECT_EQ(1, new_and_new.first.value); // [確認_正常系] - 先行する解析の値が保たれること。
    EXPECT_EQ(2, new_and_new.second.value); // [確認_正常系] - 後続する解析の値が保たれること。
    EXPECT_EQ(1, old_and_old.first.value); // [確認_正常系] - 旧 API の先行解析の値が保たれること。
    EXPECT_EQ(2, old_and_old.second.value); // [確認_正常系] - 旧 API の後続解析の値が保たれること。
    EXPECT_EQ(1, new_and_old.first.value); // [確認_正常系] - 新 API の値が保たれること。
    EXPECT_EQ(2, new_and_old.second.value); // [確認_正常系] - 旧 API の値が保たれること。
    EXPECT_EQ(1, cjson_jsonc_test_parse_lock_is_available()); // [確認_正常系] - すべての解析後にロックを取得できること。
}

// 各スレッドの失敗回数が独立していることの確認
TEST_F(cjsonJsoncTest, keeps_allocation_failures_thread_local)
{
    // Arrange
    ThreadOutcome failed;
    ThreadOutcome successful;
    std::thread failing_thread([&]() {
        reset_allocation_plan();
        cjson_jsonc_test_reset_allocation_failure_count();
        allocation_plan.fail_call = 2U;
        failed = parse_value(true, "{\"v\":1}");
    });
    std::thread successful_thread([&]() {
        reset_allocation_plan();
        cjson_jsonc_test_reset_allocation_failure_count();
        successful = parse_value(true, "{\"v\":2}");
    });

    // Act
    failing_thread.join();
    successful_thread.join();

    // Assert
    EXPECT_EQ(CJSON_APP_JSONC_OUT_OF_MEMORY, failed.status); // [確認_異常系] - 失敗させたスレッドだけがメモリ不足を返すこと。
    EXPECT_EQ(1U, failed.allocation_failures); // [確認_異常系] - 失敗したスレッドの TLS カウンターだけが増えること。
    EXPECT_EQ(CJSON_APP_JSONC_OK, successful.status); // [確認_正常系] - 他方のスレッドでは解析が成功すること。
    EXPECT_EQ(0U, successful.allocation_failures); // [確認_正常系] - 他方の TLS カウンターが増えないこと。
    EXPECT_EQ(2, successful.value); // [確認_正常系] - 他方の解析結果に値が残ること。
}

// 構文エラーと確保失敗の後も次の解析が完了することの確認
TEST_F(cjsonJsoncTest, recovers_after_syntax_and_allocation_errors)
{
    // Arrange
    const char broken[] = "{,}";
    const char valid[] = "{\"v\":1}";
    ParseOutcome syntax = parse_with_status(broken);
    reset_allocation_plan();
    allocation_plan.fail_call = 2U;
    ParseOutcome allocation = parse_with_status(valid);
    reset_allocation_plan();
    ParseOutcome recovered = parse_with_status(valid);

    // Assert
    EXPECT_EQ(CJSON_APP_JSONC_SYNTAX_ERROR, syntax.status); // [確認_異常系] - 構文エラーを返したこと。
    EXPECT_EQ(CJSON_APP_JSONC_OUT_OF_MEMORY, allocation.status); // [確認_異常系] - 確保失敗をメモリ不足として返したこと。
    ASSERT_NE(nullptr, recovered.root); // [確認_正常系] - エラーの後に次の解析が完了すること。
    EXPECT_EQ(1, recovered.root->child->valueint); // [確認_正常系] - 回復後の解析値が正しいこと。
    EXPECT_EQ(1, cjson_jsonc_test_parse_lock_is_available()); // [確認_正常系] - エラー経路の後もロックが解放されていること。

    // Cleanup
    cjson_app_cJSON_Delete(recovered.root);
}

// すべての所有領域を消去してから実際の free へ渡すことの確認
TEST_F(cjsonJsoncTest, wipes_owned_memory_before_free)
{
    // Arrange
    const char syntax_text[] = "{\"password\":\"secret-passphrase\"";
    const char valid_text[] = "{\"password\":\"secret-passphrase\"}";
    const char integer_text[] = "{\"n\":9007199254740993}";
    const size_t header_size = cjson_jsonc_test_allocation_header_size();
    std::vector<size_t> valid_allocations = record_successful_allocations(valid_text);
    std::vector<size_t> integer_allocations = record_successful_allocations(integer_text);
    std::vector<size_t> integer_ordinals;
    const size_t exact_integer_size = header_size + std::strlen("9007199254740993") + 1U;
    for (size_t index = 0U; index < integer_allocations.size(); index++)
    {
        if (integer_allocations[index] == exact_integer_size)
        {
            integer_ordinals.push_back(index + 1U);
        }
    }

    // Act
    void *direct = cjson_app_cJSON_malloc(32U);
    ASSERT_NE(nullptr, direct); // [状態確認] - 機密文字列を含む領域を確保できたこと。
    std::memset(direct, 0xA5, 32U);
    std::memcpy(direct, "sec", 3U);
    static_cast<unsigned char *>(direct)[3U] = 0U;
    std::memcpy(static_cast<unsigned char *>(direct) + 4U, "ret-passphrase!!", 16U);
    size_t before_direct_free = wipe_count.load();
    cjson_app_cJSON_free(direct);
    const size_t after_direct_free = wipe_count.load();

    ParseOutcome syntax = parse_with_status(syntax_text, false);
    const size_t syntax_wipes = wipe_count.load();
    size_t allocation_failures = 0U;
    for (size_t ordinal = 1U; ordinal <= valid_allocations.size(); ordinal++)
    {
        reset_allocation_plan();
        allocation_plan.fail_call = ordinal;
        ParseOutcome failed = parse_with_status(valid_text, false);
        if (failed.status == CJSON_APP_JSONC_OUT_OF_MEMORY)
        {
            allocation_failures++;
        }
    }

    size_t integer_failures = 0U;
    for (size_t ordinal : integer_ordinals)
    {
        reset_allocation_plan();
        allocation_plan.fail_call = ordinal;
        ParseOutcome failed = parse_with_status(integer_text, false);
        if (failed.status == CJSON_APP_JSONC_OUT_OF_MEMORY)
        {
            integer_failures++;
        }
    }

    reset_allocation_plan();
    allocation_plan.fail_call = 1U;
    const size_t normalization_wipes_before = wipe_count.load();
    ParseOutcome normalization = parse_with_status(valid_text, false);
    const size_t normalization_wipes = wipe_count.load();

    reset_allocation_plan();
    ParseOutcome created = parse_with_status(valid_text, false);
    cJSON *replacement = cjson_app_cJSON_CreateString("next");
    cJSON_bool replaced = cjson_app_cJSON_ReplaceItemInObjectCaseSensitive(created.root, "password", replacement);
    char *printed = cjson_app_cJSON_PrintUnformatted(created.root);
    cjson_app_cJSON_free(printed);
    cjson_app_cJSON_Delete(created.root);

    // Assert
    EXPECT_EQ(before_direct_free + 1U, after_direct_free); // [確認_正常系] - 解放前に標準ライブラリ mock が領域を観測したこと。
    EXPECT_EQ(CJSON_APP_JSONC_SYNTAX_ERROR, syntax.status); // [確認_異常系] - パスフレーズ後の構文エラーを返したこと。
    EXPECT_GT(syntax_wipes, after_direct_free); // [確認_異常系] - 構文エラーの部分木と正規化領域を解放したこと。
    EXPECT_EQ(valid_allocations.size(), allocation_failures); // [確認_異常系] - 各 N 回目の確保失敗をメモリ不足として返したこと。
    EXPECT_EQ(3U, integer_failures); // [確認_異常系] - 整数トークン用と整数変換用の一時領域、保存文字列の失敗を返したこと。
    EXPECT_EQ(CJSON_APP_JSONC_OUT_OF_MEMORY, normalization.status); // [確認_異常系] - 正規化用領域の確保失敗を返したこと。
    EXPECT_EQ(normalization_wipes_before, normalization_wipes); // [確認_異常系] - 正規化領域の確保前には解放領域がないこと。
    EXPECT_TRUE(replaced); // [確認_正常系] - 値の置き換えが成功したこと。
    EXPECT_EQ(0U, wipe_failure_count.load()); // [確認_正常系] - すべての解放領域が free の直前に全バイト 0 だったこと。

    // Cleanup
}

// 整列、サイズ オーバーフロー、NULL の解放と直接 cJSON 操作の確認
TEST_F(cjsonJsoncTest, checks_alignment_overflow_and_direct_cjson)
{
    // Arrange
    const char direct_text[] = "{\"name\":\"cjson\"}";

    // Act
    void *aligned = cjson_app_cJSON_malloc(1U);
    void *zero = cjson_app_cJSON_malloc(0U);
    const size_t before_null_free = wipe_count.load();
    cjson_app_cJSON_free(nullptr);
    const size_t after_null_free = wipe_count.load();
    cJSON *overflow = static_cast<cJSON *>(cjson_jsonc_test_allocate(SIZE_MAX));
    cJSON *parsed = cjson_app_cJSON_Parse(direct_text);
    cJSON *created = cjson_app_cJSON_CreateObject();
    cJSON *name = cjson_app_cJSON_CreateString("cjson");
    cJSON_bool added = cjson_app_cJSON_AddItemToObject(created, "name", name);
    char *printed = cjson_app_cJSON_PrintUnformatted(created);

    // Assert
    ASSERT_NE(nullptr, aligned); // [確認_正常系] - 1 byte を確保できること。
    EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(aligned) % alignof(std::max_align_t)); // [確認_正常系] - 確保領域が max_align_t に整列すること。
    ASSERT_NE(nullptr, zero); // [確認_正常系] - 0 byte の確保も解放可能な領域を返すこと。
    EXPECT_EQ(before_null_free, after_null_free); // [確認_正常系] - NULL の解放で標準 free を呼ばないこと。
    EXPECT_EQ(nullptr, overflow); // [確認_異常系] - サイズ加算がオーバーフローする確保を拒否すること。
    ASSERT_NE(nullptr, parsed); // [確認_正常系] - オーバーフローの後も直接解析できること。
    EXPECT_STREQ("cjson", cjson_app_cJSON_GetStringValue(cjson_app_cJSON_GetObjectItemCaseSensitive(parsed, "name"))); // [確認_正常系] - 直接解析した文字列値が保持されること。
    EXPECT_TRUE(added); // [確認_正常系] - 直接生成した木へ項目を追加できること。
    ASSERT_NE(nullptr, printed); // [確認_正常系] - 直接生成した木を出力できること。
    EXPECT_STREQ("{\"name\":\"cjson\"}", printed); // [確認_正常系] - 直接出力の JSON が期待値と一致すること。

    // Cleanup
    cjson_app_cJSON_free(aligned);
    cjson_app_cJSON_free(zero);
    cjson_app_cJSON_free(printed);
    cjson_app_cJSON_Delete(created);
    cjson_app_cJSON_Delete(parsed);
}
