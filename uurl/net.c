#define _GNU_SOURCE
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <poll.h>
#include <stdbool.h>

#include "net.h"
#include "debug.h"
#include "io.h"

// TODO: This needs to change to be a port on uurl with a default of 80 but with an option to specify something else
#define GETADDRINFO_SERVICE "http"

static bool try_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int timeout)
{
    if (connect(sockfd, addr, addrlen) == 0) {
        debug_print("[TRACE] connection successful\n");
        return true;
    }
    if (errno != EINPROGRESS && errno != EAGAIN) {
        debug_print("[ERR] connect: [%d] %m\n", errno);
        return false;
    }

    struct pollfd fds = {
        .fd = sockfd,
        .events = POLLOUT,
    };

    int retval = poll_interruptable(&fds, 1, timeout);
    if (retval == -1) {
        debug_print("[ERR] poll_interruptable: [%d] %m\n", errno);
        return false;
    }

    int sockerr = 0;
    socklen_t sockerr_len = sizeof(sockerr);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &sockerr, &sockerr_len) == -1) {
        debug_print("[ERR] getsockopt: [%d] %m\n", errno);
        return false;
    }

    return sockerr == 0;
}

int try_connect_host(const char *host, const char *port, int timeout)
{
    int sockfd = -1;

    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
     };
    struct addrinfo *results = NULL;

    int gai = getaddrinfo(host, port, &hints, &results);
    if (gai != 0) {
        debug_print("[ERR] getaddrinfo: [%d] %s\n", gai, gai_strerror(gai));
        return -1;
    }

    for(struct addrinfo *tmp = results; tmp; tmp = tmp->ai_next) {
        // JD-NOTE: SOCK_NONBLOCK was added in 2.6.27, this could be a problem.
        sockfd = socket(tmp->ai_family, tmp->ai_socktype | SOCK_NONBLOCK, tmp->ai_protocol);
        if (sockfd == -1) {
            debug_print("[ERR] socket: [%d] %m\n", errno);
            continue;
        }

        if (try_connect(sockfd, tmp->ai_addr, tmp->ai_addrlen, timeout)) {
            debug_print("[TRACE] successfully connected\n"); // TODO: Print addr
            break;
        }

        debug_print("[ERR] connect: [%d] %m\n", errno);
        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(results);
    return sockfd;
}
