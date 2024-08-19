#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#include "http.h"

#define hasatleast

// JD-NOTE: Stub out any unused functions from http.h

const char *GetHttpReason(int)
{
    return NULL;
}

const char *GetHttpHeaderName(int)
{
    return NULL;
}

bool HeaderHas(struct HttpMessage *, const char *, int, const char *,size_t)
{
    return false;
}

int64_t ParseContentLength(const char *, size_t)
{
    return -1;
}

char *FormatHttpDateTime(char[hasatleast 30], struct tm *)
{
    return NULL;
}

bool ParseHttpRange(const char *, size_t, long, long *, long *)
{
    return false;
}

int64_t ParseHttpDateTime(const char *, size_t)
{
    return -1;
}

uint64_t ParseHttpMethod(const char *, size_t)
{
    return 0;
}

bool IsValidHttpToken(const char *, size_t)
{
    return false;
}

bool IsValidCookieValue(const char *, size_t)
{
    return false;
}

bool IsAcceptablePath(const char *, size_t)
{
    return false;
}

bool IsAcceptableHost(const char *, size_t)
{
    return false;
}

bool IsAcceptablePort(const char *, size_t)
{
    return false;
}

bool IsReasonablePath(const char *, size_t)
{
    return false;
}

int ParseForwarded(const char *, size_t, uint32_t *, uint16_t *)
{
    return -1;
}

bool IsMimeType(const char *, size_t, const char *)
{
    return false;
}

ssize_t Unchunk(struct HttpUnchunker *, char *, size_t, size_t *)
{
    return -1;
}

const char *FindContentType(const char *, size_t)
{
    return NULL;
}

bool IsNoCompressExt(const char *, size_t)
{
    return false;
}

char *FoldHeader(struct HttpMessage *, const char *, int, size_t *)
{
    return NULL;
}
