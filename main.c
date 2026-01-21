#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "headers/protocol.h"
#include "headers/reader.h"
#include "headers/websocket.h"
#include "unicode_str.h"

// defined constants
#define STDIN_BUF 100
#define LISTENER_URL "ws://localhost:80/ws"

// print the byte array
static void print_byte_array(byte_array *buf) {
  const size_t len = buf->len;
  printf("msg: ");
  for (size_t i = 0; i < len; ++i) {
    printf("%c", buf->byte_data[i]);
  }
  printf("\n");
}

// the callback function for the websocket client to use
static bool callback(struct ws_client_t *client, struct ws_message_t *msg,
                     void *context) {
  // unused params
  (void)client;
  (void)context;
  // print the msg
  printf("response(type:%d):\n", msg->type);
  print_byte_array(&msg->body);
  // return true for successful execution.
  return true;
}

// thread function to listen to messages on a separate thread.
void *client_msg(void *ctx) {
  printf("listening for messages...\n");
  struct ws_client_t *client = (struct ws_client_t *)ctx;
  // this call blocks as long as the client is open.
  ws_client_on_msg(client, callback, NULL);
  return NULL;
}

int main(void) {
  // create websocket client.
  struct ws_client_t client;
  ws_client_init(&client);
  // create from URL
  if (!ws_client_from_str(LISTENER_URL, strlen(LISTENER_URL), &client)) {
    fprintf(stderr, "client from string URL failed.\n");
    return 1;
  }
  // connect
  if (!ws_client_connect(&client)) {
    fprintf(stderr, "client failed to connect.\n");
    return 1;
  }
  // setup background thread for listener
  pthread_t thread;
  const int ret_code = pthread_create(&thread, NULL, client_msg, &client);
  if (ret_code != 0) {
    fprintf(stderr, "pthread failed: %s\n", strerror(ret_code));
    return -1;
  }

  // setup stdin buffer to send messages on the client
  char stdin_buf[STDIN_BUF];
  memset(stdin_buf, 0, sizeof(char)*STDIN_BUF);
  byte_array send_buffer;
  if (!byte_array_init(&send_buffer, STDIN_BUF)) {
    fprintf(stderr, "failed to initialize buffer.\n");
    ws_client_free(&client);
    return -1;
  }
  do {
    printf("Type Here ('quit' to quit) > ");
    // grab user input
    (void)fgets(stdin_buf, STDIN_BUF, stdin);
    size_t index = 0;
    send_buffer.len = 0;
    // populate send buffer
    while(true) {
      send_buffer.byte_data[index] = (uint8_t)stdin_buf[index];
      send_buffer.len += 1;
      if (send_buffer.byte_data[index] == '\n') {
        break;
      }
      ++index;
      if (index >= STDIN_BUF) {
        break;
      }
    }
    // write message
    if (!ws_client_write(&client, OPCODE_TEXT, send_buffer)) {
      fprintf(stderr, "websocket write failed.\n");
      break;
    }
    // loop until quit is entered
  } while (strncmp("quit\n", stdin_buf, 5) != 0);

  // cleanup
  byte_array_free(&send_buffer);
  ws_client_free(&client);
  return 0;
}
