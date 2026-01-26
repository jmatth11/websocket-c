# WebSocket-C

Table of Contents:
- [Dependencies](#dependencies)
- [Build](#build)
    - [Build Options](#build-options)
- [Testing](#testing)
    - [Non-Secure](#non-secure)
    - [Secure](#secure)
- [Examples](#examples)
    - [Manual Loop](#manual-loop)
    - [Callback Loop](#callback-loop)
    - [OpenSSL Example](#openssl-example)
- [Demo](#demo)

A small Client-Side WebSocket library implemented in C.
Supports both `ws` and `wss` (if built with OpenSSL. see [build options](#build-options))

## Dependencies

To install all dependencies for the project run `./install_deps.sh`

Use the `./script/dev_deps.sh` file to install system dependencies for the project.
These include libraries or tools needed to build the project.

Use the `./script/lib_deps.sh` file to install library dependencies for the project.
These include libraries this library uses directly.

Required Deps:
- [Zig](https://github.com/ziglang/zig) The Zig programming language.
- [cstd](https://github.com/jmatth11/cstd) Common standard C functionality library.
- [utf8-zig](https://github.com/jmatth11/utf8-zig) Small UTF8 helper library written in Zig.

Optional Deps:
- [OpenSSL](https://openssl-library.org/source/) The OpenSSL library for secure
  connections. Only used if ssl flag is on when building.
  (`USE_SSL=1`/`-Duse_ssl`)

## Build

Quickstart:

Both methods are basically the same except:
- Zig build option will build a wasm target library as well.
- Makefile will build the test application.

Makefile
```bash
make RELEASE=1 USE_SSL=1 SHARED=1
```

Zig build:
```bash
zig build -Doptimize=ReleaseFast -Duse_ssl
```

### Build Options
- Makefile (builds are put in `./bin`)
    - `make` The default will build the main test executable. The test
      executable uses normal connection by default.
    - `SHARED=1` Will build the shared library for websocket-c.
    - `RELEASE=1` Builds with optimizations turned on.
    - `USE_SSL=1` Build with OpenSSL support. For the test executable it builds
      to use `wss`.
    - `DISABLE_SIMD` Force disable SIMD functionality.
- Zig (builds are put in `./zig-out/lib`)
    - `zig build -Doptimize=ReleaseFast` Builds the shared library and wasm
      library for websocket-c.
    - `-Duse_ssl` Builds with OpenSSL support.
    - `-Ddisable_simd` Force disable SIMD functionality.

## Testing

To run the test application there is a Go application provided to run against.

### Non-Secure

1. Build the test application with `make` to test non-secure functionality.
2. Inside the `test/server` directory you can build the Go application with `go
   build`.
3. Then run the application from that directory with `./test`.
4. Then from the root directory of the project run the test application with
   `./script/run.sh`.

You will be presented with a prompt in the terminal whatever you type (up-to
100 characters) will be sent to the server and echoed back to you.

### Secure

1. Build the test application with `make USE_SSL=1` to test secure functionality.
2. Inside the `test/` directory run the `./ssl_gen.sh` script from that
   directory to generate a `server.crt` certificate and `server.key` private
   key to use with the server and client.
3. Inside the `test/server` directory you can build the Go application with `go
   build`.
4. Then run the application from that directory with `sudo ./test tls`. The
   `tls` argument will start a TLS server.
5. Then from the root directory of the project run the test application with
   `./script/run.sh`.

You will be presented with a prompt in the terminal whatever you type (up-to
100 characters) will be sent to the server and echoed back to you.

## Examples

### Manual Loop

Example of using your own listener loop.

```c
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "websocket.h"

// URL
#define LISTENER_URL "ws://127.0.0.1:3000/ws"

int main(int argc, char **argv) {
  // construct our client from the URL string.
  struct ws_client_t client;
  if (!ws_client_from_str(LISTENER_URL, strlen(LISTENER_URL), &client)) {
    fprintf(stderr, "client from string URL failed.\n");
    return 1;
  }
  // connect the client to the server.
  if (!ws_client_connect(&client)) {
    fprintf(stderr, "client failed to connect.\n");
    return 1;
  }
  while (1) {
    printf("waiting for messages...\n");
    struct ws_message_t *msg = NULL;
    // waiting for the next message to be received.
    if (!ws_client_next_msg(&client, &msg)) {
      fprintf(stderr, "client failed to recv.\n");
      break;
    }
    if (msg == NULL) {
      fprintf(stderr, "message was null\n");
      break;
    }

    // handle message here

    // free the message contents and the message.
    ws_message_free(msg);
    free(msg);
  }
  // free the client.
  ws_client_free(&client);
  return 0;
}
```

### Callback Loop

Example of using the supplied `ws_client_on_msg` callback loop.

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "headers/reader.h"
#include "headers/websocket.h"

// URL
#define LISTENER_URL "ws://localhost:3000/ws"

static bool callback(struct ws_client_t *client, struct ws_message_t *msg, void *context) {
  // handle the message
  return true;
}

int main(int argc, char **argv) {
  // construct our client from the URL string.
  struct ws_client_t client;
  if (!ws_client_from_str(LISTENER_URL, strlen(LISTENER_URL), &client)) {
    fprintf(stderr, "client from string URL failed.\n");
    return 1;
  }

  // connect the client to the server.
  if (!ws_client_connect(&client)) {
    fprintf(stderr, "client failed to connect.\n");
    return 1;
  }
  printf("listening for messages...\n");
  // blocking call to handle messages on the given callback
  ws_client_on_msg(&client, callback, NULL);

  // free.
  ws_client_free(&client);
  return 0;
}
```

### OpenSSL Example

A simple example of using OpenSSL.

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "headers/reader.h"
#include "headers/websocket.h"

// URL
#define LISTENER_URL "wss://localhost:443/ws"

static bool callback(struct ws_client_t *client, struct ws_message_t *msg, void *context) {
  // handle the message
  return true;
}

int main(int argc, char **argv) {
  // Initialize the OpenSSL functionality.
  // This is an example of setting a self-signed cert.
  // For publicly hosted server you can pass NULL.
  net_init_client("./certs/server.crt", NULL);

  // construct our client from the URL string.
  struct ws_client_t client;
  if (!ws_client_from_str(LISTENER_URL, strlen(LISTENER_URL), &client)) {
    fprintf(stderr, "client from string URL failed.\n");
    return 1;
  }

  // Connect the client to the server.
  if (!ws_client_connect(&client)) {
    fprintf(stderr, "client failed to connect.\n");
    return 1;
  }
  printf("listening for messages...\n");

  // blocking call to handle messages on the given callback
  ws_client_on_msg(&client, callback, NULL);

  // free client
  ws_client_free(&client);
  // deinitialize OpenSSL
  net_deinit();
  return 0;
}
```

## Demo

This is a small demo running the test application with TLS turned on.

https://github.com/user-attachments/assets/f2d8a86d-c2c5-4319-b3e6-02c5c689be49
