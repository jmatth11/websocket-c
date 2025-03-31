#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "headers/protocol.h"
#include "headers/websocket.h"
#include "unicode_str.h"

static void print_buf(char *buf) {
  const size_t len = strlen(buf);
  printf("msg: ");
  for (int i = 0; i < len; ++i) {
    printf("%c", buf[i]);
  }
  printf("\n");
}
static void print_byte_array(byte_array *buf) {
  const size_t len = buf->len;
  printf("msg: ");
  for (int i = 0; i < len; ++i) {
    printf("%c", buf->byte_data[i]);
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
    printf("waiting for messages...\n");
    byte_array response;
    if (!ws_client_recv(&client, &response)) {
      fprintf(stderr, "client failed to recv.\n");
      break;
    }
    printf("response:\n");
    print_byte_array(&response);
    //printf("\nframe:\n");
    //struct ws_frame_t frame;
    //if (!ws_frame_init(&frame)) {
    //  fprintf(stderr, "frame failed to initialize.\n");
    //  return 1;
    //}
    //size_t resp_len = strlen(response);
    //byte_array buf;
    //if (!byte_array_init(&buf, resp_len)) {
    //  fprintf(stderr, "failed to init byte array.\n");
    //  return 1;
    //}
    //for (size_t i = 0; i < resp_len; ++i) {
    //  byte_array_insert(&buf, (uint8_t)response[i]);
    //}
    //enum ws_frame_error_t err = ws_frame_read(&frame, buf.byte_data, buf.len);
    //if (err != WS_FRAME_SUCCESS) {
    //  fprintf(stderr, "frame read failed: %d\n", err);
    //  return 1;
    //}
    //// print_buf(response);
    //if (frame.codes.flags.opcode >= OPCODE_CLOSE) {
    //  printf("control frame: %d\n", frame.codes.flags.opcode);
    //} else {
    //  print_byte_array(&frame.payload);
    //}
    //ws_frame_free(&frame);
    //byte_array_free(&buf);
    byte_array_free(&response);
  }
  ws_client_free(&client);
  return 0;
}
