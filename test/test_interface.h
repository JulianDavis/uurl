#pragma once

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "http.h"

// These are various values or struct fields that have been renamed or retyped
#ifdef TEST_COSMO_PARSE
#include "libc/serialize.h"
#   define TEST_STRUCT_SLICE                       struct HttpSlice
#   define TEST_STRUCT_HTTP_MESSAGE                struct HttpMessage
#   define TEST_MESSAGE_TYPE_REQUEST               kHttpRequest
#   define TEST_MESSAGE_TYPE_RESPONSE              kHttpResponse
#   define TEST_HEADER_HOST                        kHttpHost
#   define TEST_HEADER_CONTENT_LENGTH              kHttpContentLength
#   define TEST_HEADER_ETAG                        kHttpEtag
#   define TEST_HEADER_DNT                         kHttpDnt
#   define TEST_HEADER_EXPECT                      kHttpExpect
#   define TEST_HEADER_CONTENT_TYPE                kHttpContentType
#   define TEST_HEADER_ACCEPT                      kHttpAccept
#   define TEST_HEADER_USER_AGENT                  kHttpUserAgent
#   define TEST_ASSERT_EQUAL_METHOD(expected, msg)      \
        do {                                            \
            char actual[9] = { 0 };                     \
            WRITE64LE(actual, msg.method);              \
            TEST_ASSERT_EQUAL_STRING(expected, actual); \
        } while (0);
#   define TEST_ASSERT_EQUAL_VERSION_0_9(actual)   TEST_ASSERT_EQUAL_UINT8(9, actual)
#   define TEST_ASSERT_EQUAL_VERSION_1_0(actual)   TEST_ASSERT_EQUAL_UINT8(10, actual)
#   define TEST_ASSERT_EQUAL_VERSION_1_1(actual)   TEST_ASSERT_EQUAL_UINT8(11, actual)
#   define TEST_HEADERS_SLICE_START(i)             m_msg.headers[(i)].a
#   define TEST_HEADERS_SLICE_END(i)               m_msg.headers[(i)].b
#   define TEST_XHEADERS_SLICE_NAME(i)             m_msg.xheaders.p[(i)].k
#   define TEST_XHEADERS_SLICE_VALUE(i)            m_msg.xheaders.p[(i)].v
#else
#   define TEST_STRUCT_SLICE                       struct http_slice
#   define TEST_STRUCT_HTTP_MESSAGE                struct http_message
#   define TEST_MESSAGE_TYPE_REQUEST               HTTP_MESSAGE_TYPE_REQUEST
#   define TEST_MESSAGE_TYPE_RESPONSE              HTTP_MESSAGE_TYPE_RESPONSE
#   define TEST_HEADER_HOST                        HTTP_HEADERS_HOST
#   define TEST_HEADER_CONTENT_LENGTH              HTTP_HEADERS_CONTENT_LENGTH
#   define TEST_HEADER_ETAG                        HTTP_HEADERS_ETAG
#   define TEST_HEADER_DNT                         HTTP_HEADERS_DNT
#   define TEST_HEADER_EXPECT                      HTTP_HEADERS_EXPECT
#   define TEST_HEADER_CONTENT_TYPE                HTTP_HEADERS_CONTENT_TYPE
#   define TEST_HEADER_ACCEPT                      HTTP_HEADERS_ACCEPT
#   define TEST_HEADER_USER_AGENT                  HTTP_HEADERS_USER_AGENT
#   define TEST_ASSERT_EQUAL_METHOD(expected, msg) TEST_ASSERT_EQUAL_STRING (expected, msg.method)
#   define TEST_ASSERT_EQUAL_VERSION_0_9(actual)   TEST_ASSERT_EQUAL_INT8(HTTP_VERSION_0_9, actual)
#   define TEST_ASSERT_EQUAL_VERSION_1_0(actual)   TEST_ASSERT_EQUAL_INT8(HTTP_VERSION_1_0, actual)
#   define TEST_ASSERT_EQUAL_VERSION_1_1(actual)   TEST_ASSERT_EQUAL_INT8(HTTP_VERSION_1_1, actual)
#   define TEST_HEADERS_SLICE_START(i)             m_msg.headers[(i)].start
#   define TEST_HEADERS_SLICE_END(i)               m_msg.headers[(i)].end
#   define TEST_XHEADERS_SLICE_NAME(i)             m_msg.xheaders.headers[(i)].name
#   define TEST_XHEADERS_SLICE_VALUE(i)            m_msg.xheaders.headers[(i)].value
#endif

// Include this in any test file that is shared between cosmopolitan and uurl. This provides you with the
// `test_interface` symbol which exposes the struct of the same name.
//
// This allows you to call init, free, or parse from either implementation but from the same test.
//
// TEST_COSMO_PARSE must be defined on the build environment to use the cosmopolitan interface, otherwise it will
// default to the uurl interface.
#define TEST_INTERFACE                                          \
    static struct test_interface test_interface = { 0 };        \
    __attribute__((constructor)) void test_interface_ctor(void) \
    {                                                           \
        test_interface_init(&test_interface);                   \
    };                                                          \

typedef void (*http_msg_init_t)(TEST_STRUCT_HTTP_MESSAGE *, int);
typedef void (*http_msg_free_t)(TEST_STRUCT_HTTP_MESSAGE *);
typedef int (*http_msg_parse_t)(TEST_STRUCT_HTTP_MESSAGE *, const char *, size_t, size_t);

struct test_interface {
    http_msg_init_t init;
    http_msg_free_t free;
    http_msg_parse_t parse;
};

// Common test macros
#define TEST_ASSERT_EQUAL_ZERO(actual)     TEST_ASSERT_EQUAL(0, (actual))
#define TEST_ASSERT_NOT_EQUAL_ZERO(actual) TEST_ASSERT_NOT_EQUAL(0, (actual))

// Common HTTP parse macros
#define TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(input, msg) TEST_ASSERT_EQUAL(strlen((input)), test_interface.parse((msg), (input), strlen((input)), strlen((input))))
#define TEST_ASSERT_HTTP_MSG_PARSE_FAIL(input, msg)    TEST_ASSERT_EQUAL(-1, test_interface.parse((msg), (input), strlen((input)), strlen((input))))

#define TEST_ASSERT_HTTP_SLICE(expected, slice, msg) \
    do {                                             \
        char *s = http_slice_new((msg), (slice));    \
        TEST_ASSERT_EQUAL_STRING((expected), s);     \
        free(s);                                     \
    } while (0);

// Initialize a test interface
void test_interface_init(struct test_interface *test_interface);

// Get the value of an HTTP slice
char *http_slice_new(const char *msg, TEST_STRUCT_SLICE s);
