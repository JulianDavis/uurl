#include <string.h>
#include <stdint.h>

#include "uurl.h"

#define HTTP_SERVER_PORT "8090"
#define HTTP_SERVER_IP "127.0.0.1"
#define HTTP_SERVER_ROOT ""

int main(void)
{
    struct uurl *uurl = uurl_init();
    if (!uurl)
        return 1;

    if (!uurl_connect(uurl, HTTP_SERVER_IP, HTTP_SERVER_PORT, 3))
        return 1;
    uurl_get_request(uurl, HTTP_SERVER_ROOT"/hello");

    //if (!uurl_connect(uurl, HTTP_SERVER_IP, HTTP_SERVER_PORT, 3))
    //    return 1;
    uurl_get_request(uurl, HTTP_SERVER_ROOT"/headers");

    //if (!uurl_connect(uurl, HTTP_SERVER_IP, HTTP_SERVER_PORT, 3))
    //    return 1;
    uurl_get_request(uurl, HTTP_SERVER_ROOT"/not-a-file");

    //const char *post_data = "Hello, World!";
    //uurl_connect(uurl, "httpbin.org", 3);
    //uurl_post_request(uurl, "/post", (uint8_t *)post_data, strlen(post_data));

    uurl_free(uurl);
}
