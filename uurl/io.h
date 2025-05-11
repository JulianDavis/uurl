#pragma once

#include <sys/types.h>
#include <poll.h>

ssize_t read_interruptable(int fd, void *buf, size_t count);
ssize_t write_interruptable(int fd, const void *buf, size_t count);
int poll_interruptable(struct pollfd *fds, nfds_t nfds, int timeout);

ssize_t write_with_timeout(int fd, const void *buf, size_t count, int timeout);
ssize_t read_with_timeout(int fd, void *buf, size_t count, int timeout);
