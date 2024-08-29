/*═════════════════════════════════════════════════════════════════════════════╕
│ Copyright 2020 Justine Alexandra Roberts Tunney                              │
│                                                                              │
│ Permission to use, copy, modify, and/or distribute this software for         │
│ any purpose with or without fee is hereby granted, provided that the         │
│ above copyright notice and this permission notice appear in all copies.      │
│                                                                              │
│ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
│ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
│ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
│ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
│ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
│ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
│ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
│ PERFORMANCE OF THIS SOFTWARE.                                                │
╚─────────────────────────────────────────────────────────────────────────────*/

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "debug.h"
#include "http.h"
#include "gcc_attributes.h"

// RFC7230 § 2.6
PACKED struct http_version {
    char name[4]; // "HTTP"
    char slash;
    char major;
    char dot;
    char minor;
};
#define HTTP_VERSION_LEN sizeof(struct http_version)

#define TO_DECIMAL(ch) ((ch) - '0')

#define HTTP_STATUS_MIN 100
#define HTTP_STATUS_MAX 999

#define CHAR_IS_CRLF(c)          ((c) == '\r' || (c) == '\n')
#define CHAR_IS_CRLF_OR_SPACE(c) ((c) == '\r' || (c) == '\n' || (c) == ' ')
#define CHAR_IS_HTAB_OR_SPACE(c) ((c) == '\t' || (c) == ' ')
#define CHAR_IS_ISO_8859_1(c)    (((c) >= 0x20 && (c) <= 0x7E) || ((c) >= 0xA0 && (c) <= 0xFF))

#define DEBUG_PRINT_INVALID_TOKEN(ch)      debug_print("Invalid token: '%c' [0x%hhx]\n", ch, ch)
#define DEBUG_PRINT_INVALID_ISO_8859_1(ch) debug_print("Invalid ISO-8859-1: '%c' [0x%hhx]\n", ch, ch);

// Append the current character to the response status
static bool status_append(struct http_message *msg)
{
    if (!isdigit(msg->parser.ch)) {
        debug_print("bad status: '%c' [0x%hhx] is not a digit\n", msg->parser.ch, msg->parser.ch);
        return false;
    }

    msg->status *= 10;
    msg->status += TO_DECIMAL(msg->parser.ch);

    if (msg->status > HTTP_STATUS_MAX) {
        debug_print("bad status: %u > %u\n", msg->status, HTTP_STATUS_MAX);
        return false;
    }

    return true;
}

// Don't call this directly, call xheaders_insert instead and let it grow when needed.
static bool xheaders_grow(struct http_xheaders *xheaders)
{
    if (!xheaders) {
        debug_print("bad args: xheaders is null\n");
        return false;
    }

    // Double the capacity
    uint32_t capacity_new = xheaders->capacity == 0 ? 1 : xheaders->capacity * 2;

    size_t size_new = capacity_new * sizeof(*xheaders->headers);
    struct http_header *headers_temp = realloc(xheaders->headers, size_new);
    if (!headers_temp) {
        debug_print("realloc: [%d] %m\n", errno);
        return false;
    }

    xheaders->headers = headers_temp;
    xheaders->capacity = capacity_new;
    return true;
}

// Insert an x-header, growing the buffer if necessary
static bool xheaders_insert(struct http_message *msg)
{
    if (msg->xheaders.count == msg->xheaders.capacity) {
        if (!xheaders_grow(&msg->xheaders))
            return false;
    }
    if (msg->xheaders.count < msg->xheaders.capacity) {
        msg->xheaders.headers[msg->xheaders.count].name = msg->parser.tmp.header;
        msg->xheaders.headers[msg->xheaders.count].value.start = msg->parser.cursor;
        msg->xheaders.headers[msg->xheaders.count].value.end = msg->parser.tmp.i;
        ++msg->xheaders.count;
        return true;
    }
    return false;
}

// Check if a header entry is already populated
static bool header_exists(struct http_message *msg, enum http_headers header)
{
    if (header == HTTP_HEADERS_UNKNOWN)
        return false;
    return msg->headers[header].start != 0;
}

