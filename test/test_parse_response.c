#include <string.h>
#include <stdlib.h>
// Must be included before any other local includes
#include "unity.h"

#include "test_interface.h"
#include "http.h"

TEST_INTERFACE

// Some tests ripped off from:
//  https://github.com/jart/cosmopolitan/tree/master/test/net/http
//  https://github.com/nodejs/llhttp/tree/main/test/response

static TEST_STRUCT_HTTP_MESSAGE m_msg;

void setUp(void)
{
    test_interface.init(&m_msg, TEST_MESSAGE_TYPE_RESPONSE);
}

void tearDown(void)
{
    test_interface.free(&m_msg);
}

void test_parse_http_message_empty_should_return_zero(void)
{
    TEST_ASSERT_EQUAL_ZERO(test_interface.parse(&m_msg, "", 0, 32768));
}

void test_parse_http_message_too_short_should_return_zero(void)
{
    TEST_ASSERT_EQUAL_ZERO(test_interface.parse(&m_msg, "HT", 2, 32768));
}

void test_parse_http_message_tiniest_http_response_should_succeed(void)
{
    const char *response = {
        "HTTP/1.0 200 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(response, &m_msg);
}

void test_parse_http_message_tiny_http_response_should_succeed(void)
{
    const char *response = {
        "HTTP/1.0 200 OK\r\n"
        "Accept-Encoding: gzip\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(response, &m_msg);
}

void test_parse_http_message_missing_status_message_should_succeed(void)
{
    const char *response = {
        "HTTP/1.1 200\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(response, &m_msg);
}

void test_parse_http_message_empty_status_message_should_succeed(void)
{
    const char *response = {
        "HTTP/1.1 200 \r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(response, &m_msg);
}

void test_parse_http_message_invalid_version_digits_should_succeed(void)
{
    TEST_IGNORE_MESSAGE("Decide if I want to allow this or not. Cosmo currently does and that's where this test comes from. I don't like the idea of parsing an unknown version and then making assertions about its formatting, etc");
    const char *response = {
        "HTTP/4.2 200 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(response, &m_msg);
}

void test_parse_http_message_extra_space_between_version_and_status_code_should_succeed(void)
{
    const char *response = {
        "HTTP/1.0 200  OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(response, &m_msg);
}

void test_parse_http_message_http_v1_1_should_succeed(void)
{
    const char *response = {
        "HTTP/1.1 404 Not Found\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(response, &m_msg);
    TEST_ASSERT_EQUAL(404, m_msg.status);
    TEST_ASSERT_HTTP_SLICE("Not Found", m_msg.message, response);
    TEST_ASSERT_EQUAL_VERSION_1_1(m_msg.version);
}

void test_parse_http_message_http_v1_0_should_succeed(void)
{
    const char *response = {
        "HTTP/1.0 404 Not Found\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(response, &m_msg);
    TEST_ASSERT_EQUAL(404, m_msg.status);
    TEST_ASSERT_HTTP_SLICE("Not Found", m_msg.message, response);
    TEST_ASSERT_EQUAL_VERSION_1_0(m_msg.version);
}

void test_parse_http_message_no_headers_should_succeed(void)
{
    const char *response = {
        "HTTP/1.0 200 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(response, &m_msg);
    TEST_ASSERT_EQUAL(200, m_msg.status);
    TEST_ASSERT_HTTP_SLICE("OK", m_msg.message, response);
    TEST_ASSERT_EQUAL_VERSION_1_0(m_msg.version);
}

void test_parse_http_message_some_headers_should_succeed(void)
{
    const char *response = {
        "HTTP/1.0 200 OK\r\n"
        "Host: foo.example\r\n"
        "Content-Length: 0\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(response, &m_msg);
    TEST_ASSERT_EQUAL(200, m_msg.status);
    TEST_ASSERT_HTTP_SLICE("OK", m_msg.message, response);
    TEST_ASSERT_EQUAL_VERSION_1_0(m_msg.version);
    TEST_ASSERT_HTTP_SLICE("foo.example", m_msg.headers[TEST_HEADER_HOST], response);
    TEST_ASSERT_HTTP_SLICE("0", m_msg.headers[TEST_HEADER_CONTENT_LENGTH], response);
}

void test_parse_http_message_only_lf_should_succeed(void)
{
    const char *response = {
        "HTTP/1.0 200 OK\n"
        "Host: foo.example\n"
        "Content-Length: 0\n"
        "\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(response, &m_msg);
    TEST_ASSERT_EQUAL(200, m_msg.status);
    TEST_ASSERT_HTTP_SLICE("OK", m_msg.message, response);
    TEST_ASSERT_EQUAL_VERSION_1_0(m_msg.version);
    TEST_ASSERT_HTTP_SLICE("foo.example", m_msg.headers[TEST_HEADER_HOST], response);
    TEST_ASSERT_HTTP_SLICE("0", m_msg.headers[TEST_HEADER_CONTENT_LENGTH], response);
}

void test_parse_http_message_xheaders_should_succeed(void)
{
    const char *response = {
        "HTTP/1.0 200 OK\r\n"
        "X-User-Agent: hi\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(response, &m_msg);
    TEST_ASSERT_EQUAL(200, m_msg.status);
    TEST_ASSERT_HTTP_SLICE("OK", m_msg.message, response);
    TEST_ASSERT_EQUAL_VERSION_1_0(m_msg.version);
    TEST_ASSERT_HTTP_SLICE("X-User-Agent", TEST_XHEADERS_SLICE_NAME(0), response);
    TEST_ASSERT_HTTP_SLICE("hi", TEST_XHEADERS_SLICE_VALUE(0), response);
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
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(response, &m_msg);
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
    TEST_ASSERT_HTTP_MSG_PARSE_SUCCESS(response, &m_msg);
}

void test_parse_http_message_incomplete_http_protocol_should_fail(void)
{
    const char *response = {
        "HTP/1.1 200 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(response, &m_msg);
}

void test_parse_http_message_extra_digit_major_version_should_fail(void)
{
    const char *response = {
        "HTTP/01.1 200 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(response, &m_msg);
}

void test_parse_http_message_extra_digit_major_version2_should_fail(void)
{
    const char *response = {
        "HTTP/11.1 200 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(response, &m_msg);
}

void test_parse_http_message_extra_digit_minor_version_should_fail(void)
{
    const char *response = {
        "HTTP/1.01 200 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(response, &m_msg);
}

void test_parse_http_message_tab_after_version_should_fail(void)
{
    const char *response = {
        "HTTP/1.1\t200 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(response, &m_msg);
}

void test_parse_http_message_headers_separated_by_cr_should_fail(void)
{
    const char *response = {
        "HTTP/1.1 200 OK\r\n"
        "Foo: 1\r"
        "Bar: 2\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(response, &m_msg);
}

void test_parse_http_message_whitespace_before_name_should_fail(void)
{
    const char *response = {
        "HTTP/1.1 200 OK\r\n"
        " Host: foo\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(response, &m_msg);
}

void test_parse_http_message_single_digit_status_code_should_fail(void)
{
    const char *response = {
        "HTTP/1.1 2 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(response, &m_msg);
}

void test_parse_http_message_double_digit_status_code_should_fail(void)
{
    const char *response = {
        "HTTP/1.1 20 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(response, &m_msg);
}

void test_parse_http_message_quad_digit_status_code_should_fail(void)
{
    const char *response = {
        "HTTP/1.1 2000 OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(response, &m_msg);
}

void test_parse_http_message_missing_lf_should_fail(void)
{
    const char *response = {
        "HTTP/1.1 200 OK\r"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(response, &m_msg);
}

void test_parse_http_message_non_digit_status_should_fail(void)
{
    const char *response = {
        "HTTP/1.1 ABC\r"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(response, &m_msg);
}

void test_parse_http_message_leading_space_should_fail(void)
{
    const char *response = {
        " HTTP/1.0 200  OK\r\n"
        "\r\n"
    };
    TEST_ASSERT_HTTP_MSG_PARSE_FAIL(response, &m_msg);
}
