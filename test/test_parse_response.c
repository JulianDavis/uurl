#include <string.h>
#include <stdlib.h>
// Must be included before any other local includes
#include "unity.h"

#include "http.h"

// Some tests ripped off from:
//  https://github.com/nodejs/llhttp/tree/main/test/response
//  https://github.com/jart/cosmopolitan/tree/master/test/net/http

// TODO: Create neutral name and rename both?
// These are things in cosmopolitan that I have renamed in uurl
#ifndef TEST_COSMO_PARSE
#   define kHttpHost          HTTP_HEADERS_HOST
#   define kHttpContentLength HTTP_HEADERS_CONTENT_LENGTH
#endif

typedef void (*http_msg_init_t)(struct HttpMessage *, int);
typedef void (*http_msg_free_t)(struct HttpMessage *);
typedef int (*http_msg_parse_t)(struct HttpMessage *, const char *, size_t, size_t);

struct test_interface {
    http_msg_init_t init;
    http_msg_free_t free;
    http_msg_parse_t parse;
};

static struct test_interface m_test_interface;

void setUp(void) {
#ifdef TEST_COSMO_PARSE
    m_test_interface.init = InitHttpMessage;
    m_test_interface.free = DestroyHttpMessage;
    m_test_interface.parse = ParseHttpMessage;
#else // TEST_UURL
    m_test_interface.init = http_msg_init;
    m_test_interface.free = http_msg_free;
    m_test_interface.parse = http_msg_parse;
#endif
}

#define TEST_ASSERT_HTTP_SLICE(expected, slice, response) \
    do {                                                  \
        char *s = get_httpslice(response, slice);         \
        TEST_ASSERT_EQUAL_STRING(expected, s);            \
        free(s);                                          \
    } while (0);

#define TEST_ASSERT_PARSE_RESPONSE(expected, response)                               \
    do {                                                                             \
        struct HttpMessage msg = { 0 };                                              \
        m_test_interface.init(&msg, kHttpResponse);                                        \
        size_t resp_len = strlen((response));                                        \
        int bytes_parsed = m_test_interface.parse(&msg, (response), resp_len, resp_len);   \
        m_test_interface.free(&msg);                                                    \
        TEST_ASSERT_EQUAL(expected, bytes_parsed);                                   \
    } while (0);

#define TEST_ASSERT_PARSE_RESPONSE_SUCCESS(response) TEST_ASSERT_PARSE_RESPONSE(strlen((response)), (response))
#define TEST_ASSERT_PARSE_RESPONSE_FAIL(response)    TEST_ASSERT_PARSE_RESPONSE(-1, (response))

static char *get_httpslice(const char *msg, struct HttpSlice s)
{
#ifdef TEST_COSMO_PARSE
    size_t slice_len = s.b - s.a;
    const char *slice_start = msg + s.a;
#else // TEST_UURL
    size_t slice_len = s.slice_end - s.slice_start;
    const char *slice_start = msg + s.slice_start;
#endif

    char *slice = malloc(slice_len + 1); // +1 for null terminator
    TEST_ASSERT(slice);

    memcpy(slice, slice_start, slice_len);
    slice[slice_len] = '\0';
    return slice;
}

