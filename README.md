# WebSocket-C

Simple WebSocket library in C.

Currently only implements the client side to talk to a server.

## Example

```c
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "websocket.h"

#define LISTENER_URL "ws://127.0.0.1:3000/ws"

int main(int argc, char **argv) {
  struct ws_client_t client;
  if (!ws_client_from_str(LISTENER_URL, strlen(LISTENER_URL), &client)) {
    fprintf(stderr, "client from string URL failed.\n");
    return 1;
  }
  if (!ws_client_connect(&client)) {
    fprintf(stderr, "client failed to connect.\n");
    return 1;
  }
  while (1) {
    printf("waiting for messages...\n");
    struct ws_message_t *msg = NULL;
    if (!ws_client_next_msg(&client, &msg)) {
      fprintf(stderr, "client failed to recv.\n");
      break;
    }
    if (msg == NULL) {
      fprintf(stderr, "message was null\n");
      break;
    }

    // handle message here

    ws_message_free(msg);
    free(msg);
  }
  ws_client_free(&client);
  return 0;
}
```