// Everything in the HTTP version scheme is case specific and exact.
static enum http_versions parse_version(struct http_message *msg)
{
    size_t version_len = msg->parser.i - msg->parser.cursor;
    if (version_len != HTTP_VERSION_LEN)
        return HTTP_VERSION_UNKNOWN;

    const char *input_src = msg->parser.args.input + msg->parser.cursor;
    struct http_version version = { 0 };
    memcpy(&version, input_src, sizeof(version));

    if (strncmp(version.name, "HTTP", 4) != 0)
        return HTTP_VERSION_UNKNOWN;
    if (version.slash != '/' || version.dot != '.')
        return HTTP_VERSION_UNKNOWN;
    if (!isdigit(version.major) || !isdigit(version.minor))
        return HTTP_VERSION_UNKNOWN;

    if (version.major == '0' && version.minor == '9')
        return HTTP_VERSION_0_9;
    if (version.major == '1' && version.minor == '0')
        return HTTP_VERSION_1_0;
    if (version.major == '1' && version.minor == '1')
        return HTTP_VERSION_1_1;

    return HTTP_VERSION_UNKNOWN;
}

static char get_current_char(struct http_message *msg)
{
    return msg->parser.args.input[msg->parser.i];
}

// Insert the current character into the method buffer
static bool insert_method_char(struct http_message *msg)
{
    if (msg->parser.method_i == HTTP_METHOD_MAX_STRLEN) {
        debug_print("[ERR] method is too long\n");
        return false;
    }
    if (!http_is_token(msg->parser.ch)) {
        DEBUG_PRINT_INVALID_TOKEN(msg->parser.ch);
        return false;
    }

    msg->method[msg->parser.method_i++] = toupper(get_current_char(msg));
    return true;
}

// Set the parser's temporary index to point to the current input character then move it to the left if there's any
// space or HTAB. The cursor should be set to point to the beginning of the value to trim before calling this.
static void set_tmp_i_to_input_rtrim(struct http_message *msg)
{
    msg->parser.tmp.i = msg->parser.i;
    while (msg->parser.tmp.i > msg->parser.cursor && (CHAR_IS_HTAB_OR_SPACE(msg->parser.args.input[msg->parser.tmp.i - 1])))
        --msg->parser.tmp.i;
}

/**
 * Parses HTTP request or response.
 *
 * This parser is responsible for determining the length of a message
 * and slicing the strings inside it. Performance is attained using
 * perfect hash tables. No memory allocation is performed for normal
 * messagesy. Line folding is forbidden. State persists across calls so
 * that fragmented messages can be handled efficiently. A limitation on
 * message size is imposed to make the header data structures smaller.
 *
 * This parser assumes ISO-8859-1 and guarantees no C0 or C1 control
 * codes are present in message fields, with the exception of tab.
 * Please note that fields like kHttpStateUri may use UTF-8 percent encoding.
 * This parser doesn't care if you choose ASA X3.4-1963 or MULTICS newlines.
 *
 * ISO-8859-1 https://en.wikipedia.org/wiki/ISO/IEC_8859-1#Code_page_layout
 *
 * kHttpRepeatable defines which standard header fields are O(1) and
 * which ones may have comma entries spilled over into xheaders. For
 * most headers it's sufficient to simply check the static slice. If
 * r->headers[kHttpFoo].a is zero then the header is totally absent.
 *
 * This parser has linear complexity. Each character only needs to be
 * considered a single time. That's the case even if messages are
 * fragmented. If a message is valid but incomplete, this function will
 * return zero so that it can be resumed as soon as more data arrives.
 *
 * This parser takes about 400 nanoseconds to parse a 403 byte Chrome
 * HTTP request under MODE=rel on a Core i9 which is about three cycles
 * per byte or a gigabyte per second of throughput per core.
 *
 * @note we assume p points to a buffer that has >=SHRT_MAX bytes
 * @see HTTP/1.1 RFC2616 RFC2068
 * @see HTTP/1.0 RFC1945
 */
