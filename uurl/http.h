
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

// strlen of the longest possible HTTP method: "OPTIONS" or "CONNECT"
#define HTTP_METHOD_MAX_STRLEN 7

// Supported HTTP message types
enum http_message_types {
    HTTP_MESSAGE_TYPE_UNKNOWN = -1,
    HTTP_MESSAGE_TYPE_REQUEST,
    HTTP_MESSAGE_TYPE_RESPONSE,
};

// Supported HTTP versions
enum http_versions {
    HTTP_VERSION_UNKNOWN = -1,
    HTTP_VERSION_0_9,
    HTTP_VERSION_1_0,
    HTTP_VERSION_1_1,
};

// Common HTTP headers
enum http_headers {
    HTTP_HEADERS_UNKNOWN = -1,
    HTTP_HEADERS_HOST,
    HTTP_HEADERS_CACHE_CONTROL,
    HTTP_HEADERS_CONNECTION,
    HTTP_HEADERS_ACCEPT,
    HTTP_HEADERS_ACCEPT_LANGUAGE,
    HTTP_HEADERS_ACCEPT_ENCODING,
    HTTP_HEADERS_USER_AGENT,
    HTTP_HEADERS_REFERER,
    HTTP_HEADERS_X_FORWARDED_FOR,
    HTTP_HEADERS_ORIGIN,
    HTTP_HEADERS_UPGRADE_INSECURE_REQUESTS,
    HTTP_HEADERS_PRAGMA,
    HTTP_HEADERS_COOKIE,
    HTTP_HEADERS_DNT,
    HTTP_HEADERS_SEC_GPC,
    HTTP_HEADERS_FROM,
    HTTP_HEADERS_IF_MODIFIED_SINCE,
    HTTP_HEADERS_X_REQUESTED_WITH,
    HTTP_HEADERS_X_FORWARDED_HOST,
    HTTP_HEADERS_X_FORWARDED_PROTO,
    HTTP_HEADERS_X_CSRF_TOKEN,
    HTTP_HEADERS_SAVE_DATA,
    HTTP_HEADERS_RANGE,
    HTTP_HEADERS_CONTENT_LENGTH,
    HTTP_HEADERS_CONTENT_TYPE,
    HTTP_HEADERS_VARY,
    HTTP_HEADERS_DATE,
    HTTP_HEADERS_SERVER,
    HTTP_HEADERS_EXPIRES,
    HTTP_HEADERS_CONTENT_ENCODING,
    HTTP_HEADERS_LAST_MODIFIED,
    HTTP_HEADERS_ETAG,
    HTTP_HEADERS_ALLOW,
    HTTP_HEADERS_CONTENT_RANGE,
    HTTP_HEADERS_ACCEPT_CHARSET,
    HTTP_HEADERS_ACCESS_CONTROL_ALLOW_CREDENTIALS,
    HTTP_HEADERS_ACCESS_CONTROL_ALLOW_HEADERS,
    HTTP_HEADERS_ACCESS_CONTROL_ALLOW_METHODS,
    HTTP_HEADERS_ACCESS_CONTROL_ALLOW_ORIGIN,
    HTTP_HEADERS_ACCESS_CONTROL_MAXAGE,
    HTTP_HEADERS_ACCESS_CONTROL_METHOD,
    HTTP_HEADERS_ACCESS_CONTROL_REQUEST_HEADERS,
    HTTP_HEADERS_ACCESS_CONTROL_REQUEST_METHOD,
    HTTP_HEADERS_ACCESS_CONTROL_REQUEST_METHODS,
    HTTP_HEADERS_AGE,
    HTTP_HEADERS_AUTHORIZATION,
    HTTP_HEADERS_CONTENT_BASE,
    HTTP_HEADERS_CONTENT_DESCRIPTION,
    HTTP_HEADERS_CONTENT_DISPOSITION,
    HTTP_HEADERS_CONTENT_LANGUAGE,
    HTTP_HEADERS_CONTENT_LOCATION,
    HTTP_HEADERS_CONTENT_MD5,
    HTTP_HEADERS_EXPECT,
    HTTP_HEADERS_IF_MATCH,
    HTTP_HEADERS_IF_NONE_MATCH,
    HTTP_HEADERS_IF_RANGE,
    HTTP_HEADERS_IF_UNMODIFIED_SINCE,
    HTTP_HEADERS_KEEP_ALIVE,
    HTTP_HEADERS_LINK,
    HTTP_HEADERS_LOCATION,
    HTTP_HEADERS_MAX_FORWARDS,
    HTTP_HEADERS_PROXY_AUTHENTICATE,
    HTTP_HEADERS_PROXY_AUTHORIZATION,
    HTTP_HEADERS_PROXY_CONNECTION,
    HTTP_HEADERS_PUBLIC,
    HTTP_HEADERS_RETRY_AFTER,
    HTTP_HEADERS_TE,
    HTTP_HEADERS_TRAILER,
    HTTP_HEADERS_TRANSFER_ENCODING,
    HTTP_HEADERS_UPGRADE,
    HTTP_HEADERS_WARNING,
    HTTP_HEADERS_WWW_AUTHENTICATE,
    HTTP_HEADERS_VIA,
    HTTP_HEADERS_STRICT_TRANSPORT_SECURITY,
    HTTP_HEADERS_X_FRAME_OPTIONS,
    HTTP_HEADERS_X_CONTENT_TYPE_OPTIONS,
    HTTP_HEADERS_ALT_SVC,
    HTTP_HEADERS_REFERRER_POLICY,
    HTTP_HEADERS_X_XSS_PROTECTION,
    HTTP_HEADERS_ACCEPT_RANGES,
    HTTP_HEADERS_SET_COOKIE,
    HTTP_HEADERS_SEC_CH_UA,
    HTTP_HEADERS_SEC_CH_UA_MOBILE,
    HTTP_HEADERS_SEC_CH_UA_PLATFORM,
    HTTP_HEADERS_SEC_FETCH_SITE,
    HTTP_HEADERS_SEC_FETCH_MODE,
    HTTP_HEADERS_SEC_FETCH_USER,
    HTTP_HEADERS_SEC_FETCH_DEST,
    HTTP_HEADERS_CF_RAY,
    HTTP_HEADERS_CF_VISITOR,
    HTTP_HEADERS_CF_CONNECTING_IP,
    HTTP_HEADERS_CF_IPCOUNTRY,
    HTTP_HEADERS_CDN_LOOP,
    HTTP_HEADERS_MAX,
};

