#include <unistd.h>
#include <poll.h>
#include <errno.h>

#include "io.h"
#include "debug.h"

ssize_t read_interruptable(int fd, void *buf, size_t count)
{
	ssize_t n = -1;

	do {
		n = read(fd, buf, count);
	} while (n < 0 && errno == EINTR);

	return n;
}

ssize_t write_interruptable(int fd, const void *buf, size_t count)
{
	ssize_t n = -1;

	do {
		n = write(fd, buf, count);
	} while (n < 0 && errno == EINTR);

	return n;
}

int poll_interruptable(struct pollfd *fds, nfds_t nfds, int timeout)
{
    int n = -1;

	do {
		n = poll(fds, nfds, timeout);
    } while (n < 0 && errno == EINTR);

    return n;
}

ssize_t write_with_timeout(int fd, const void *buf, size_t buf_size, int timeout)
{
    ssize_t total = 0;
    size_t remaining = buf_size;

    do {
        ssize_t written = write_interruptable(fd, buf, buf_size);
        if (written == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                debug_print("[ERR] write_interruptable: [%d] %m\n", errno);
                return -1;
            }

            struct pollfd fds = {
                .fd = fd,
                .events = POLLOUT,
            };

            int rcount = poll_interruptable(&fds, 1, timeout);
            if (rcount == 0) {
                debug_print("[TRACE] poll_interruptable timed out\n");
                return total;
            }
            if (rcount == -1) {
                debug_print("[ERR] poll_interruptable: [%d] %m\n", errno);
                return -1;
            }
            if (rcount == 1 && fds.revents & POLLOUT) {
                timeout--; // Back the timeout off slowly every time we poll
                continue;
            }

            debug_print("[ERR] unexpected revents: %d\n", fds.revents);
            return -1;
        }


        total += written;
        remaining -= written;
    } while (remaining > 0);

    return total;
}

// TODO: This is always waiting until the complete timeout, even on success. Figure out how to do this better
ssize_t read_with_timeout(int fd, void *buf, size_t buf_size, int timeout)
{
    ssize_t total = 0;
    size_t remaining = buf_size;

    do {
        ssize_t read = read_interruptable(fd, buf, buf_size);
        if (read == 0) {
            // JD-NOTE: This is a special case where the connection needs to be reset. Figure out the best way to bubble
            // that up.
            debug_print("[TRACE] Read EOF\n");
            return total;
        }
        if (read == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                debug_print("[ERR] read_interruptable: [%d] %m\n", errno);
                return -1;
            }

            struct pollfd fds = {
                .fd = fd,
                .events = POLLIN,
            };

            int rcount = poll_interruptable(&fds, 1, timeout);
            if (rcount == 0) {
                debug_print("[TRACE] poll_interruptable timed out\n");
                return total;
            }
            if (rcount == -1) {
                debug_print("[ERR] poll_interruptable: [%d] %m\n", errno);
                return -1;
            }
            if (rcount == 1 && fds.revents & POLLIN) {
                // TODO: Figure out backoff time, this is in miliseconds so obviously this is dumb. This was stolen from
                // busybox, should it even be kept?
                timeout--; // Back the timeout off slowly every time we poll
                continue;
            }

            // TODO: Need to handle the hangup case
            // This logic can probably be simplified to group with the -1 case
            debug_print("[ERR] unexpected revents: %d\n", fds.revents);
            return -1;
        }


        total += read;
        remaining -= read;
    } while (remaining > 0);

    return total;
}
