
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct uurl;
struct uurl_response;

struct uurl *uurl_init(void);
void uurl_free(struct uurl *uurl);

// curl_easy_setopt(curl, CURLOPT_URL, "http://example.com/");
// res = curl_easy_perform(curl);

// curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_callback);
// size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
// curl_easy_setopt(handle, CURLOPT_WRITEDATA, custom_pointer);

// curl_easy_setopt(handle, CURLOPT_READFUNCTION, read_callback);

// size_t read_callback(char *buffer, size_t size, size_t nitems, void *stream);
// curl_easy_setopt(handle, CURLOPT_READDATA, custom_pointer);

struct uurl_response *uurl_get_request(struct uurl *uurl, const char *resource);
struct uurl_response *uurl_post_request(struct uurl *uurl, const char *resource, const uint8_t *data, size_t data_size);
bool uurl_connect(struct uurl *uurl, const char *host, const char *port, int timeout);
