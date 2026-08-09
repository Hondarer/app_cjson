#include <mock_cjson.h>

#include <cstdio>
#include <type_traits>

Mock_cjson *_mock_cjson = nullptr;

namespace
{

template <typename T> void trace_result(const char *func, const T value)
{
    if (getTraceLevel() <= TRACE_NONE)
    {
        return;
    }

    std::printf("  > %s", func);
    if (getTraceLevel() >= TRACE_DETAIL)
    {
        if constexpr (std::is_pointer_v<T>)
        {
            std::printf(" -> 0x%p\n", (const void *)value);
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            std::printf(" -> %f\n", (double)value);
        }
        else
        {
            std::printf(" -> %lld\n", (long long)value);
        }
    }
    else
    {
        std::printf("\n");
    }
}

void trace_void(const char *func)
{
    if (getTraceLevel() > TRACE_NONE)
    {
        std::printf("  > %s\n", func);
    }
}

} // namespace

#ifndef _WIN32
    #define MOCK_CJSON_IMPL(return_type, name, ...) MOCK_WEAK_IMPL(return_type, name, __VA_ARGS__)
#else
    #define MOCK_CJSON_IMPL(return_type, name, ...) extern "C" return_type CJSON_STDCALL name(__VA_ARGS__)
#endif

#define MOCK_CJSON_EXPAND(...) __VA_ARGS__

#define MOCK_CJSON_RET(return_type, name, parameters, arguments, matchers) \
    return_type delegate_real_##name parameters \
    { \
        static auto real_fn = reinterpret_cast<decltype(&name)>(resolveSharedSymbolOrExit(kLibCjsonName, #name)); \
        return real_fn arguments; \
    } \
    MOCK_CJSON_IMPL(return_type, name, MOCK_CJSON_EXPAND parameters) \
    { \
        return_type return_value; \
        if (_mock_cjson != nullptr) \
        { \
            return_value = _mock_cjson->name arguments; \
        } \
        else \
        { \
            return_value = delegate_real_##name arguments; \
        } \
        trace_result(__func__, return_value); \
        return return_value; \
    }

#define MOCK_CJSON_VOID(return_type, name, parameters, arguments, matchers) \
    return_type delegate_real_##name parameters \
    { \
        static auto real_fn = reinterpret_cast<decltype(&name)>(resolveSharedSymbolOrExit(kLibCjsonName, #name)); \
        real_fn arguments; \
    } \
    MOCK_CJSON_IMPL(return_type, name, MOCK_CJSON_EXPAND parameters) \
    { \
        if (_mock_cjson != nullptr) \
        { \
            _mock_cjson->name arguments; \
        } \
        else \
        { \
            delegate_real_##name arguments; \
        } \
        trace_void(__func__); \
    }

#include <mock_cjson_api_table.h>

#undef MOCK_CJSON_VOID
#undef MOCK_CJSON_RET
#undef MOCK_CJSON_EXPAND
#undef MOCK_CJSON_IMPL

Mock_cjson::Mock_cjson()
{
#define MOCK_CJSON_RET(return_type, name, parameters, arguments, matchers) \
    ON_CALL(*this, name matchers).WillByDefault(Invoke(delegate_real_##name));
#define MOCK_CJSON_VOID(return_type, name, parameters, arguments, matchers) \
    ON_CALL(*this, name matchers).WillByDefault(Invoke(delegate_real_##name));
#include <mock_cjson_api_table.h>
#undef MOCK_CJSON_VOID
#undef MOCK_CJSON_RET

    _mock_cjson = this;
}

Mock_cjson::~Mock_cjson()
{
    _mock_cjson = nullptr;
}