void test_parse_http_message_incomplete_http_protocol_should_fail(void)
{
    const char *response = {
        "HTP/1.1 200 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_FAIL(response);
}

void test_parse_http_message_extra_digit_major_version_should_fail(void)
{
    const char *response = {
        "HTTP/01.1 200 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_FAIL(response);
}

void test_parse_http_message_extra_digit_major_version2_should_fail(void)
{
    const char *response = {
        "HTTP/11.1 200 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_FAIL(response);
}

void test_parse_http_message_extra_digit_minor_version_should_fail(void)
{
    const char *response = {
        "HTTP/1.01 200 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_FAIL(response);
}

void test_parse_http_message_tab_after_version_should_fail(void)
{
    const char *response = {
        "HTTP/1.1\t200 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_FAIL(response);
}

void test_parse_http_message_headers_separated_by_cr_should_fail(void)
{
    const char *response = {
        "HTTP/1.1 200 OK\r\n"
        "Foo: 1\r"
        "Bar: 2\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_FAIL(response);
}

void test_parse_http_message_whitespace_before_name_should_fail(void)
{
    const char *response = {
        "HTTP/1.1 200 OK\r\n"
        " Host: foo\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_FAIL(response);
}

void test_parse_http_message_single_digit_status_code_should_fail(void)
{
    const char *response = {
        "HTTP/1.1 2 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_FAIL(response);
}

void test_parse_http_message_double_digit_status_code_should_fail(void)
{
    const char *response = {
        "HTTP/1.1 20 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_FAIL(response);
}

void test_parse_http_message_quad_digit_status_code_should_fail(void)
{
    const char *response = {
        "HTTP/1.1 2000 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_FAIL(response);
}

void test_parse_http_message_missing_lf_should_fail(void)
{
    const char *response = {
        "HTTP/1.1 200 OK\r"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_FAIL(response);
}

void test_parse_http_message_missing_status_message_should_succeed(void)
{
    const char *response = {
        "HTTP/1.1 200\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_SUCCESS(response);
}

void test_parse_http_message_empty_status_message_should_succeed(void)
{
    const char *response = {
        "HTTP/1.1 200 \r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_SUCCESS(response);
}

void test_parse_http_message_invalid_version_digits_should_succeed(void)
{
    const char *response = {
        "HTTP/4.2 200 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_SUCCESS(response);
}

void test_parse_http_message_extra_space_between_version_and_status_code_should_succeed(void)
{
    const char *response = {
        "HTTP/1.1 200  OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_SUCCESS(response);
}

void test_parse_http_message_tiniest_http_response_should_succeed(void)
{
    const char *response = {
        "HTTP/1.0 200 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_SUCCESS(response);
}

void test_parse_http_message_tiny_http_response_should_succeed(void)
{
    const char *response = {
        "HTTP/1.0 200 OK\r\n"
        "Accept-Encoding: gzip\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_SUCCESS(response);
}

void test_parse_http_message_standard_http_response_should_succeed(void)
{
    const char *response = {
        "HTTP/1.1 200 OK\r\n"
        "Server: nginx\r\n"
        "Date: Sun, 27 Jun 2021 19:09:59 GMT\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: keep-alive\r\n"
        "Vary: Accept-Encoding\r\n"
        "Cache-Control: private; max-age=0\r\n"
        "X-Frame-Options: DENY\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "X-XSS-Protection: 1; mode=block\r\n"
        "Referrer-Policy: origin\r\n"
        "Strict-Transport-Security: max-age=31556900\r\n"
        "Content-Security-Policy: default-src 'self'; script-src 'self' 'unsafe-inline' https://www.google.com/recaptcha/ https://www.gstatic.com/recaptcha/ https://cdnjs.cloudflare.com/; frame-src 'self' https://www.google.com/recaptcha/; style-src 'self' 'unsafe-inline'\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_SUCCESS(response);
}

void test_parse_http_message_unstandard_http_response_should_succeed(void)
{
    const char *response = {
        "HTTP/1.1 200 OK\r\n"
        "date: Sun, 27 Jun 2021 19:00:36 GMT\r\n"
        "server: Apache\r\n"
        "x-frame-options: SAMEORIGIN\r\n"
        "x-xss-protection: 0\r\n"
        "vary: Accept-Encoding\r\n"
        "referrer-policy: no-referrer\r\n"
        "x-slack-backend: r\r\n"
        "strict-transport-security: max-age=31536000; includeSubDomains; preload\r\n"
        "set-cookie: b=5aboacm0axrlzntx5wfec7r42; expires=Fri, 27-Jun-2031 19:00:36 GMT; Max-Age=315532800; path=/; domain=.slack.com; secure; SameSite=None\r\n"
        "set-cookie: x=5aboacm0axrlzntx5wfec7r42.1624820436; expires=Sun, 27-Jun-2021 19:15:36 GMT; Max-Age=900; path=/; domain=.slack.com; secure; SameSite=None\r\n"
        "content-type: text/html; charset=utf-8\r\n"
        "x-envoy-upstream-service-time: 19\r\n"
        "x-backend: main_normal main_canary_with_overflow main_control_with_overflow\r\n"
        "x-server: slack-www-hhvm-main-iad-a9ic\r\n"
        "x-via: envoy-www-iad-qd3r, haproxy-edge-pdx-klqo\r\n"
        "x-slack-shared-secret-outcome: shared-secret\r\n"
        "via: envoy-www-iad-qd3r\r\n"
        "transfer-encoding: chunked\r\n"
        "\r\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_SUCCESS(response);
}

void test_parse_http_message_only_lf_should_succeed(void)
{
    const char *response = {
        "HTTP/1.0 200 OK\n"
        "Host: foo.example\n"
        "Content-Length: 0\n"
        "\n"
    };
    TEST_ASSERT_PARSE_RESPONSE_SUCCESS(response);
}

// TODO: Create a TEST_ASSERT_PARSE_RESPONSE_ZERO?
void test_parse_http_message_empty_should_return_zero(void)
{
    struct HttpMessage msg = { 0 };
    m_test_interface.init(&msg, kHttpResponse);

    TEST_ASSERT_EQUAL(0, m_test_interface.parse(&msg, "", 0, 32768));
    m_test_interface.free(&msg);
}

void test_parse_http_message_short_should_return_zero(void)
{
    const char *response = "HTTP";
    size_t resp_len = strlen(response);

    struct HttpMessage msg = { 0 };
    m_test_interface.init(&msg, kHttpResponse);

    TEST_ASSERT_EQUAL(0, m_test_interface.parse(&msg, response, resp_len, 32768));
    m_test_interface.free(&msg);
}

void test_parse_http_message_http_v1_1_should_succeed(void)
{
    const char *response = {
        "HTTP/1.0 404 Not Found\r\n"
        "\r\n"
    };
    size_t resp_len = strlen(response);

    struct HttpMessage msg = { 0 };
    m_test_interface.init(&msg, kHttpResponse);

    int bytes_parsed = m_test_interface.parse(&msg, response, resp_len, resp_len);
    TEST_ASSERT_EQUAL(resp_len, bytes_parsed);
    m_test_interface.free(&msg);

    TEST_ASSERT_EQUAL(404, msg.status);
    TEST_ASSERT_HTTP_SLICE("Not Found", msg.message, response);
    TEST_ASSERT_EQUAL_UINT8(10, msg.version);

}

void test_parse_http_message_http_v1_0_should_succeed(void)
{
    const char *response = {
        "HTTP/1.1 404 Not Found\r\n"
        "\r\n"
    };
    size_t resp_len = strlen(response);

    struct HttpMessage msg = { 0 };
    m_test_interface.init(&msg, kHttpResponse);

    int bytes_parsed = m_test_interface.parse(&msg, response, resp_len, resp_len);
    TEST_ASSERT_EQUAL(resp_len, bytes_parsed);

    TEST_ASSERT_EQUAL(404, msg.status);
    TEST_ASSERT_HTTP_SLICE("Not Found", msg.message, response);
    TEST_ASSERT_EQUAL_UINT8(11, msg.version);
}

void test_parse_http_message_no_headers_should_succeed(void)
{
    const char *response = {
        "HTTP/1.0 200 OK\r\n"
        "\r\n"
    };
    size_t resp_len = strlen(response);

    struct HttpMessage msg = { 0 };
    m_test_interface.init(&msg, kHttpResponse);

    int bytes_parsed = m_test_interface.parse(&msg, response, resp_len, resp_len);
    TEST_ASSERT_EQUAL(resp_len, bytes_parsed);

    TEST_ASSERT_EQUAL(200, msg.status);
    TEST_ASSERT_HTTP_SLICE("OK", msg.message, response);
    TEST_ASSERT_EQUAL_UINT8(10, msg.version);

    m_test_interface.free(&msg);
}

void test_parse_http_message_some_headers_should_succeed(void)
{
    const char *response = {
        "HTTP/1.0 200 OK\r\n"
        "Host: foo.example\r\n"
        "Content-Length: 0\r\n"
        "\r\n"
    };
    size_t resp_len = strlen(response);

    struct HttpMessage msg = { 0 };
    m_test_interface.init(&msg, kHttpResponse);

    int bytes_parsed = m_test_interface.parse(&msg, response, resp_len, resp_len);
    TEST_ASSERT_EQUAL(resp_len, bytes_parsed);

    TEST_ASSERT_EQUAL(200, msg.status);
    TEST_ASSERT_HTTP_SLICE("OK", msg.message, response);
    TEST_ASSERT_EQUAL_UINT8(10, msg.version);
    TEST_ASSERT_HTTP_SLICE("foo.example", msg.headers[kHttpHost], response);
    TEST_ASSERT_HTTP_SLICE("0", msg.headers[kHttpContentLength], response);
    m_test_interface.free(&msg);
}
