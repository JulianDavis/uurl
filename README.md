# µURL: Lightweight HTTP Client for Embedded Systems

µURL (pronounced "Earl") is a small and lightweight HTTP client library specifically designed for embedded Linux systems with limited resources. It aims to provide core HTTP client functionality while maintaining a minimal footprint.

## Table of Contents

- [Overview](#overview)
- [Current State](#current-state)
- [Implementation Plan](#implementation-plan)
  - [Core GET/POST Functionality](#core-getpost-functionality)
  - [Response Processing](#response-processing)
  - [Callback Support](#callback-support)
  - [Custom Headers](#custom-headers)
  - [Error Handling](#error-handling)
- [Implementation Priorities](#implementation-priorities)
- [Feature Roadmap](#feature-roadmap)
- [Advanced Features](#advanced-features)
  - [Connection Pooling](#connection-pooling)
  - [Chunked Transfer Encoding](#chunked-transfer-encoding)
  - [TLS Integration](#tls-integration)
- [Testing Strategy](#testing-strategy)
- [Contributing](#contributing)

## Overview

µURL is designed as a lightweight alternative to libcurl for embedded systems, focusing on essential HTTP client functionality. It provides a clean API for making HTTP requests and processing responses with minimal memory and CPU usage.

Key design goals:
- Minimal memory footprint
- Simple and intuitive API
- Support for embedded Linux systems with older kernels
- IPv4 support (no IPv6 dependency)
- No external dependencies beyond standard C libraries

## Current State

The current implementation includes:
- HTTP message parser for both requests and responses
- Socket connection handling with timeout management
- Interrupt-safe I/O operations
- Basic GET and POST request support
- Request and response header management

## Implementation Plan

### Core GET/POST Functionality

The request/response lifecycle consists of the following steps:

1. Establish a connection to the server
2. Format and send the HTTP request
3. Read and parse the HTTP response
4. Process any content encoding
5. Return the response with appropriate metadata

Example implementation pattern:

```c
struct uurl_response *uurl_get_request(struct uurl *u, const char *resource) {
    // 1. Check if connection exists or establish new one
    if (!ensure_connection(u)) {
        return NULL;
    }

    // 2. Format and send HTTP request
    if (!send_get_request(u, resource)) {
        return NULL;
    }

    // 3. Read and parse HTTP response
    struct uurl_response *resp = read_response(u);
    if (!resp) {
        return NULL;
    }

    // 4. Process response (handle chunked encoding, etc.)
    if (!process_response_body(u, resp)) {
        uurl_response_free(resp);
        return NULL;
    }

    // 5. Return completed response
    return resp;
}
```

### Response Processing

The response handling will be encapsulated in a dedicated structure:

```c
struct uurl_response {
    int status_code;               // HTTP status code
    struct http_message *message;  // Parsed HTTP message
    char *body;                    // Response body buffer
    size_t body_length;            // Length of response body
    bool is_chunked;               // Is transfer-encoding chunked?
    bool is_complete;              // Was the entire response read?
};

void uurl_response_free(struct uurl_response *response);
```

### Callback Support

To provide a flexible API similar to libcurl, callback mechanisms will be implemented:

```c
// Callback function signatures
typedef size_t (*uurl_write_callback)(char *ptr, size_t size, size_t nmemb, void *userdata);
typedef size_t (*uurl_read_callback)(char *buffer, size_t size, size_t nitems, void *userdata);

// API to set callbacks
void uurl_set_write_callback(struct uurl *u, uurl_write_callback callback, void *userdata);
void uurl_set_read_callback(struct uurl *u, uurl_read_callback callback, void *userdata);
```

The callbacks are invoked during request and response processing:
- The write callback is called when response data is received
- The read callback is called when request data needs to be sent (for POST/PUT)

### Custom Headers

Support for custom HTTP headers:

```c
// API for headers
bool uurl_add_header(struct uurl *u, const char *header);
void uurl_clear_headers(struct uurl *u);
```

Header management internally:

```c
struct {
    char **headers;
    size_t count;
    size_t capacity;
} custom_headers;
```

### Error Handling

A comprehensive error handling system will provide detailed information about failures:

```c
enum uurl_error {
    UURL_OK = 0,
    UURL_NETWORK_ERROR,
    UURL_TIMEOUT,
    UURL_PROTOCOL_ERROR,
    UURL_MEMORY_ERROR,
    // More error codes as needed
};

// Get human-readable error message
const char *uurl_strerror(enum uurl_error error);

// Get detailed error information
const char *uurl_get_error_message(struct uurl *u);
```

## Implementation Priorities

1. **Connection Management**: Properly establish, maintain, and reuse HTTP connections
2. **GET Functionality**: Implement reliable GET requests with proper response handling
3. **Response Parsing**: Correctly parse and extract data from HTTP responses
4. **POST Functionality**: Support POST requests with data uploads
5. **Callback System**: Implement the read/write callback mechanism
6. **Custom Headers**: Support for setting custom HTTP headers
7. **Error Handling**: Provide detailed error reporting

## Feature Roadmap

### Phase 1: Core Functionality (Current Focus)
- HTTP request/response parsing
- Socket connection handling
- Complete GET implementation
- Complete POST implementation
- Basic error handling
- Custom headers

### Phase 2: Advanced HTTP Features
- Chunked transfer encoding support
- Content-Length handling
- Automatic redirect following
- Connection persistence/reuse
- Timeout controls
- Speed limiting

### Phase 3: Security & Robustness
- TLS integration with mbedTLS (build-time integration)
- Certificate validation and security options
- Support for other TLS backends (OpenSSL, etc.)
- Request cancellation
- Memory usage optimizations

### Phase 4: Extended Features
- Custom DNS resolution
- HTTP authentication (Basic, Digest)
- Cookie handling
- Proxy support
- Compression (gzip, deflate)

## Advanced Features

### Connection Pooling

Connection pooling maintains a set of open, reusable network connections rather than establishing a new connection for each HTTP request. This optimization provides several benefits:

#### Benefits
- **Eliminates TCP Connection Overhead**: Avoids TCP handshake (SYN, SYN-ACK, ACK) for subsequent requests
- **Avoids SSL/TLS Handshake Costs**: Eliminates expensive cryptographic operations for HTTPS connections
- **Takes Advantage of TCP Slow Start**: Established connections have higher throughput
- **Reduces Resource Usage**: Fewer sockets, lower memory consumption, and less CPU usage
- **Extends Battery Life**: Minimizes radio activity on mobile/embedded devices

#### Implementation Design
```c
struct connection {
    char *host;
    char *port;
    int sockfd;
    bool is_secure;
    void *tls_context;  // If using TLS
    time_t last_used;   // Timestamp for idle timeout
};

struct connection_pool {
    struct connection *connections;
    size_t count;
    size_t capacity;
};
```

#### Connection Management Policies
- **Idle Timeout**: Close connections that haven't been used for a certain period
- **Maximum Pool Size**: Limit the total number of connections
- **Connection Validation**: Verify connections are still operational before reuse
- **FIFO Eviction**: Close the least recently used connection when the pool is full

### Chunked Transfer Encoding

Chunked transfer encoding is an HTTP/1.1 feature that allows sending data without knowing its total size in advance.

#### Implementation Design
```c
struct chunked_decoder {
    size_t chunk_remaining;
    bool receiving_chunk_size;
    bool trailer_headers;
    bool complete;
};
```

#### Chunked Decoding Process
1. Parse chunk size from hex string
2. Read chunk data (chunk_size bytes)
3. Expect CRLF after chunk
4. If chunk size is 0, process optional trailers then finish
5. Otherwise, return to step 1

### TLS Integration

TLS support will be implemented using a build-time integration approach, providing a clean internal API while optimizing for code size and performance. This approach is especially suitable for embedded systems where resources are constrained.

#### Build-time Integration

TLS functionality will be integrated at compile time using preprocessor directives:

```c
// In uurl_tls.h
#ifdef UURL_USE_MBEDTLS
#include <mbedtls/ssl.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#define UURL_TLS_SUPPORTED 1
#elif defined(UURL_USE_OPENSSL)
#include <openssl/ssl.h>
#include <openssl/err.h>
#define UURL_TLS_SUPPORTED 1
#else
#define UURL_TLS_SUPPORTED 0
#endif
```

#### Internal TLS API

While the TLS implementation is selected at build time, an internal API will provide a consistent interface for the rest of the library:

```c
// Internal functions used by the µURL library
struct uurl_tls_context;

// Initialize a TLS context for a connection
struct uurl_tls_context *uurl_tls_init(const char *hostname);

// Perform TLS handshake on an established socket connection
int uurl_tls_connect(struct uurl_tls_context *ctx, int sockfd);

// Read data through TLS connection
ssize_t uurl_tls_read(struct uurl_tls_context *ctx, void *buf, size_t len);

// Write data through TLS connection
ssize_t uurl_tls_write(struct uurl_tls_context *ctx, const void *buf, size_t len);

// Clean up TLS context
void uurl_tls_cleanup(struct uurl_tls_context *ctx);
```

#### TLS Configuration Options

```c
// TLS configuration structure
struct uurl_tls_config {
    const char *ca_cert_file;      // Path to CA certificate file
    const char *ca_cert_dir;       // Path to directory with CA certificates
    const char *client_cert_file;  // Client certificate for mutual TLS
    const char *client_key_file;   // Client private key
    bool verify_peer;              // Whether to verify server certificate
};

// Set TLS configuration for the client
void uurl_set_tls_config(struct uurl *u, const struct uurl_tls_config *config);
```

#### Implementation Strategy

1. **Initial Support**: First implement mbedTLS integration, as it's well-suited for embedded systems
2. **Backend Separation**: Keep all TLS-specific code in dedicated source files
3. **Clear Interfaces**: Maintain a consistent internal API between µURL core and TLS code
4. **Fallback Behavior**: Gracefully handle when TLS support is not compiled in

#### Adding New TLS Backends

The design allows for additional TLS backends to be added in the future:

1. Create new implementation files (e.g., `uurl_tls_openssl.c`)
2. Implement the internal TLS API functions for the new backend
3. Add appropriate preprocessor directives in the build system
4. Document the new build option

## Testing Strategy

A comprehensive testing strategy ensures the reliability and correctness of µURL:

1. **Unit Tests**: Test individual components in isolation
   - HTTP parser
   - Connection management
   - Request/response formatting
   - Callback mechanisms

2. **Integration Tests**: Test the library against real HTTP servers
   - Various HTTP server implementations
   - Different response types and status codes
   - Edge cases like redirects and errors

3. **Compliance Tests**: Verify behavior against RFC specifications
   - HTTP/1.1 conformance
   - Header handling
   - Content encoding support

4. **Performance Tests**: Measure resource usage
   - Memory consumption
   - CPU utilization
   - Bandwidth efficiency

5. **Stress Tests**: Test under challenging conditions
   - High concurrency
   - Unreliable network conditions
   - Resource constraints

## Contributing

Contributions to µURL are welcome! Here's how you can help:

1. **Bug Reports**: Submit detailed bug reports with reproduction steps
2. **Feature Requests**: Suggest new features or improvements
3. **Pull Requests**: Implement new features or fix bugs
4. **Documentation**: Improve or expand this documentation
5. **Testing**: Help test the library on different platforms and configurations

Please follow the project's coding style and include appropriate tests with your contributions.

---

This documentation is a work in progress and will be updated as µURL evolves.
