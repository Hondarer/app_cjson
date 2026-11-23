#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    /* testfw.h の using namespace std より後に windows.h を置くと、std::byte と rpcndr.h の byte が衝突する。 */
    #ifdef byte
        #undef byte
    #endif
#else
    #include <dlfcn.h>
#endif

#include <testfw.h>

#include <cJSON_JSONC.h>

#include <cstring>
#include <string>
#include <thread>

namespace
{

struct LoadedLibrary
{
#if defined(_WIN32)
    HMODULE module;
#else
    void *module;
#endif
};

struct ThreadResult
{
    cjson_app_jsonc_status status;
    int value;
};

LoadedLibrary open_cjson_library(const std::string &path)
{
    LoadedLibrary loaded = {};
#if defined(_WIN32)
    loaded.module = LoadLibraryA(path.c_str());
#else
    loaded.module = dlopen(path.c_str(), RTLD_NOW);
#endif
    return loaded;
}

void close_cjson_library(LoadedLibrary loaded)
{
#if defined(_WIN32)
    if ((loaded.module != nullptr) && (FreeLibrary(loaded.module) == 0))
    {
        return;
    }
#else
    if ((loaded.module != nullptr) && (dlclose(loaded.module) != 0))
    {
        return;
    }
#endif
}

void *lookup_symbol(LoadedLibrary loaded, const char *name)
{
#if defined(_WIN32)
    return reinterpret_cast<void *>(GetProcAddress(loaded.module, name));
#else
    return dlsym(loaded.module, name);
#endif
}

using parse_opts_fn = cJSON *(CJSON_STDCALL *)(const char *, size_t, cjson_app_jsonc_status *, size_t *);
using delete_fn = void(CJSON_STDCALL *)(cJSON *);
using get_item_fn = cJSON *(CJSON_STDCALL *)(const cJSON *, const char *);

void parse_on_thread(parse_opts_fn parse_opts, delete_fn delete_tree, get_item_fn get_item, const char *input,
                     ThreadResult *result)
{
    result->status = CJSON_APP_JSONC_INVALID_ARGUMENT;
    result->value = -1;
    cJSON *root = parse_opts(input, std::strlen(input), &result->status, nullptr);
    if (root != nullptr)
    {
        cJSON *value = get_item(root, "v");
        if (value != nullptr)
        {
            result->value = value->valueint;
        }
        delete_tree(root);
    }
}

} // namespace

// 動的に読み込んだ libcjson_app の公開 API を複数スレッドから呼び出せることの確認
TEST(cjsonJsoncDynamicLoadTest, public_api_works_on_threads_after_dynamic_load)
{
    // Arrange
    // Windows Vista 以降は、LoadLibrary した DLL の __declspec(thread) を使える。
    // see: https://learn.microsoft.com/en-us/cpp/parallel/thread-local-storage-tls
    std::string path = findWorkspaceRoot() + "/app/cjson/prod/lib/libcjson_app" +
                       TESTFW_SHARED_LIBRARY_EXTENSION;
    LoadedLibrary loaded = open_cjson_library(path);
    ASSERT_NE(nullptr, loaded.module); // [状態確認] - libcjson_app を実行時に読み込めたこと。
    auto parse_opts = reinterpret_cast<parse_opts_fn>(lookup_symbol(loaded, "cjson_app_cJSON_ParseJSONCWithLengthOpts"));
    auto delete_tree = reinterpret_cast<delete_fn>(lookup_symbol(loaded, "cjson_app_cJSON_Delete"));
    auto get_item = reinterpret_cast<get_item_fn>(lookup_symbol(loaded, "cjson_app_cJSON_GetObjectItemCaseSensitive"));
    ASSERT_NE(nullptr, parse_opts); // [状態確認] - 状態付き解析 API がエクスポートされていること。
    ASSERT_NE(nullptr, delete_tree); // [状態確認] - 木の解放 API がエクスポートされていること。
    ASSERT_NE(nullptr, get_item); // [状態確認] - オブジェクト項目の取得 API がエクスポートされていること。
    ThreadResult first_result = {CJSON_APP_JSONC_INVALID_ARGUMENT, -1};
    ThreadResult second_result = {CJSON_APP_JSONC_INVALID_ARGUMENT, -1};

    // Act
    std::thread first(parse_on_thread, parse_opts, delete_tree, get_item, "{\"v\":1}", &first_result);
    std::thread second(parse_on_thread, parse_opts, delete_tree, get_item, "{\"v\":2}", &second_result);
    first.join();
    second.join();

    // Assert
    EXPECT_EQ(CJSON_APP_JSONC_OK, first_result.status); // [確認_正常系] - 動的読み込み後の 1 つ目のスレッドで解析が成功したこと。
    EXPECT_EQ(1, first_result.value); // [確認_正常系] - 1 つ目のスレッドの解析値が保持されたこと。
    EXPECT_EQ(CJSON_APP_JSONC_OK, second_result.status); // [確認_正常系] - 動的読み込み後の 2 つ目のスレッドで解析が成功したこと。
    EXPECT_EQ(2, second_result.value); // [確認_正常系] - 2 つ目のスレッドの解析値が保持されたこと。

    // Cleanup
    close_cjson_library(loaded);
}
