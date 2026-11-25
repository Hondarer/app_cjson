#ifndef _IN_TEST_SRC
    #include "cJSON_JSONC.c"
#endif

#include "cJSON_JSONC.inject.h"

void cjson_jsonc_test_install_hooks(void)
{
    jsonc_install_allocation_hooks();
}

size_t cjson_jsonc_test_allocation_failure_count(void)
{
    return jsonc_allocation_failure_count;
}

void cjson_jsonc_test_reset_allocation_failure_count(void)
{
    jsonc_allocation_failure_count = 0U;
}

size_t cjson_jsonc_test_allocation_header_size(void)
{
    return sizeof(jsonc_block_header);
}

void *cjson_jsonc_test_allocate(size_t size)
{
    return jsonc_hook_malloc(size);
}
