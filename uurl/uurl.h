
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct uurl;
struct uurl_response;

struct uurl *uurl_init(void);
void uurl_free(struct uurl *uurl);

struct uurl_response *uurl_get_request(struct uurl *uurl, const char *resource);
struct uurl_response *uurl_post_request(struct uurl *uurl, const char *resource, const uint8_t *data, size_t data_size);
bool uurl_connect(struct uurl *uurl, const char *host, const char *port, int timeout);
