# µURL API Design Plan

## Overview

µURL is a lightweight HTTP client library designed for embedded Linux systems with limited resources. It provides essential HTTP client functionality inspired by libcurl but with a significantly lighter footprint and focused API.

## Design Goals

- **Minimal Memory Footprint**: Optimized for embedded systems with limited resources
- **Simple API**: Intuitive interface that's easy to use but powerful
- **IPv4 Support**: Focus on IPv4 for older embedded Linux kernels
- **Standard C Library**: Only depend on the standard C library
- **Libcurl-inspired**: Familiar to users of libcurl but simpler

## Core API Design

### Client Handle

```c
// Opaque handle to a µURL session
typedef struct uurl_client* uurl_handle;
```

### Core Functions

```c
// Initialize a new µURL session
uurl_handle uurl_init(void);

// Clean up and free all resources
void uurl_cleanup(uurl_handle handle);

// Perform a GET request
uurl_code uurl_get(uurl_handle handle, const char* url, uurl_response** response);

// Perform a POST request
uurl_code uurl_post(uurl_handle handle, const char* url,
                  const void* data, size_t data_size,
                  uurl_response** response);

// Free a response
void uurl_response_free(uurl_response* response);
```

### Error Handling

Error handling uses status codes with X-macros for automatic string generation:

```c
// Error codes definition
typedef enum {
    UURL_OK,                // No error
    UURL_INIT_FAILED,       // Failed to initialize
    UURL_URL_PARSE_ERROR,   // Failed to parse URL
    UURL_CONNECT_FAILED,    // Failed to connect to host
    UURL_SEND_FAILED,       // Failed to send request
    UURL_RECV_FAILED,       // Failed to receive response
    UURL_MEMORY_ERROR,      // Memory allocation failed
    UURL_TIMEOUT,           // Operation timed out
    UURL_HTTP_ERROR,        // HTTP protocol error
    UURL_UNSUPPORTED        // Unsupported feature or protocol
    // More error codes as needed
} uurl_code;

// Get error string from error code
const char* uurl_strerror(uurl_code code);
```

### Response Structure

```c
// HTTP response structure
typedef struct uurl_response {
    int status_code;        // HTTP status code
    char* body;             // Response body (null-terminated)
    size_t body_length;     // Actual body length (excluding null terminator)
    char** headers;         // Array of headers
    size_t header_count;    // Number of headers
} uurl_response;
```

### Configuration Options

```c
// Set a custom header
uurl_code uurl_add_header(uurl_handle handle, const char* header);

// Clear all custom headers
void uurl_clear_headers(uurl_handle handle);

// Set connection timeout in milliseconds
uurl_code uurl_set_timeout(uurl_handle handle, long timeout_ms);

// Set maximum receive speed (bytes per second)
uurl_code uurl_set_max_recv_speed(uurl_handle handle, size_t bytes_per_second);

// Set maximum send speed (bytes per second)
uurl_code uurl_set_max_send_speed(uurl_handle handle, size_t bytes_per_second);
```

### Callback Support

```c
// Write callback function type
typedef size_t (*uurl_write_callback)(char *ptr, size_t size, size_t nmemb, void *userdata);

// Read callback function type
typedef size_t (*uurl_read_callback)(char *buffer, size_t size, size_t nmemb, void *userdata);

// Set write callback for response data
uurl_code uurl_set_write_callback(uurl_handle handle, uurl_write_callback callback, void* userdata);

// Set read callback for request data
uurl_code uurl_set_read_callback(uurl_handle handle, uurl_read_callback callback, void* userdata);
```

### Connection Management Options

```c
// Control connection reuse behavior
uurl_code uurl_set_connection_reuse(uurl_handle handle, bool enable);

// Set maximum number of connections in the pool (for future expansion)
uurl_code uurl_set_max_connections(uurl_handle handle, unsigned int max_connections);

// Set connection cache timeout (in seconds)
uurl_code uurl_set_connection_timeout(uurl_handle handle, long timeout_seconds);
```

## Key Design Decisions

### 1. URL Format

µURL accepts full URLs (e.g., "http://example.com:8080/path?query") like libcurl, making it intuitive and familiar.

