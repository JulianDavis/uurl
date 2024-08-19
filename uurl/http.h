
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define kHttpRequest  0
#define kHttpResponse 1

#define READ32LE(P)                        \
    (__extension__({                       \
        uint32_t __x;                      \
        __builtin_memcpy(&__x, P, 32 / 8); \
        __SWAPLE32(__x);                   \
    }))

#define READ64LE(P)                        \
    (__extension__({                       \
        uint64_t __x;                      \
        __builtin_memcpy(&__x, P, 64 / 8); \
        __SWAPLE32(__x);                   \
    }))

#define kHttpGet     READ32LE("GET")
#define kHttpHead    READ32LE("HEAD")
#define kHttpPost    READ32LE("POST")
#define kHttpPut     READ32LE("PUT")
#define kHttpDelete  READ64LE("DELETE\0")
#define kHttpOptions READ64LE("OPTIONS")
#define kHttpConnect READ64LE("CONNECT")
#define kHttpTrace   READ64LE("TRACE\0\0")

#define kHttpStateStart   0
#define kHttpStateMethod  1
#define kHttpStateUri     2
#define kHttpStateVersion 3
#define kHttpStateStatus  4
#define kHttpStateMessage 5
#define kHttpStateName    6
#define kHttpStateColon   7
#define kHttpStateValue   8
#define kHttpStateCr      9
#define kHttpStateLf1     10
#define kHttpStateLf2     11

#define kHttpClientStateHeaders      0
#define kHttpClientStateBody         1
#define kHttpClientStateBodyChunked  2
#define kHttpClientStateBodyLengthed 3

#define kHttpStateChunkStart   0
#define kHttpStateChunkSize    1
#define kHttpStateChunkExt     2
#define kHttpStateChunkLf1     3
#define kHttpStateChunk        4
#define kHttpStateChunkCr2     5
#define kHttpStateChunkLf2     6
#define kHttpStateTrailerStart 7
#define kHttpStateTrailer      8
#define kHttpStateTrailerLf1   9
#define kHttpStateTrailerLf2   10

enum http_headers {
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

extern const char g_http_token[256];
extern const bool g_http_repeatable_header[HTTP_HEADERS_MAX];

struct HttpSlice {
    short slice_start;
    short slice_end;
};

struct HttpHeader {
    struct HttpSlice key;
    struct HttpSlice value;
};

struct HttpHeaders {
    unsigned int count;
    unsigned int capacity;
    struct HttpHeader *headers;
};

struct HttpMessage {
    int i;
    int cursor;
    int status;
    unsigned char state;
    unsigned char type;
    unsigned char version;
    uint64_t method;
    struct HttpSlice key;
    struct HttpSlice uri;
    struct HttpSlice scratch;
    struct HttpSlice message;
    struct HttpSlice headers[HTTP_HEADERS_MAX];
    struct HttpHeaders xheaders;
};

struct HttpUnchunker {
    int t;
    size_t i;
    size_t j;
    ssize_t m;
};

enum http_headers http_header_lookup(const char *str);
void http_msg_init(struct HttpMessage *, int);
void http_msg_free(struct HttpMessage *);
int http_msg_parse(struct HttpMessage *msg, const char *input, size_t max_size, size_t capacity);
bool http_header_is_repeatable(enum http_headers header);
