/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│ vi: set et ft=c ts=2 sts=2 sw=2 fenc=utf-8                               :vi │
╞══════════════════════════════════════════════════════════════════════════════╡
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
#include "net/http/gethttpheader.inc"
#include "net/http/http.h"

// JD-NOTE: This file is completely altered. The original is a code-gen hash map that isn't committed. This is
//          functionally the same, albeit dramatically slower.
#define _GNU_SOURCE
#include <string.h>

#include "http.h"

static int LookupHttpHeader(const char *str)
{
  if (strncasecmp("Host", str, 4) == 0)
    return kHttpHost;
  if (strncasecmp("Cache-Control", str, 13) == 0)
      return kHttpCacheControl;
  if (strncasecmp("Connection", str, 10) == 0)
      return kHttpConnection;
  if (strncasecmp("Accept", str, 6) == 0)
      return kHttpAccept;
  if (strncasecmp("Accept-Language", str, 15) == 0)
      return kHttpAcceptLanguage;
  if (strncasecmp("Accept-Encoding", str, 15) == 0)
      return kHttpAcceptEncoding;
  if (strncasecmp("User-Agent", str, 10) == 0)
      return kHttpUserAgent;
  if (strncasecmp("Referer", str, 7) == 0)
      return kHttpReferer;
  if (strncasecmp("X-Forwarded-For", str, 15) == 0)
      return kHttpXForwardedFor;
  if (strncasecmp("Origin", str, 6) == 0)
      return kHttpOrigin;
  if (strncasecmp("Upgrade-Insecure-Requests", str, 25) == 0)
      return kHttpUpgradeInsecureRequests;
  if (strncasecmp("Pragma", str, 6) == 0)
      return kHttpPragma;
  if (strncasecmp("Cookie", str, 6) == 0)
      return kHttpCookie;
  if (strncasecmp("DNT", str, 3) == 0)
      return kHttpDnt;
  if (strncasecmp("Sec-GPC", str, 7) == 0)
      return kHttpSecGpc;
  if (strncasecmp("From", str, 4) == 0)
      return kHttpFrom;
  if (strncasecmp("If-Modified-Since", str, 17) == 0)
      return kHttpIfModifiedSince;
  if (strncasecmp("X-Requested-With", str, 16) == 0)
      return kHttpXRequestedWith;
  if (strncasecmp("X-Forwarded-Host", str, 16) == 0)
      return kHttpXForwardedHost;
  if (strncasecmp("X-Forwarded-Proto", str, 17) == 0)
      return kHttpXForwardedProto;
  if (strncasecmp("X-CSRF-Token", str, 12) == 0)
      return kHttpXCsrfToken;
  if (strncasecmp("Save-Data", str, 9) == 0)
      return kHttpSaveData;
  if (strncasecmp("Range", str, 5) == 0)
      return kHttpRange;
  if (strncasecmp("Content-Length", str, 14) == 0)
      return kHttpContentLength;
  if (strncasecmp("Content-Type", str, 12) == 0)
      return kHttpContentType;
  if (strncasecmp("Vary", str, 4) == 0)
      return kHttpVary;
  if (strncasecmp("Date", str, 4) == 0)
      return kHttpDate;
  if (strncasecmp("Server", str, 6) == 0)
      return kHttpServer;
  if (strncasecmp("Expires", str, 7) == 0)
      return kHttpExpires;
  if (strncasecmp("Content-Encoding", str, 16) == 0)
      return kHttpContentEncoding;
  if (strncasecmp("Last-Modified", str, 13) == 0)
      return kHttpLastModified;
  if (strncasecmp("ETag", str, 4) == 0)
      return kHttpEtag;
  if (strncasecmp("Allow", str, 5) == 0)
      return kHttpAllow;
  if (strncasecmp("Content-Range", str, 13) == 0)
      return kHttpContentRange;
  if (strncasecmp("Accept-Charset", str, 14) == 0)
      return kHttpAcceptCharset;
  if (strncasecmp("Access-Control-Allow-Credentials", str, 32) == 0)
      return kHttpAccessControlAllowCredentials;
  if (strncasecmp("Access-Control-Allow-Headers", str, 28) == 0)
      return kHttpAccessControlAllowHeaders;
  if (strncasecmp("Access-Control-Allow-Methods", str, 28) == 0)
      return kHttpAccessControlAllowMethods;
  if (strncasecmp("Access-Control-Allow-Origin", str, 27) == 0)
      return kHttpAccessControlAllowOrigin;
  if (strncasecmp("Access-Control-MaxAge", str, 21) == 0)
      return kHttpAccessControlMaxAge;
  if (strncasecmp("Access-Control-Method", str, 21) == 0)
      return kHttpAccessControlMethod;
  if (strncasecmp("Access-Control-RequestHeaders", str, 29) == 0)
      return kHttpAccessControlRequestHeaders;
  if (strncasecmp("Access-Control-Request-Method", str, 29) == 0)
      return kHttpAccessControlRequestMethod;
  if (strncasecmp("Access-Control-Request-Methods", str, 30) == 0)
      return kHttpAccessControlRequestMethods;
  if (strncasecmp("Age", str, 3) == 0)
      return kHttpAge;
  if (strncasecmp("Authorization", str, 13) == 0)
      return kHttpAuthorization;
  if (strncasecmp("Content-Base", str, 12) == 0)
      return kHttpContentBase;
  if (strncasecmp("Content-Description", str, 19) == 0)
      return kHttpContentDescription;
  if (strncasecmp("Content-Disposition", str, 19) == 0)
      return kHttpContentDisposition;
  if (strncasecmp("Content-Language", str, 16) == 0)
      return kHttpContentLanguage;
  if (strncasecmp("Content-Location", str, 16) == 0)
      return kHttpContentLocation;
  if (strncasecmp("Content-MD5", str, 11) == 0)
      return kHttpContentMd5;
  if (strncasecmp("Expect", str, 6) == 0)
      return kHttpExpect;
  if (strncasecmp("If-Match", str, 8) == 0)
      return kHttpIfMatch;
  if (strncasecmp("If-None-Match", str, 13) == 0)
      return kHttpIfNoneMatch;
  if (strncasecmp("If-Range", str, 8) == 0)
      return kHttpIfRange;
  if (strncasecmp("If-Unmodified-Since", str, 19) == 0)
      return kHttpIfUnmodifiedSince;
  if (strncasecmp("Keep-Alive", str, 10) == 0)
      return kHttpKeepAlive;
  if (strncasecmp("Link", str, 4) == 0)
      return kHttpLink;
  if (strncasecmp("Location", str, 8) == 0)
      return kHttpLocation;
  if (strncasecmp("Max-Forwards", str, 12) == 0)
      return kHttpMaxForwards;
  if (strncasecmp("Proxy-Authenticate", str, 18) == 0)
      return kHttpProxyAuthenticate;
  if (strncasecmp("Proxy-Authorization", str, 19) == 0)
      return kHttpProxyAuthorization;
  if (strncasecmp("Proxy-Connection", str, 16) == 0)
      return kHttpProxyConnection;
  if (strncasecmp("Public", str, 6) == 0)
      return kHttpPublic;
  if (strncasecmp("Retry-After", str, 11) == 0)
      return kHttpRetryAfter;
  if (strncasecmp("TE", str, 2) == 0)
      return kHttpTe;
  if (strncasecmp("Trailer", str, 7) == 0)
      return kHttpTrailer;
  if (strncasecmp("Transfer-Encoding", str, 17) == 0)
      return kHttpTransferEncoding;
  if (strncasecmp("Upgrade", str, 7) == 0)
      return kHttpUpgrade;
  if (strncasecmp("Warning", str, 7) == 0)
      return kHttpWarning;
  if (strncasecmp("WWW-Authenticate", str, 16) == 0)
      return kHttpWwwAuthenticate;
  if (strncasecmp("Via", str, 3) == 0)
      return kHttpVia;
  if (strncasecmp("Strict-Transport-Security", str, 25) == 0)
      return kHttpStrictTransportSecurity;
  if (strncasecmp("X-Frame-Options", str, 15) == 0)
      return kHttpXFrameOptions;
  if (strncasecmp("X-Content-Type-Options", str, 22) == 0)
      return kHttpXContentTypeOptions;
  if (strncasecmp("Alt-Svc", str, 7) == 0)
      return kHttpAltSvc;
  if (strncasecmp("Referrer-Policy", str, 15) == 0)
      return kHttpReferrerPolicy;
  if (strncasecmp("X-XSS-Protection", str, 16) == 0)
      return kHttpXXssProtection;
  if (strncasecmp("Accept-Ranges", str, 13) == 0)
      return kHttpAcceptRanges;
  if (strncasecmp("Set-Cookie", str, 10) == 0)
      return kHttpSetCookie;
  if (strncasecmp("Sec-CH-UA", str, 9) == 0)
      return kHttpSecChUa;
  if (strncasecmp("Sec-CH-UA-Mobile", str, 16) == 0)
      return kHttpSecChUaMobile;
  if (strncasecmp("Sec-CH-UA-Platform", str, 18) == 0)
      return kHttpSecChUaPlatform;
  if (strncasecmp("Sec-Fetch-Site", str, 14) == 0)
      return kHttpSecFetchSite;
  if (strncasecmp("Sec-Fetch-Mode", str, 14) == 0)
      return kHttpSecFetchMode;
  if (strncasecmp("Sec-Fetch-User", str, 14) == 0)
      return kHttpSecFetchUser;
  if (strncasecmp("Sec-Fetch-Dest", str, 14) == 0)
      return kHttpSecFetchDest;
  if (strncasecmp("CF-RAY", str, 6) == 0)
      return kHttpCfRay;
  if (strncasecmp("CF-Visitor", str, 10) == 0)
      return kHttpCfVisitor;
  if (strncasecmp("CF-Connecting-IP", str, 16) == 0)
      return kHttpCfConnectingIp;
  if (strncasecmp("CF-IPCountry", str, 12) == 0)
      return kHttpCfIpcountry;
  if (strncasecmp("CDN-Loop", str, 8) == 0)
      return kHttpCdnLoop;

  return -1;
}

/**
 * Returns small number for HTTP header, or -1 if not found.
 */
int GetHttpHeader(const char *str, size_t len)
{
  (void)len;
  return LookupHttpHeader(str);

// JD-NOTE: This is the original code below
//  const struct HttpHeaderSlot *slot;
//  if ((slot = LookupHttpHeader(str, len))) {
//    return slot->code;
//  } else {
//    return -1;
//  }
}