// State used for parsing HTTP messages
enum http_message_parser_state {
    STATE_START,
    STATE_METHOD,
    STATE_URI,
    STATE_VERSION,
    STATE_STATUS,
    STATE_MESSAGE,
    STATE_NAME,
    STATE_COLON,
    STATE_VALUE,
    STATE_CR,
    STATE_LF1,
    STATE_LF2,
};

struct http_slice {
    uint16_t start;
    uint16_t end;
};

struct http_header {
    struct http_slice name;
    struct http_slice value;
};

struct http_xheaders {
    struct http_header *headers;
    uint32_t count;
    uint32_t capacity;
};

struct http_message_parser {
    // The current state of the parser.
    enum http_message_parser_state state;

    // The index of input as each character is iterated. This is should be initialized to zero and then carry over on
    // each call to http_msg_parse.
    uint32_t i;

    // The current character.
    uint8_t ch;

    // This is a cursor used to mark different sections of the input at different times. The cursor should be
    // initialized to zero on every call to http_msg_parse.
    uint32_t cursor;

    // This is to track characters being inserted into the method buffer. This should never exceed
    // HTTP_METHOD_MAX_STRLEN.
    uint32_t method_i;

    // Store the arguments that were passed to http_msg_parse.
    struct {
        const char *input;
        size_t input_size;
        size_t input_capacity;
    } args;

    // Storage for temporary values.
    struct {
        struct http_slice header;
        uint32_t i;
    } tmp ;
};

struct http_message {
    struct http_message_parser parser;
    enum http_message_types type;
    char method[HTTP_METHOD_MAX_STRLEN + 1];
    enum http_versions version;
    struct http_slice uri;
    uint32_t status;
    struct http_slice message;
    struct http_slice headers[HTTP_HEADERS_MAX];
    struct http_xheaders xheaders;
};

enum http_headers http_header_lookup(const char *str);
void http_msg_init(struct http_message *msg, enum http_message_types type);
void http_msg_free(struct http_message *msg);
int http_msg_parse(struct http_message *msg, const char *input, size_t max_size, size_t capacity);
bool http_header_is_repeatable(enum http_headers header);
bool http_is_token(uint8_t token);
char *http_header_get_xheader_value(struct http_message *msg, const char *xheader);
