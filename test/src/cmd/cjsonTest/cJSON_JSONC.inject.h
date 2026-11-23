#ifndef CJSON_JSONC_INJECT_H
#define CJSON_JSONC_INJECT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void cjson_jsonc_test_install_hooks(void);
    size_t cjson_jsonc_test_allocation_failure_count(void);
    void cjson_jsonc_test_reset_allocation_failure_count(void);
    size_t cjson_jsonc_test_allocation_header_size(void);
    void *cjson_jsonc_test_allocate(size_t size);
    int cjson_jsonc_test_parse_lock_is_available(void);

#ifdef __cplusplus
}
#endif

#endif /* CJSON_JSONC_INJECT_H */
