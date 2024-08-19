#pragma once
#include <errno.h>

#define einval() ({errno = EINVAL; -1;})
#define ebadmsg() ({errno = EBADMSG; -1;})
