#define _GNU_SOURCE
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

#include "net.h"
#include "io.h"
#include "uurl.h"
#include "debug.h"

#define DEFAULT_TIMEOUT_MS 3000

#define MAX_HOST_LEN 2000
#define MAX_RESP_LEN 1000
#define MAX_REQ_LEN 4096

#define SENDFMT(fd, fmt, ...)            \
	do {                                 \
		printf("> " fmt, ##__VA_ARGS__); \
		dprintf(fd, fmt, ##__VA_ARGS__); \
	} while (0);

struct uurl {
    char *host;
    int sockfd;
};

struct uurl_response {
    int unknown;
};

struct uurl *uurl_init(void)
{
    struct uurl *uurl = malloc(sizeof(*uurl));
    if (!uurl) {
        debug_print("[ERR] malloc: [%d] %m\n", errno);
        return NULL;
    }

    return uurl;
}

bool uurl_connect(struct uurl *uurl, const char *host, const char *port, int timeout)
{
    uurl->sockfd = try_connect_host(host, port, timeout);
    if (uurl->sockfd == -1) {
        debug_print("[ERR] Failed to connect to the host\n");
        return false;
    }

    if (strlen(host) >= MAX_HOST_LEN) {
        debug_print("[ERR] Host is longer than max: %zu > %d\n", strlen(host), MAX_HOST_LEN);
        return false;
    }
    uurl->host = strdup(host);

    return true;
}

void uurl_free(struct uurl *uurl)
{
    if (!uurl)
        return;

    if (uurl->sockfd >= 0) {
        close(uurl->sockfd);
        uurl->sockfd = -1;
    }

    free(uurl->host);
    uurl->host = NULL;

    free(uurl);
}

struct uurl_response *uurl_get_request(struct uurl *uurl, const char *resource)
{
    /*

    Check if there is a connection
        Attempt to connect if needed
    Send a request
        Handle a disconnect here and try to reconnect?
    Read the response
        Read a fixed buf length
        If EOF is read the server has shutdown its write-side of the socket and the connection should be closed
        Parse the request and if it's complete then you're done
            If not then continue at step #1





    */



    SENDFMT(uurl->sockfd, "GET %s HTTP/1.1\r\n", resource);
    SENDFMT(uurl->sockfd, "Host: %s\r\n", uurl->host);
    SENDFMT(uurl->sockfd, "Connection: keep-alive\r\n");
    dprintf(uurl->sockfd, "\r\n");

    FILE *test = fdopen(uurl->sockfd, "r+");
    if (!test) {
        debug_print("[ERR] fdopen: [%d] %m\n", errno);
        return NULL;
    }

    char response[MAX_RESP_LEN] = { 0 };
    //size_t read_size = fread(response, sizeof(response), 1, test);
    ssize_t read_size = read_with_timeout(uurl->sockfd, response, sizeof(response), DEFAULT_TIMEOUT_MS);
    debug_print("[TRACE] read_size: %zd\n", read_size);
    (void)read_size;
    debug_print("\n\n%s\n", response);

    return NULL;
}

struct uurl_response *uurl_post_request(struct uurl *uurl, const char *resource, const uint8_t *data, size_t data_size)
{
    SENDFMT(uurl->sockfd, "POST %s HTTP/1.1\r\n", resource);
    SENDFMT(uurl->sockfd, "Host: %s\r\n", uurl->host);
    SENDFMT(uurl->sockfd, "Content-Length: %zu\r\n", data_size);
    SENDFMT(uurl->sockfd, "Connection: keep-alive\r\n");
    dprintf(uurl->sockfd, "\r\n");

    write_with_timeout(uurl->sockfd, data, data_size, DEFAULT_TIMEOUT_MS);

    char response[MAX_RESP_LEN] = { 0 };
    read_with_timeout(uurl->sockfd, response, sizeof(response), DEFAULT_TIMEOUT_MS);
    debug_print("\n\n%s\n", response);

    return NULL;
}
