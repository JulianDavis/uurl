/*═════════════════════════════════════════════════════════════════════════════╕
│ Copyright 2021 Justine Alexandra Roberts Tunney                              │
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
#define _GNU_SOURCE
#include <stdbool.h>
#include <string.h>

#include "http.h"

/**
 * Set of standard comma-separate HTTP headers that may span lines.
 *
 * These headers may specified on multiple lines, e.g.
 *
 *     Allow: GET
 *     Allow: POST
 *
 * Is the same as:
 *
 *     Allow: GET, POST
 *
 * Standard headers that aren't part of this set will be overwritten in
 * the event that they're specified multiple times. For example,
 *
 *     Content-Type: application/octet-stream
 *     Content-Type: text/plain; charset=utf-8
 *
 * Is the same as:
 *
 *     Content-Type: text/plain; charset=utf-8
 *
 * This set exists to optimize header lookups and parsing. The existence
 * of standard headers that aren't in this set is an O(1) operation. The
 * repeatable headers in this list require an O(1) operation if they are
 * not present, otherwise the extended headers list needs to be crawled.
 *
 * Please note non-standard headers exist, e.g. Cookie, that may span
 * multiple lines, even though they're not comma-delimited. For those
 * headers we simply don't add them to the perfect hash table.
 *
 * @note we choose to not recognize this grammar for kHttpConnection
 * @note `grep '[A-Z][a-z]*".*":"' rfc2616`
 * @note `grep ':.*#' rfc2616`
 * @see RFC7230 § 4.2
 */
bool http_header_is_repeatable(enum http_headers header)
{
    static const bool repeatable_header[HTTP_HEADERS_MAX] = {
        [HTTP_HEADERS_ACCEPT_CHARSET] = true,
        [HTTP_HEADERS_ACCEPT_ENCODING] = true,
        [HTTP_HEADERS_ACCEPT_LANGUAGE] = true,
        [HTTP_HEADERS_ACCEPT] = true,
        [HTTP_HEADERS_ALLOW] = true,
        [HTTP_HEADERS_CACHE_CONTROL] = true,
        [HTTP_HEADERS_CONTENT_ENCODING] = true,
        [HTTP_HEADERS_CONTENT_LANGUAGE] = true,
        [HTTP_HEADERS_EXPECT] = true,
        [HTTP_HEADERS_IF_MATCH] = true,
        [HTTP_HEADERS_IF_NONE_MATCH] = true,
        [HTTP_HEADERS_PRAGMA] = true,
        [HTTP_HEADERS_PROXY_AUTHENTICATE] = true,
        [HTTP_HEADERS_PUBLIC] = true,
        [HTTP_HEADERS_TE] = true,
        [HTTP_HEADERS_TRAILER] = true,
        [HTTP_HEADERS_TRANSFER_ENCODING] = true,
        [HTTP_HEADERS_UPGRADE] = true,
        [HTTP_HEADERS_VARY] = true,
        [HTTP_HEADERS_VIA] = true,
        [HTTP_HEADERS_WARNING] = true,
        [HTTP_HEADERS_WWW_AUTHENTICATE] = true,
        [HTTP_HEADERS_X_FORWARDED_FOR] = true,
        [HTTP_HEADERS_ACCESS_CONTROL_ALLOW_HEADERS] = true,
        [HTTP_HEADERS_ACCESS_CONTROL_ALLOW_METHODS] = true,
        [HTTP_HEADERS_ACCESS_CONTROL_REQUEST_HEADERS] = true,
        [HTTP_HEADERS_ACCESS_CONTROL_REQUEST_METHODS] = true,
    };
    return repeatable_header[header];
}

