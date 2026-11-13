#define MOCK_CJSON_DISABLE_CALLER_INJECTION
#include <mock_cjson.h>
#undef MOCK_CJSON_DISABLE_CALLER_INJECTION

#include <cstdio>
#include <type_traits>

Mock_cjson *_mock_cjson = nullptr;

namespace
{

template <typename T> void trace_result(const char *func, const char *caller_file, const int caller_line,
                                       const char *caller_function, const T value)
{
    if (getTraceLevel() <= TRACE_NONE)
    {
        return;
    }

    std::printf("  > %s (%s:%d %s)", func, caller_file, caller_line, caller_function);
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

void trace_void(const char *func, const char *caller_file, const int caller_line, const char *caller_function)
{
    if (getTraceLevel() > TRACE_NONE)
    {
        std::printf("  > %s (%s:%d %s)\n", func, caller_file, caller_line, caller_function);
    }
}

} // namespace

#ifndef _WIN32
    #define MOCK_CJSON_IMPL(return_type, name, ...) MOCK_WEAK_IMPL(return_type, name, __VA_ARGS__)
#else
    #define MOCK_CJSON_IMPL(return_type, name, ...) extern "C" return_type CJSON_STDCALL name(__VA_ARGS__)
#endif

#define MOCK_CJSON_EXPAND(...) __VA_ARGS__

#define MOCK_CJSON_RET(return_type, original_name, api_name, parameters, arguments, mock_parameters, mock_arguments, matchers) \
    return_type delegate_real_##api_name mock_parameters \
    { \
        (void)caller_file; \
        (void)caller_line; \
        (void)caller_function; \
        static auto real_fn = reinterpret_cast<decltype(&api_name)>(resolveSharedSymbolOrExit(kLibCjsonName, #api_name)); \
        return real_fn arguments; \
    } \
    MOCK_CJSON_IMPL(return_type, api_name, MOCK_CJSON_EXPAND parameters) \
    { \
        const char *caller_file = "<unknown>"; \
        const int caller_line = 0; \
        const char *caller_function = "<unknown>"; \
        return mock_##api_name mock_arguments; \
    } \
    extern "C" return_type mock_##api_name mock_parameters \
    { \
        return_type return_value; \
        if (_mock_cjson != nullptr) \
            return_value = _mock_cjson->api_name mock_arguments; \
        else \
            return_value = delegate_real_##api_name mock_arguments; \
        trace_result(#api_name, caller_file, caller_line, caller_function, return_value); \
        return return_value; \
    }

#define MOCK_CJSON_VOID(return_type, original_name, api_name, parameters, arguments, mock_parameters, mock_arguments, matchers) \
    return_type delegate_real_##api_name mock_parameters \
    { \
        (void)caller_file; \
        (void)caller_line; \
        (void)caller_function; \
        static auto real_fn = reinterpret_cast<decltype(&api_name)>(resolveSharedSymbolOrExit(kLibCjsonName, #api_name)); \
        real_fn arguments; \
    } \
    MOCK_CJSON_IMPL(return_type, api_name, MOCK_CJSON_EXPAND parameters) \
    { \
        const char *caller_file = "<unknown>"; \
        const int caller_line = 0; \
        const char *caller_function = "<unknown>"; \
        mock_##api_name mock_arguments; \
    } \
    extern "C" return_type mock_##api_name mock_parameters \
    { \
        if (_mock_cjson != nullptr) \
            _mock_cjson->api_name mock_arguments; \
        else \
            delegate_real_##api_name mock_arguments; \
        trace_void(#api_name, caller_file, caller_line, caller_function); \
    }

#include <mock_cjson_api_table.h>

#undef MOCK_CJSON_VOID
#undef MOCK_CJSON_RET
#undef MOCK_CJSON_EXPAND
#undef MOCK_CJSON_IMPL

Mock_cjson::Mock_cjson()
{
#define MOCK_CJSON_RET(return_type, original_name, api_name, parameters, arguments, mock_parameters, mock_arguments, matchers) \
    ON_CALL(*this, api_name matchers).WillByDefault(Invoke(delegate_real_##api_name));
#define MOCK_CJSON_VOID(return_type, original_name, api_name, parameters, arguments, mock_parameters, mock_arguments, matchers) \
    ON_CALL(*this, api_name matchers).WillByDefault(Invoke(delegate_real_##api_name));
#include <mock_cjson_api_table.h>
#undef MOCK_CJSON_VOID
#undef MOCK_CJSON_RET

    TESTFW_REGISTER_MOCK_INSTANCE(_mock_cjson);
}

Mock_cjson::~Mock_cjson()
{
    TESTFW_UNREGISTER_MOCK_INSTANCE(_mock_cjson);
}
