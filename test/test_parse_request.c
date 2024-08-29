#include <string.h>
#include <stdlib.h>
// Must be included before any other local includes
#include "unity.h"

#include "test_interface.h"
#include "http.h"

TEST_INTERFACE

static TEST_STRUCT_HTTP_MESSAGE m_msg;

void setUp(void)
{
    test_interface.init(&m_msg, TEST_MESSAGE_TYPE_REQUEST);
}

void tearDown(void)
{
    test_interface.free(&m_msg);
}

void test_parse_http_message_empty_should_return_zero(void)
{
    TEST_ASSERT_EQUAL_ZERO(test_interface.parse(&m_msg, "", 0, 512));
}

void test_parse_http_message_too_short_should_return_zero(void)
{
    TEST_ASSERT_EQUAL_ZERO(test_interface.parse(&m_msg, "HT", 2, 32768));
}

void test_parse_http_message_leading_crlf_should_fail(void)
{
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL("\r\n", &m_msg);
}

void test_parse_http_message_no_headers_should_succeed(void)
{
    const char *request = {
        "GET /foo HTTP/1.0\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(request, &m_msg);
    TEST_ASSERT_EQUAL_METHOD("GET", m_msg);
    TEST_ASSERT_HTTP_SLICE("/foo", m_msg.uri, request);
    TEST_ASSERT_EQUAL_VERSION_1_0(m_msg.version);
}

void test_parse_http_message_method_max_strlen_should_succeed(void)
{
    const char *request = {
        "OPTIONS * HTTP/1.0\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(request, &m_msg);
    TEST_ASSERT_EQUAL_METHOD("OPTIONS", m_msg);
    TEST_ASSERT_HTTP_SLICE("*", m_msg.uri, request);
    TEST_ASSERT_EQUAL_VERSION_1_0(m_msg.version);
}

// cosmopolitan fails on a method of 8+ because it treats it as a 64-bit int
// uurl fails on a method of 7+ because it treats it as a char[8]
void test_parse_http_message_method_too_long_should_fail(void)
{
    const char *request = {
#ifdef TEST_UURL_PARSE
        "OPTIONSX * HTTP/1.0\r\n"
#else
        "OPTIONSXX * HTTP/1.0\r\n"
#endif
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(request, &m_msg);
}

void test_parse_http_message_method_with_invalid_token_should_fail(void)
{
    const char *request = {
        "OPT:IONS * HTTP/1.0\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(request, &m_msg);
}

void test_parse_http_message_get_1_0_should_succeed(void)
{
    const char *request = {
        "GET / HTTP/1.0\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(request, &m_msg);
    TEST_ASSERT_EQUAL_METHOD("GET", m_msg);
    TEST_ASSERT_HTTP_SLICE("/", m_msg.uri, request);
    TEST_ASSERT_EQUAL_VERSION_1_0(m_msg.version);
}

void test_parse_http_message_unknown_method_should_succeed(void)
{
    const char *request = {
        "#%*+_^ / HTTP/1.0\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(request, &m_msg);
    TEST_ASSERT_EQUAL_METHOD("#%*+_^", m_msg);
    TEST_ASSERT_HTTP_SLICE("/", m_msg.uri, request);
    TEST_ASSERT_EQUAL_VERSION_1_0(m_msg.version);
}

void test_parse_http_message_illegal_method_should_fail(void)
{
    const char *request = {
        "ehd@oruc / HTTP/1.0\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(request, &m_msg);
}

void test_parse_http_message_illegal_method_casing_gets_upper_cased_should_succeed(void)
{
    const char *request = {
        "get / HTTP/1.0\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(request, &m_msg);
    TEST_ASSERT_EQUAL_METHOD("GET", m_msg);
}

void test_parse_http_message_empty_method_should_fail(void)
{
    const char *request = {
        " / HTTP/1.0\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(request, &m_msg);
}

void test_parse_http_message_empty_uri_should_fail(void)
{
    const char *request = {
        "GET  HTTP/1.0\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(request, &m_msg);
    TEST_ASSERT_EQUAL_METHOD("GET", m_msg);
}

void test_parse_http_message_version_0_9_should_succeed(void)
{
    const char *request = {
        "GET /\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(request, &m_msg);
    TEST_ASSERT_EQUAL_METHOD("GET", m_msg);
    TEST_ASSERT_HTTP_SLICE("/", m_msg.uri, request);
    TEST_ASSERT_EQUAL_VERSION_0_9(m_msg.version);
}

void test_parse_http_message_leading_linefeeds_are_ignored_should_succeed(void)
{
    const char *request = {
        "\r\nGET /foo?bar%20hi HTTP/1.0\r\n"
        "User-Agent: hi\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(request, &m_msg);
    TEST_ASSERT_EQUAL_METHOD("GET", m_msg);
    TEST_ASSERT_HTTP_SLICE("/foo?bar%20hi", m_msg.uri, request);
    TEST_ASSERT_EQUAL_VERSION_1_0(m_msg.version);
}

void test_parse_http_message_line_folding_should_fail(void)
{
    const char *request = {
        "GET /foo?bar%20hi HTTP/1.0\r\n"
        "User-Agent: hi\r\n"
        " there\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(request, &m_msg);
}

void test_parse_http_message_empty_header_name_should_fail(void)
{
    const char *request = {
        "GET /foo?bar%20hi HTTP/1.0\r\n"
        "User-Agent: hi\r\n"
        ": hi\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(request, &m_msg);
}

void test_parse_http_message_unix_newlines_should_succeed(void)
{
    const char *request = {
        "POST /foo?bar%20hi HTTP/1.0\n"
        "Host: foo.example\n"
        "Content-Length: 0\n"
        "\n"
        "\n"
    };
    TEST_ASSERT_EQUAL(strlen(request)-1, test_interface.parse(&m_msg, request, strlen(request), strlen(request)));
    TEST_ASSERT_EQUAL_METHOD("POST", m_msg);
    TEST_ASSERT_HTTP_SLICE("/foo?bar%20hi", m_msg.uri, request);
    TEST_ASSERT_EQUAL_VERSION_1_0(m_msg.version);
    TEST_ASSERT_HTTP_SLICE("foo.example", m_msg.headers[TEST_HEADER_HOST], request);
    TEST_ASSERT_HTTP_SLICE("0", m_msg.headers[TEST_HEADER_CONTENT_LENGTH], request);
    TEST_ASSERT_HTTP_SLICE("", m_msg.headers[TEST_HEADER_ETAG], request);
}

void test_parse_http_message_chrome_should_succeed(void)
{
    const char *request = {
        "GET /tool/net/redbean.png HTTP/1.1\r\n"
        "Host: 10.10.10.124:8080\r\n"
        "Connection: keep-alive\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.90 Safari/537.36\r\n"
        "DNT:  \t1\r\n"
        "Accept: image/avif,image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8\r\n"
        "Referer: http://10.10.10.124:8080/\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Accept-Language: en-US,en;q=0.9\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(request, &m_msg);
    TEST_ASSERT_EQUAL_METHOD("GET", m_msg);
    TEST_ASSERT_HTTP_SLICE("/tool/net/redbean.png", m_msg.uri, request);
    TEST_ASSERT_EQUAL_VERSION_1_1(m_msg.version);
    TEST_ASSERT_HTTP_SLICE("10.10.10.124:8080", m_msg.headers[TEST_HEADER_HOST], request);
    TEST_ASSERT_HTTP_SLICE("1", m_msg.headers[TEST_HEADER_DNT], request);
    TEST_ASSERT_HTTP_SLICE("", m_msg.headers[TEST_HEADER_EXPECT], request);
    TEST_ASSERT_HTTP_SLICE("", m_msg.headers[TEST_HEADER_CONTENT_LENGTH], request);
    TEST_ASSERT_HTTP_SLICE("", m_msg.headers[TEST_HEADER_EXPECT], request);
}

void test_parse_http_message_xheaders_should_succeed(void)
{
    const char *request = {
        "GET /foo?bar%20hi HTTP/1.0\r\n"
        "X-User-Agent: hi\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(request, &m_msg);
    TEST_ASSERT_HTTP_SLICE("X-User-Agent", TEST_XHEADERS_SLICE_NAME(0), request);
    TEST_ASSERT_HTTP_SLICE("hi", TEST_XHEADERS_SLICE_VALUE(0), request);
}

void test_parse_http_message_normal_header_gets_replaced_should_succeed(void)
{
    const char *request = {
        "GET / HTTP/1.1\r\n"
        "Content-Type: text/html\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(request, &m_msg);
    TEST_ASSERT_HTTP_SLICE("text/plain", m_msg.headers[TEST_HEADER_CONTENT_TYPE], request);
}

void test_parse_http_message_repeated_header_should_succeed(void)
{
    const char *request = {
        "GET / HTTP/1.1\r\n"
        "Accept: text/html\r\n"
        "Accept: text/plain\r\n"
        "Accept: text/csv\r\n"
        "Accept: text/xml\r\n"
        "Accept: text/css\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(request, &m_msg);
    TEST_ASSERT_HTTP_SLICE("text/html", m_msg.headers[TEST_HEADER_ACCEPT], request);
    TEST_ASSERT_HTTP_SLICE("Accept", TEST_XHEADERS_SLICE_NAME(0), request);
    TEST_ASSERT_HTTP_SLICE("text/plain", TEST_XHEADERS_SLICE_VALUE(0), request);
    TEST_ASSERT_HTTP_SLICE("Accept", TEST_XHEADERS_SLICE_NAME(1), request);
    TEST_ASSERT_HTTP_SLICE("text/csv", TEST_XHEADERS_SLICE_VALUE(1), request);
    TEST_ASSERT_HTTP_SLICE("Accept", TEST_XHEADERS_SLICE_NAME(2), request);
    TEST_ASSERT_HTTP_SLICE("text/xml", TEST_XHEADERS_SLICE_VALUE(2), request);
    TEST_ASSERT_HTTP_SLICE("Accept", TEST_XHEADERS_SLICE_NAME(3), request);
    TEST_ASSERT_HTTP_SLICE("text/css", TEST_XHEADERS_SLICE_VALUE(3), request);
}

void test_parse_http_message_header_value_whitespace_gets_trimmed_should_succeed(void)
{
    const char *request = {
        "OPTIONS * HTTP/1.0\r\n"
        "User-Agent:  \t hi there \t \r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(request, &m_msg);
    TEST_ASSERT_HTTP_SLICE("hi there", m_msg.headers[TEST_HEADER_USER_AGENT], request);
    TEST_ASSERT_HTTP_SLICE("*", m_msg.uri, request);
}

void test_parse_http_message_absent_host_set_to_zero_should_succeed(void)
{
    const char *request = {
        "GET / HTTP/1.1\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(request, &m_msg);
    TEST_ASSERT_EQUAL_ZERO(TEST_HEADERS_SLICE_START(TEST_HEADER_HOST));
    TEST_ASSERT_EQUAL_ZERO(TEST_HEADERS_SLICE_END(TEST_HEADER_HOST));
}
void test_parse_http_message_empty_host_set_slice_to_nonzero_should_succeed(void)
{
    const char *request = {
        "GET / HTTP/1.1\r\n"
        "Host:\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(request, &m_msg);
    TEST_ASSERT_NOT_EQUAL_ZERO(TEST_HEADERS_SLICE_START(TEST_HEADER_HOST));
    TEST_ASSERT_EQUAL(TEST_HEADERS_SLICE_START(TEST_HEADER_HOST), TEST_HEADERS_SLICE_END(TEST_HEADER_HOST));
}

void test_parse_http_message_whitespace_host_set_slice_to_nonzero_should_succeed(void)
{
    const char *request = {
        "GET / HTTP/1.1\r\n"
        "Host:    \r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(request, &m_msg);
    TEST_ASSERT_NOT_EQUAL_ZERO(TEST_HEADERS_SLICE_START(TEST_HEADER_HOST));
    TEST_ASSERT_EQUAL(TEST_HEADERS_SLICE_START(TEST_HEADER_HOST), TEST_HEADERS_SLICE_END(TEST_HEADER_HOST));
}

void test_parse_http_message_with_leading_space_should_fail(void)
{
    const char *request = {
        " GET / HTTP/1.1\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(request, &m_msg);
}

void test_parse_http_uri_has_trailing_space_should_fail(void)
{
    const char *request = {
        "GET /      HTTP/1.1\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(request, &m_msg);
}

void test_parse_http_message_header_value_with_invalid_iso_8869_1_should_fail(void)
{
    const char *request = {
        "OPTIONS * HTTP/1.0\r\n"
        "User-Agent: hi there\x88\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(request, &m_msg);
}

void test_parse_http_message_header_value_with_valid_iso_8869_1_should_succeed(void)
{
    const char *request = {
        "OPTIONS * HTTP/1.0\r\n"
        "User-Agent: hi there\xFF\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(request, &m_msg);
}