enum http_headers http_header_lookup(const char *str)
{
  if (strncasecmp("Host", str, 4) == 0)
    return HTTP_HEADERS_HOST;
  if (strncasecmp("Cache-Control", str, 13) == 0)
      return HTTP_HEADERS_CACHE_CONTROL;
  if (strncasecmp("Connection", str, 10) == 0)
      return HTTP_HEADERS_CONNECTION;
  if (strncasecmp("Accept", str, 6) == 0)
      return HTTP_HEADERS_ACCEPT;
  if (strncasecmp("Accept-Language", str, 15) == 0)
      return HTTP_HEADERS_ACCEPT_LANGUAGE;
  if (strncasecmp("Accept-Encoding", str, 15) == 0)
      return HTTP_HEADERS_ACCEPT_ENCODING;
  if (strncasecmp("User-Agent", str, 10) == 0)
      return HTTP_HEADERS_USER_AGENT;
  if (strncasecmp("Referer", str, 7) == 0)
      return HTTP_HEADERS_REFERER;
  if (strncasecmp("X-Forwarded-For", str, 15) == 0)
      return HTTP_HEADERS_X_FORWARDED_FOR;
  if (strncasecmp("Origin", str, 6) == 0)
      return HTTP_HEADERS_ORIGIN;
  if (strncasecmp("Upgrade-Insecure-Requests", str, 25) == 0)
      return HTTP_HEADERS_UPGRADE_INSECURE_REQUESTS;
  if (strncasecmp("Pragma", str, 6) == 0)
      return HTTP_HEADERS_PRAGMA;
  if (strncasecmp("Cookie", str, 6) == 0)
      return HTTP_HEADERS_COOKIE;
  if (strncasecmp("DNT", str, 3) == 0)
      return HTTP_HEADERS_DNT;
  if (strncasecmp("Sec-GPC", str, 7) == 0)
      return HTTP_HEADERS_SEC_GPC;
  if (strncasecmp("From", str, 4) == 0)
      return HTTP_HEADERS_FROM;
  if (strncasecmp("If-Modified-Since", str, 17) == 0)
      return HTTP_HEADERS_IF_MODIFIED_SINCE;
  if (strncasecmp("X-Requested-With", str, 16) == 0)
      return HTTP_HEADERS_X_REQUESTED_WITH;
  if (strncasecmp("X-Forwarded-Host", str, 16) == 0)
      return HTTP_HEADERS_X_FORWARDED_HOST;
  if (strncasecmp("X-Forwarded-Proto", str, 17) == 0)
      return HTTP_HEADERS_X_FORWARDED_PROTO;
  if (strncasecmp("X-CSRF-Token", str, 12) == 0)
      return HTTP_HEADERS_X_CSRF_TOKEN;
  if (strncasecmp("Save-Data", str, 9) == 0)
      return HTTP_HEADERS_SAVE_DATA;
  if (strncasecmp("Range", str, 5) == 0)
      return HTTP_HEADERS_RANGE;
  if (strncasecmp("Content-Length", str, 14) == 0)
      return HTTP_HEADERS_CONTENT_LENGTH;
  if (strncasecmp("Content-Type", str, 12) == 0)
      return HTTP_HEADERS_CONTENT_TYPE;
  if (strncasecmp("Vary", str, 4) == 0)
      return HTTP_HEADERS_VARY;
  if (strncasecmp("Date", str, 4) == 0)
      return HTTP_HEADERS_DATE;
  if (strncasecmp("Server", str, 6) == 0)
      return HTTP_HEADERS_SERVER;
  if (strncasecmp("Expires", str, 7) == 0)
      return HTTP_HEADERS_EXPIRES;
  if (strncasecmp("Content-Encoding", str, 16) == 0)
      return HTTP_HEADERS_CONTENT_ENCODING;
  if (strncasecmp("Last-Modified", str, 13) == 0)
      return HTTP_HEADERS_LAST_MODIFIED;
  if (strncasecmp("ETag", str, 4) == 0)
      return HTTP_HEADERS_ETAG;
  if (strncasecmp("Allow", str, 5) == 0)
      return HTTP_HEADERS_ALLOW;
  if (strncasecmp("Content-Range", str, 13) == 0)
      return HTTP_HEADERS_CONTENT_RANGE;
  if (strncasecmp("Accept-Charset", str, 14) == 0)
      return HTTP_HEADERS_ACCEPT_CHARSET;
  if (strncasecmp("Access-Control-Allow-Credentials", str, 32) == 0)
      return HTTP_HEADERS_ACCESS_CONTROL_ALLOW_CREDENTIALS;
  if (strncasecmp("Access-Control-Allow-Headers", str, 28) == 0)
      return HTTP_HEADERS_ACCESS_CONTROL_ALLOW_HEADERS;
  if (strncasecmp("Access-Control-Allow-Methods", str, 28) == 0)
      return HTTP_HEADERS_ACCESS_CONTROL_ALLOW_METHODS;
  if (strncasecmp("Access-Control-Allow-Origin", str, 27) == 0)
      return HTTP_HEADERS_ACCESS_CONTROL_ALLOW_ORIGIN;
  if (strncasecmp("Access-Control-MaxAge", str, 21) == 0)
      return HTTP_HEADERS_ACCESS_CONTROL_MAXAGE;
  if (strncasecmp("Access-Control-Method", str, 21) == 0)
      return HTTP_HEADERS_ACCESS_CONTROL_METHOD;
  if (strncasecmp("Access-Control-Request-Headers", str, 30) == 0)
      return HTTP_HEADERS_ACCESS_CONTROL_REQUEST_HEADERS;
  if (strncasecmp("Access-Control-Request-Method", str, 29) == 0)
      return HTTP_HEADERS_ACCESS_CONTROL_REQUEST_METHOD;
  if (strncasecmp("Access-Control-Request-Methods", str, 30) == 0)
      return HTTP_HEADERS_ACCESS_CONTROL_REQUEST_METHODS;
  if (strncasecmp("Age", str, 3) == 0)
      return HTTP_HEADERS_AGE;
  if (strncasecmp("Authorization", str, 13) == 0)
      return HTTP_HEADERS_AUTHORIZATION;
  if (strncasecmp("Content-Base", str, 12) == 0)
      return HTTP_HEADERS_CONTENT_BASE;
  if (strncasecmp("Content-Description", str, 19) == 0)
      return HTTP_HEADERS_CONTENT_DESCRIPTION;
  if (strncasecmp("Content-Disposition", str, 19) == 0)
      return HTTP_HEADERS_CONTENT_DISPOSITION;
  if (strncasecmp("Content-Language", str, 16) == 0)
      return HTTP_HEADERS_CONTENT_LANGUAGE;
  if (strncasecmp("Content-Location", str, 16) == 0)
      return HTTP_HEADERS_CONTENT_LOCATION;
  if (strncasecmp("Content-MD5", str, 11) == 0)
      return HTTP_HEADERS_CONTENT_MD5;
  if (strncasecmp("Expect", str, 6) == 0)
      return HTTP_HEADERS_EXPECT;
  if (strncasecmp("If-Match", str, 8) == 0)
      return HTTP_HEADERS_IF_MATCH;
  if (strncasecmp("If-None-Match", str, 13) == 0)
      return HTTP_HEADERS_IF_NONE_MATCH;
  if (strncasecmp("If-Range", str, 8) == 0)
      return HTTP_HEADERS_IF_RANGE;
  if (strncasecmp("If-Unmodified-Since", str, 19) == 0)
      return HTTP_HEADERS_IF_UNMODIFIED_SINCE;
  if (strncasecmp("Keep-Alive", str, 10) == 0)
      return HTTP_HEADERS_KEEP_ALIVE;
  if (strncasecmp("Link", str, 4) == 0)
      return HTTP_HEADERS_LINK;
  if (strncasecmp("Location", str, 8) == 0)
      return HTTP_HEADERS_LOCATION;
  if (strncasecmp("Max-Forwards", str, 12) == 0)
      return HTTP_HEADERS_MAX_FORWARDS;
  if (strncasecmp("Proxy-Authenticate", str, 18) == 0)
      return HTTP_HEADERS_PROXY_AUTHENTICATE;
  if (strncasecmp("Proxy-Authorization", str, 19) == 0)
      return HTTP_HEADERS_PROXY_AUTHORIZATION;
  if (strncasecmp("Proxy-Connection", str, 16) == 0)
      return HTTP_HEADERS_PROXY_CONNECTION;
  if (strncasecmp("Public", str, 6) == 0)
      return HTTP_HEADERS_PUBLIC;
  if (strncasecmp("Retry-After", str, 11) == 0)
      return HTTP_HEADERS_RETRY_AFTER;
  if (strncasecmp("TE", str, 2) == 0)
      return HTTP_HEADERS_TE;
  if (strncasecmp("Trailer", str, 7) == 0)
      return HTTP_HEADERS_TRAILER;
  if (strncasecmp("Transfer-Encoding", str, 17) == 0)
      return HTTP_HEADERS_TRANSFER_ENCODING;
  if (strncasecmp("Upgrade", str, 7) == 0)
      return HTTP_HEADERS_UPGRADE;
  if (strncasecmp("Warning", str, 7) == 0)
      return HTTP_HEADERS_WARNING;
  if (strncasecmp("WWW-Authenticate", str, 16) == 0)
      return HTTP_HEADERS_WWW_AUTHENTICATE;
  if (strncasecmp("Via", str, 3) == 0)
      return HTTP_HEADERS_VIA;
  if (strncasecmp("Strict-Transport-Security", str, 25) == 0)
      return HTTP_HEADERS_STRICT_TRANSPORT_SECURITY;
  if (strncasecmp("X-Frame-Options", str, 15) == 0)
      return HTTP_HEADERS_X_FRAME_OPTIONS;
  if (strncasecmp("X-Content-Type-Options", str, 22) == 0)
      return HTTP_HEADERS_X_CONTENT_TYPE_OPTIONS;
  if (strncasecmp("Alt-Svc", str, 7) == 0)
      return HTTP_HEADERS_ALT_SVC;
  if (strncasecmp("Referrer-Policy", str, 15) == 0)
      return HTTP_HEADERS_REFERRER_POLICY;
  if (strncasecmp("X-XSS-Protection", str, 16) == 0)
      return HTTP_HEADERS_X_XSS_PROTECTION;
  if (strncasecmp("Accept-Ranges", str, 13) == 0)
      return HTTP_HEADERS_ACCEPT_RANGES;
  if (strncasecmp("Set-Cookie", str, 10) == 0)
      return HTTP_HEADERS_SET_COOKIE;
  if (strncasecmp("Sec-CH-UA", str, 9) == 0)
      return HTTP_HEADERS_SEC_CH_UA;
  if (strncasecmp("Sec-CH-UA-Mobile", str, 16) == 0)
      return HTTP_HEADERS_SEC_CH_UA_MOBILE;
  if (strncasecmp("Sec-CH-UA-Platform", str, 18) == 0)
      return HTTP_HEADERS_SEC_CH_UA_PLATFORM;
  if (strncasecmp("Sec-Fetch-Site", str, 14) == 0)
      return HTTP_HEADERS_SEC_FETCH_SITE;
  if (strncasecmp("Sec-Fetch-Mode", str, 14) == 0)
      return HTTP_HEADERS_SEC_FETCH_MODE;
  if (strncasecmp("Sec-Fetch-User", str, 14) == 0)
      return HTTP_HEADERS_SEC_FETCH_USER;
  if (strncasecmp("Sec-Fetch-Dest", str, 14) == 0)
      return HTTP_HEADERS_SEC_FETCH_DEST;
  if (strncasecmp("CF-RAY", str, 6) == 0)
      return HTTP_HEADERS_CF_RAY;
  if (strncasecmp("CF-Visitor", str, 10) == 0)
      return HTTP_HEADERS_CF_VISITOR;
  if (strncasecmp("CF-Connecting-IP", str, 16) == 0)
      return HTTP_HEADERS_CF_CONNECTING_IP;
  if (strncasecmp("CF-IPCountry", str, 12) == 0)
      return HTTP_HEADERS_CF_IPCOUNTRY;
  if (strncasecmp("CDN-Loop", str, 8) == 0)
      return HTTP_HEADERS_CDN_LOOP;

  return -1;
}
