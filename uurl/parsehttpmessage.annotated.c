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
//#include "libc/assert.h"
//#include "libc/limits.h"
//#include "libc/macros.internal.h"
//#include "libc/mem/alg.h"
//#include "libc/mem/arraylist.internal.h"
//#include "libc/mem/mem.h"
//#include "libc/serialize.h"
//#include "libc/stdio/stdio.h"
//#include "libc/str/str.h"
//#include "libc/str/tab.internal.h"
//#include "libc/sysv/errfuns.h"
//#include "libc/x/x.h"
//#include "net/http/http.h"

#include <strings.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include "http.h"
#include "debug.h"
#include "serialize.h"
#include "gethttpheader.h"

#define LIMIT (SHRT_MAX - 2)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/**
 * Initializes HTTP message parser.
 */
void http_msg_init(struct HttpMessage *r, int type) {
  unassert(type == kHttpRequest || type == kHttpResponse);
  bzero(r, sizeof(*r));
  r->type = type;
}

/**
 * Destroys HTTP message parser.
 */
void http_msg_free(struct HttpMessage *r) {
  if (r->xheaders.headers) {
    free(r->xheaders.headers);
    r->xheaders.headers = NULL;
    r->xheaders.count = 0;
  }
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
int ParseHttpMessage(struct HttpMessage *msg, const char *input, size_t max_size) {
  int curr_char, h, i;
  for (max_size = MIN(max_size, LIMIT); msg->i < max_size; ++msg->i) {
    // Get the first character, cast as an int for faster operations?
    curr_char = input[msg->i] & 0xff;

    // Switch on the current state of the message
    switch (msg->state) {
      // Start state
      //
      // The first state:
      //  * Consumes any leading CR or LF
      //  * Asserts that the current char is a valid token
      //  * Sets the state depending on the message type.
      case kHttpStateStart:
        if (curr_char == '\r' || curr_char == '\n')
          break;  // RFC7230 § 3.5

        if (!g_http_token[curr_char]) {
          debug_print("Invalid token: '%c' [0x%hhx]\n", curr_char, curr_char);
          return -1;
        }

        if (msg->type == kHttpRequest) {
          msg->state = kHttpStateMethod;
          msg->method = toupper(curr_char);
          msg->cursor = 8;
        } else {
          // kHttpResponse
          msg->state = kHttpStateVersion;
          msg->cursor = msg->i; // Set msg->a to point to the index of the 'H' in HTTP/1.X
        }
        break;

      // Method state
      //
      // Consume up to 8 valid HTTP tokens, breaking on whitespace
      //
      // JD-NOTE: cursor is being reused here when shifting "characters" into the uint64_t. Just keep cursor as += 1 and
      //          multiply the shift by 8 instead...
      //
      // Get the upper case ASCII value of each method character then left shift 8-bits for each byte that has already
      // been OR'd, then OR the shifted result into the uint64_t method field.
      case kHttpStateMethod:
        for (;;) {
          if (curr_char == ' ') {
            msg->cursor = msg->i + 1;
            msg->state = kHttpStateUri;
            break;
          } else if (msg->cursor == 64 || !g_http_token[curr_char]) {
            debug_print("Invalid token: '%c' [0x%hhx]    (or in valid method???)\n", curr_char, curr_char);
            return -1;
          }
          curr_char = toupper(curr_char);
          msg->method |= (uint64_t)curr_char << msg->cursor;
          msg->cursor += 8;
          if (++msg->i == max_size)
            break;
          curr_char = input[msg->i] & 0xff;
        }
        break;

      // URI state
      //
      // Iterate through all valid tokens until whitespace is encountered, then:
      //  * Assert that the URI isn't empty
      //  * Point the start of the URI to the cursor and the end of the URI to the current character which will always be either
      //    SP, CR, or LF
      //  * Check if a version was provided and parse it if so, otherwise default to HTTP 0.9
      case kHttpStateUri:
        for (;;) {
          if (curr_char == ' ' || curr_char == '\r' || curr_char == '\n') {
            if (msg->i == msg->cursor)
              return -1;
            msg->uri.slice_start = msg->cursor;
            msg->uri.slice_end = msg->i;
            if (curr_char == ' ') {
              msg->cursor = msg->i + 1;
              msg->state = kHttpStateVersion;
            } else {
              msg->version = 9;
              msg->state = curr_char == '\r' ? kHttpStateCr : kHttpStateLf1;
            }
            break;
          } else if (curr_char < 0x20 || (0x7F <= curr_char && curr_char < 0xA0)) {
            debug_print("Invalid ISO-8859-1: '%c' [0x%hhx]\n", curr_char, curr_char);
            return -1;
          }
          if (++msg->i == max_size)
            break;
          curr_char = input[msg->i] & 0xff;
        }
        break;

      // Version state
      //
      // When we enter this state we keep iterating over the stream until we receive our first whitespace character. As
      // soon as we do we then assert all of the following:
      //  * We've read exactly 8 bytes
      //  * The version matches "HTTP/*.*" where the wildcards can be anything but everything else is an exact match
      //  * The 5th and 7th index characters of "HTTP/*.*", the wildcards, are digits
      //
      // If all of these assertions are passed then:
      //  * Convert the 5th index, the major version, from ASCII to decimal, and multiply it by 10
      //  * Convert the 6th index, the minor version, from ASCII to decimal
      //  * Add the two values together and store them in msg->version
      //    - HTTP/1.0 == msg->version == 10
      //    - HTTP/1.1 == msg->version == 11
      //
      // Once finished with asserting and parsing the version:
      //  * If this was a request and the current character is a CR then we go into the CR state otherwise we assume it
      //    must be a LF and go into the LF1 state.
      //  * If this was a response then we go into the status state
      case kHttpStateVersion:
        // Check if we've read whitespace
        if (curr_char == ' ' || curr_char == '\r' || curr_char == '\n') {
          if (msg->i - msg->cursor == 8 &&
              // Read a 64-bit big endian value of the ASCII "HTTP/X.X" where the X's are masked out
              // Then compare this against 0x485454502F002E00 which is ASCII "HTTP/\0.\0"
              (READ64BE(input + msg->cursor) & 0xFFFFFFFFFF00FF00) == 0x485454502F002E00 &&
              isdigit(input[msg->cursor + 5]) && isdigit(input[msg->cursor + 7])) {
            msg->version = (input[msg->cursor + 5] - '0') * 10 + (input[msg->cursor + 7] - '0');
            if (msg->type == kHttpRequest) {
              msg->state = curr_char == '\r' ? kHttpStateCr : kHttpStateLf1;
            } else {
              msg->state = kHttpStateStatus;
            }
          } else {
            return -1;
          }
        }
        break;

      // Status state
      //
      // This function will mutate the index and current character. This function does not validate the status beyond
      // that it's within the range of 100-999
      //
      // Begin by iterating over each character while checking for whitespace. If we get whitespace and haven't yet
      // parsed a valid status value then we exit with a critical failure.
      //
      // * If we get whitespace and our status has already been boundary checked:
      //  - SP: change to message state
      //  - CR: change to CR state
      //  - LF: change to LF state
      // * A response message is optional and not required.
      //
      // * Otherwise, if the current character is anything in the range of 0-9 ASCII:
      //    - Multiply msg->status by 10 to shift the decimal to the right to make space for our new digit
      //    - Convert the current character from ASCII to decimal
      //    - Add the current character as a decimal value to the msg->status
      //    - Check if we've parsed more than 3 digits and if so exit with a critical failure
      //
      // * Any other character results in an exit with a critical failure
      case kHttpStateStatus:
        for (;;) {
          if (curr_char == ' ' || curr_char == '\r' || curr_char == '\n') {
            if (msg->status < 100)
              return -1;

            if (curr_char == ' ') {
              // BUG: Could this possibly point past the buffer? We're blindly indexing deeper without checking.
              msg->cursor = msg->i + 1; // Point the cursor to the next character, whatever it may be doesn't really matter
              msg->state = kHttpStateMessage;
            } else {
              msg->state = curr_char == '\r' ? kHttpStateCr : kHttpStateLf1;
            }
            break;
          } else if ('0' <= curr_char && curr_char <= '9') {
            msg->status *= 10; // shift to the left
            msg->status += curr_char - '0'; // add the decimal value

            // We've looped enough times to parse 3 digits, anything past this is an error
            if (msg->status > 999) {
              return -1;
            }
          } else {
            return -1;
          }
          if (++msg->i == max_size)
            break;
          curr_char = input[msg->i] & 0xff;
        }
        break;

      // Message state
      //
      // Optional status message that follows the response status value
      //
      // This function will mutate the index and current character.
      //
      // If we get a CR or LF then we assume we've reached the end of the message.
      //  * Set the message to point at the current cursor.
      //    - The cursor begins at whatever was one character past the status. The response message must be encoded as
      //      ISO-8859-1 https://en.wikipedia.org/wiki/ISO/IEC_8859-1#Code_page_layout
      //  * Set the message end to point at the current character, this will always be either a CR or LF
      //
      // Then update the state:
      //  * CR: change to CR state
      //  * LF: change to LF1 state
      case kHttpStateMessage:
        for (;;) {
          if (curr_char == '\r' || curr_char == '\n') {
            msg->message.slice_start = msg->cursor;
            // This points at the character that trails the last character of the string, in this case it will always be
            // either \r or \n
            msg->message.slice_end = msg->i;
            // JD-NOTE: I think instead of this going to LF1 state it should be failing with a critical error. Same goes
            //          for everywhere else this is being done. I can't find anything in the RFC that says it's okay to
            //          have just a LF and no CR.
            msg->state = curr_char == '\r' ? kHttpStateCr : kHttpStateLf1;
            break;
          } else if (curr_char < 0x20 || (0x7F <= curr_char && curr_char < 0xA0)) {
            debug_print("Invalid ISO-8859-1: '%c' [0x%hhx]\n", curr_char, curr_char);
            return -1;
          }
          if (++msg->i == max_size)
            break;
          curr_char = input[msg->i] & 0xff;
        }
        break;

      // CR state
      //
      // We only enter this state when we received a CR. This asserts that the CR was immediately followed by a LF.
      case kHttpStateCr:
        if (curr_char != '\n') {
          return -1;
        }
        msg->state = kHttpStateLf1;
        break;

      // LF1 state
      //
      // We only enter this state after we received a CR and a LF.
      // If the current character is a CR then we go to LF2 state.
      //
      // If the current character is:
      //  * CR: change to LF2 state
      //  * LF: return the number of bytes parsed, I don't know why...
      //  * Invalid token: exit with a critical failure
      //  * Valid token: point the start of the key to the current index and change to the name state
      //
      // "Although the line terminator for the start-line and header fields is the sequence CRLF, a recipient MAY
      // recognize a single LF as a line terminator and ignore any preceding CR." (RFC7230 §3.5)
      // JD-NOTE: Maybe add a lenient mode to toggle support for this?
      case kHttpStateLf1:
        if (curr_char == '\r') {
          msg->state = kHttpStateLf2;
          break;
        } else if (curr_char == '\n') {
          return ++msg->i;
        } else if (!g_http_token[curr_char]) {
          // 1. Forbid empty header name (RFC2616 §2.2)
          // 2. Forbid line folding (RFC7230 §3.2.4)
          return -1;
        }
        msg->key.slice_start = msg->i;
        msg->state = kHttpStateName;
        break;

      // Name state
      //
      // Name is the key for any key value pair
      //    [Name][Colon][Value]
      //
      // Keep iterating until you receive:
      //  * Colon: mark the end of the name at current index and change to the colon state
      //  * Invalid token: exit with a critical failure
      //  * Valid token: continue
      case kHttpStateName:
        for (;;) {
          if (curr_char == ':') {
            msg->key.slice_end = msg->i;
            msg->state = kHttpStateColon;
            break;
          } else if (!g_http_token[curr_char]) {
            return -1;
          }
          if (++msg->i == max_size)
            break;
          curr_char = input[msg->i] & 0xff;
        }
        break;

      // Colon state
      //
      // Colon is the delimeter for any key value pair
      //  * [Name][Colon][Value]
      //
      // Iterate until you get anything that isn't a SP or HTAB. Once you do then set the cursor to the current index
      // and fallthrough to the value state
      case kHttpStateColon:
        if (curr_char == ' ' || curr_char == '\t')
          break;
        msg->cursor = msg->i;
        msg->state = kHttpStateValue;
        // fallthrough

      // Value state
      //
      // Value is the value for any key value pair
      //  * [Name][Colon][Value]
      //
      // This function will mutate the index and current character.
      //
      // Iterate all valid tokens until a CR or LF is found, then:
      //  * Right-trim any SP or HTAB
      //  * Attempt to lookup the key in a hashmap and if found it returns the index of that key in the headers field.
      //    If the lookup results in an index and there header isn't already populated or the header isn't repeatable
      //    then set the beginning and end and you're done.
      //  * Otherwise it checks if the xheaders.headers buf needs to grow by 2x
      //  * Then it checks if there is space in xheaders.headers to add the new header
      //
      case kHttpStateValue:
        for (;;) {
          if (curr_char == '\r' || curr_char == '\n') {
            i = msg->i;
            // Right trim
            while (i > msg->cursor && (input[i - 1] == ' ' || input[i - 1] == '\t'))
              --i;
            printf("Checking if known header\n");
            if ((h = GetHttpHeader(input + msg->key.slice_start, msg->key.slice_end - msg->key.slice_start)) != -1 &&
                (!msg->headers[h].slice_start || !g_http_repeatable_header[h])) {
                  printf("Found a header!\n");
              msg->headers[h].slice_start = msg->cursor;
              msg->headers[h].slice_end = i;
            } else {
              printf("Header not known, addin gas x-header\n");
              if (msg->xheaders.count == msg->xheaders.capacity) {
                unsigned tmp_capacity;
                struct HttpHeader *tmp_headers1, *tmp_headers_2;
                tmp_headers1 = msg->xheaders.headers;
                tmp_capacity = msg->xheaders.capacity;
                if (tmp_capacity == 0) {
                  tmp_capacity = 1;
                } else {
                  tmp_capacity = tmp_capacity * 2;
                }
                if ((tmp_headers_2 = realloc(tmp_headers1, tmp_capacity * sizeof(*tmp_headers1)))) {
                  msg->xheaders.headers = tmp_headers_2;
                  msg->xheaders.capacity = tmp_capacity;
                }
              }
              if (msg->xheaders.count < msg->xheaders.capacity) {
                msg->xheaders.headers[msg->xheaders.count].key = msg->key;
                msg->xheaders.headers[msg->xheaders.count].value.slice_start = msg->cursor;
                msg->xheaders.headers[msg->xheaders.count].value.slice_end = i;
                msg->xheaders.headers = msg->xheaders.headers; // JD-NOTE: wtf is this?
                ++msg->xheaders.count;
              }
            }
            msg->state = curr_char == '\r' ? kHttpStateCr : kHttpStateLf1;
            break;
          }
          if ((curr_char < 0x20 && curr_char != '\t') || (0x7F <= curr_char && curr_char < 0xA0)) {
            debug_print("Invalid ISO-8859-1: '%c' [0x%hhx]\n", curr_char, curr_char);
            return -1;
          }
          if (++msg->i == max_size)
            break;
          curr_char = input[msg->i] & 0xff;
        }
        break;

      // LF2 state
      //
      // We only enter this state after we received a CRLF and another CR.
      // If the current character is a LF then return the number of bytes parsed, anything else is a critical error.
      case kHttpStateLf2:
        if (curr_char == '\n') {
          return ++msg->i;
        }
        return -1;

      default:
        __builtin_unreachable();
    }
  }
  if (msg->i < LIMIT) {
    return 0;
  } else {
    return -1;
  }
}
