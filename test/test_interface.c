#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "unity.h"
#include "test_interface.h"
#include "http.h"

void test_interface_init(struct test_interface *test_interface)
{
    if (!test_interface)
        return;

#ifdef TEST_COSMO_PARSE
        test_interface->init = InitHttpMessage;
        test_interface->free = DestroyHttpMessage;
        test_interface->parse = ParseHttpMessage;
#else // TEST_UURL
        test_interface->init = http_msg_init;
        test_interface->free = http_msg_free;
        test_interface->parse = http_msg_parse;
#endif
}

char *http_slice_new(const char *msg, TEST_STRUCT_SLICE s)
{
#ifdef TEST_COSMO_PARSE
    size_t slice_len = s.b - s.a;
    const char *slice_start = msg + s.a;
#else // TEST_UURL
    size_t slice_len = s.end - s.start;
    const char *slice_start = msg + s.start;
#endif

    char *slice = malloc(slice_len + 1); // +1 for null terminator
    TEST_ASSERT(slice);

    memcpy(slice, slice_start, slice_len);
    slice[slice_len] = '\0';
    return slice;
}