```c
// Example usage
uurl_handle client = uurl_init();
uurl_response* response = NULL;
uurl_get(client, "http://example.com/api/resource", &response);
```

### 2. Automatic Connection Management

Connections are managed automatically like in libcurl:

- Connections are pooled and reused automatically
- Connections respect HTTP protocol rules (Connection: close headers)
- Pool size and behavior can be configured with options
- Connection reuse is enabled by default

### 3. Default Callbacks

Callbacks default to standard file operations:

- Default write callback uses `fwrite()` to stdout
- Default read callback uses `fread()` from stdin
- Custom callbacks can be set for specialized behavior

### 4. Memory Management

µURL manages memory for responses and other resources:

- Response objects are created by the library
- Users must free response objects with `uurl_response_free()`
- Internal resources are managed automatically

### 5. Error Handling

Error handling uses error codes with associated error strings:

- Functions return status codes
- Error strings can be retrieved with `uurl_strerror()`
- X macros maintain consistency between error codes and strings

## Implementation Strategy

### Phase 1: Core Functionality

1. URL parsing
2. Basic HTTP client (GET and POST)
3. Connection management with simple reuse
4. Basic response handling

### Phase 2: Enhanced Features

1. Header management
2. Full callback support
3. Full connection pooling
4. Speed control

### Phase 3: Advanced Features

1. Chunked transfer encoding
2. TLS support (mbedTLS)
3. More HTTP methods (PUT, DELETE, etc.)

## Usage Examples

### Simple GET Request

```c
// Initialize the client
uurl_handle client = uurl_init();
if (!client) {
    fprintf(stderr, "Failed to initialize µURL\n");
    return 1;
}

// Perform a GET request
uurl_response* response = NULL;
uurl_code result = uurl_get(client, "http://example.com/api/resource", &response);

if (result != UURL_OK) {
    fprintf(stderr, "Request failed: %s\n", uurl_strerror(result));
    uurl_cleanup(client);
    return 1;
}

// Use the response
printf("Status code: %d\n", response->status_code);
printf("Body: %s\n", response->body);

// Clean up
uurl_response_free(response);
uurl_cleanup(client);
```

### POST Request with Custom Headers

```c
// Initialize the client
uurl_handle client = uurl_init();

// Add custom headers
uurl_add_header(client, "Content-Type: application/json");
uurl_add_header(client, "X-API-Key: abcd1234");

// Create POST data
const char* data = "{\"name\":\"John\",\"age\":30}";

// Perform a POST request
uurl_response* response = NULL;
uurl_code result = uurl_post(client, "http://example.com/api/users",
                          data, strlen(data), &response);

// Use the response
if (result == UURL_OK) {
    printf("Status code: %d\n", response->status_code);
    printf("Body: %s\n", response->body);
    uurl_response_free(response);
}

// Clean up
uurl_cleanup(client);
```

### Using Callbacks

```c
// Custom write callback
size_t my_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    // Process received data
    // ...
    return size * nmemb;
}

// Initialize the client
uurl_handle client = uurl_init();

// Set custom callback
uurl_set_write_callback(client, my_write_callback, my_userdata);

// Perform a GET request
uurl_code result = uurl_get(client, "http://example.com/api/resource", NULL);

// Clean up
uurl_cleanup(client);
```

## Internal Architecture

### Client Structure

The client structure manages state for requests, including:

- Connection pool
- Custom headers
- Callbacks and userdata
- Timeout and speed settings

### Connection Pooling

Connections are managed in a simple pool:

- Connections are identified by host and port
- Recently used connections are reused when possible
- Idle connections time out after a configurable period
- Pool size is configurable

### URL Parsing

URLs are parsed into components:
- Scheme (http)
- Host
- Port
- Path
- Query string
- Fragment

### HTTP Message Handling

HTTP parsing uses the existing http.h API:
- Request formatting
- Response parsing
- Header extraction

This design provides a clean, intuitive API for µURL while maintaining the lightweight nature required for embedded systems. The automatic connection management with optional configuration offers the best of both worlds - simplicity for basic use and flexibility for advanced scenarios.
