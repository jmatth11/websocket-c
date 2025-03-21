#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "websocket.h"

static void print_buf(char *buf) {
  const size_t len = strlen(buf);
  printf("msg: ");
  for (int i = 0; i < len; ++i) {
    printf("%c", buf[i]);
  }
  printf("\n");
}

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
    char *response = NULL;
    if (!ws_client_recv(&client, &response)) {
      fprintf(stderr, "client failed to recv.\n");
      break;
    }
    print_buf(response);
    free(response);
  }
  ws_client_free(&client);
  return 0;
}