int http_msg_parse(struct http_message *msg, const char * const input, const size_t input_size, const size_t input_capacity) {
    if (input_size > input_capacity) {
        debug_print("ERR: The max size is larger than the capacity: %zu > %zu\n", input_size, input_capacity);
        return -1;
    }

    if (msg->type != HTTP_MESSAGE_TYPE_REQUEST && msg->type != HTTP_MESSAGE_TYPE_RESPONSE) {
        debug_print("Unrecognized HTTP message type: %u\n", msg->type);
        return -1;
    }

    msg->parser.args.input = input;
    msg->parser.args.input_size = input_size > SHRT_MAX ? SHRT_MAX : input_size;
    msg->parser.args.input_capacity = input_capacity > SHRT_MAX ? SHRT_MAX : input_capacity;

    for (; msg->parser.i < msg->parser.args.input_size; ++msg->parser.i) {
        msg->parser.ch = get_current_char(msg);

        switch (msg->parser.state) {
        case STATE_START:
            if (CHAR_IS_CRLF(msg->parser.ch))
                break;  // RFC7230 § 3.5
            msg->parser.cursor = msg->parser.i;

            if (msg->type == HTTP_MESSAGE_TYPE_REQUEST) {
                msg->parser.state = STATE_METHOD;
                if (!insert_method_char(msg))
                    return -1;
            } else /* msg->type == HTTP_MESSAGE_TYPE_RESPONSE */ {
                msg->parser.state = STATE_VERSION;
            }
            break;

        case STATE_METHOD:
            if (msg->parser.ch == ' ') {
                // This cursor placed here acts like an anchor to point to the start of the URI
                msg->parser.cursor = msg->parser.i + 1;
                msg->parser.state = STATE_URI;
            } else if (!insert_method_char(msg)) {
                debug_print("insert_method_char\n");
                return -1;
            }
            break;

        case STATE_URI:
            if (CHAR_IS_CRLF_OR_SPACE(msg->parser.ch)) {
                if (msg->parser.i == msg->parser.cursor) {
                    debug_print("[ERR] empty uri\n");
                    return -1;
                }
                msg->uri.start = msg->parser.cursor;
                msg->uri.end = msg->parser.i;
                if (msg->parser.ch == ' ') {
                    msg->parser.cursor = msg->parser.i + 1;
                    msg->parser.state = STATE_VERSION;
                } else {
                    // HTTP/0.9 lacks a version
                    msg->version = HTTP_VERSION_0_9;
                    msg->parser.state = msg->parser.ch == '\r' ? STATE_CR : STATE_LF1;
                }
            } else if (!CHAR_IS_ISO_8859_1(msg->parser.ch)) {
                DEBUG_PRINT_INVALID_ISO_8859_1(msg->parser.ch);
                return -1;
            }
            break;

        case STATE_VERSION:
            if (CHAR_IS_CRLF_OR_SPACE(msg->parser.ch)) {
                enum http_versions version = parse_version(msg);
                if (version == HTTP_VERSION_UNKNOWN) {
                    debug_print("[ERR] unable to parse version\n");
                    return -1;
                }
                msg->version = version;

                if (msg->type == HTTP_MESSAGE_TYPE_REQUEST) {
                    msg->parser.state = msg->parser.ch == '\r' ? STATE_CR : STATE_LF1;
                } else {
                    msg->parser.state = STATE_STATUS;
                }
            }
            break;

        case STATE_STATUS:
            // Keep getting the next character and try appending it to the status until you're finished
            if (CHAR_IS_CRLF_OR_SPACE(msg->parser.ch)) {
                // We've finished parsing the status, now check that it's below the minimum for a valid status code.
                if (msg->status < HTTP_STATUS_MIN) {
                    debug_print("bad status: %u < %u\n", msg->status, HTTP_STATUS_MIN);
                    return -1;
                }

                // Any trailing whitespace means there could be an optional status message, otherwise parse the CRLF
                if (msg->parser.ch == ' ') {
                    // BUG: Could this possibly point past the buffer? We're blindly indexing deeper without checking.
                    msg->parser.cursor = msg->parser.i + 1;
                    msg->parser.state = STATE_MESSAGE;
                } else {
                    msg->parser.state = msg->parser.ch == '\r' ? STATE_CR : STATE_LF1;
                }
            } else if (!status_append(msg)) {
                debug_print("status_append\n");
                return -1;
            }
            break;

        case STATE_MESSAGE:
            if (CHAR_IS_CRLF(msg->parser.ch)) {
                msg->message.start = msg->parser.cursor;
                msg->message.end = msg->parser.i;
                msg->parser.state = msg->parser.ch == '\r' ? STATE_CR : STATE_LF1;
            } else if (!CHAR_IS_ISO_8859_1(msg->parser.ch)) {
                DEBUG_PRINT_INVALID_ISO_8859_1(msg->parser.ch);
                return -1;
            }
            break;

        case STATE_CR:
            if (msg->parser.ch != '\n') {
                debug_print("expected LF, got '%c' [0x%hhx]\n", msg->parser.ch, msg->parser.ch);
                return -1;
            }
            msg->parser.state = STATE_LF1;
            break;

        // "Although the line terminator for the start-line and header fields is the sequence CRLF, a recipient MAY
        // recognize a single LF as a line terminator and ignore any preceding CR."
        // RFC7230 §3.5
        //
        // JD-NOTE: Maybe add a lenient mode to toggle support for this?
        case STATE_LF1:
            if (msg->parser.ch == '\r') {
                msg->parser.state = STATE_LF2;
                break;
            } else if (msg->parser.ch == '\n') {
                return ++msg->parser.i;
            } else if (!http_is_token(msg->parser.ch)) {
                // 1. Forbid empty header name (RFC2616 §2.2)
                // 2. Forbid line folding (RFC7230 §3.2.4)
                DEBUG_PRINT_INVALID_TOKEN(msg->parser.ch);
                return -1;
            }
            msg->parser.tmp.header.start = msg->parser.i;
            msg->parser.state = STATE_NAME;
            break;

        case STATE_NAME:
            if (msg->parser.ch == ':') {
                msg->parser.tmp.header.end = msg->parser.i;
                msg->parser.state = STATE_COLON;
            } else if (!http_is_token(msg->parser.ch)) {
                DEBUG_PRINT_INVALID_TOKEN(msg->parser.ch);
                return -1;
            }
            break;

        case STATE_COLON:
            if (CHAR_IS_HTAB_OR_SPACE(msg->parser.ch))
                break;

            msg->parser.cursor = msg->parser.i;
            msg->parser.state = STATE_VALUE;
            // fallthrough
        case STATE_VALUE:
            if (CHAR_IS_CRLF(msg->parser.ch)) {
                set_tmp_i_to_input_rtrim(msg);

                enum http_headers header = http_header_lookup(msg->parser.args.input + msg->parser.tmp.header.start);
                if (header == HTTP_HEADERS_UNKNOWN || (header_exists(msg, header) && http_header_is_repeatable(header))) {
                    if (!xheaders_insert(msg))
                        return -1;
                } else {
                    msg->headers[header].start = msg->parser.cursor;
                    msg->headers[header].end = msg->parser.tmp.i;
                }

                msg->parser.state = msg->parser.ch == '\r' ? STATE_CR : STATE_LF1;
            } else if (msg->parser.ch != '\t' && !CHAR_IS_ISO_8859_1(msg->parser.ch)) {
                DEBUG_PRINT_INVALID_ISO_8859_1(msg->parser.ch);
                return -1;
            }
            break;

        case STATE_LF2:
            if (msg->parser.ch == '\n')
                return ++msg->parser.i;

            debug_print("expected LF, got '%c' [0x%hhx]\n", msg->parser.ch, msg->parser.ch);
            return -1;

        default:
            __builtin_unreachable();
        }
    }

    if (msg->parser.i < msg->parser.args.input_capacity) {
        return 0;
    } else {
        return -1;
    }
}

// Initializes HTTP message parser.
void http_msg_init(struct http_message *msg, enum http_message_types type)
{
    assert(type == HTTP_MESSAGE_TYPE_REQUEST || type == HTTP_MESSAGE_TYPE_RESPONSE);
    memset(msg, '\0', sizeof(*msg));
    msg->type = type;
}

// Destroys HTTP message parser.
void http_msg_free(struct http_message *msg)
{
    if (!msg)
        return;

    if (!msg->xheaders.headers)
        return;

    free(msg->xheaders.headers);
    msg->xheaders.headers = NULL;
    msg->xheaders.count = 0;
}
