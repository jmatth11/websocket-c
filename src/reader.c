#include "headers/reader.h"
#include "headers/protocol.h"
#include "queue.h"
#include <stdio.h>
#include <sys/socket.h>

struct ws_reader_t {
  struct simple_queue_t *frame_queue;
  struct simple_queue_t *msg_queue;
};

static void construct_msg_from_frames(struct ws_reader_t *reader) {
  struct ws_message_t *msg = malloc(sizeof(struct ws_message_t));
  if (!ws_message_init(msg)) {
    fprintf(stderr, "message init failed.\n");
    free(msg);
    return;
  }
  size_t frame_count = simple_queue_len(reader->frame_queue);
  for (; frame_count > 0; --frame_count) {
    struct ws_frame_t *frame = NULL;
    if (!simple_queue_pop(reader->frame_queue, (void**)&frame)){
      fprintf(stderr, "popping frame queue failed.\n");
      free(frame);
      ws_message_free(msg);
      free(msg);
      return;
    }
    if (frame->codes.flags.opcode != OPCODE_CONT) {
      msg->type = frame->codes.flags.opcode;
    }
    for (size_t i = 0; i < frame->payload.len; ++i) {
      if (!byte_array_insert(&msg->body, frame->payload.byte_data[i])) {
        fprintf(stderr, "concat msg failed\n");
        free(frame);
        ws_message_free(msg);
        free(msg);
        return;
      }
    }
    free(frame);
  }
  if (!simple_queue_push(reader->msg_queue, msg)) {
    fprintf(stderr, "msg queue failed\n");
    ws_message_free(msg);
    free(msg);
    return;
  }
}

struct ws_reader_t* ws_reader_create() {
  struct ws_reader_t *result = malloc(sizeof(struct ws_reader_t));
  result->frame_queue = simple_queue_create();
  result->msg_queue = simple_queue_create();
  return result;
}
bool ws_reader_handle(struct ws_reader_t *reader, int socket) {
  // 10 for possible 8 byte payload value
  uint8_t header[10] = {0};
#ifdef DEBUG
  printf("peeking from socket\n");
#endif
  ssize_t n = recv(socket, header, 10, MSG_PEEK);
  if (n == -1) {
    return false;
  }
#ifdef DEBUG
  printf("header=%s\n", header);
#endif
  struct ws_frame_t *frame = malloc(sizeof(struct ws_frame_t));;
  if (!ws_frame_init(frame)) {
    return false;
  }
  enum ws_frame_error_t err = ws_frame_read_header(frame, header, n);
  if (err != WS_FRAME_SUCCESS) {
    ws_frame_free(frame);
    free(frame);
    return false;
  }
#ifdef DEBUG
  ws_frame_print(frame);
#endif
  size_t payload_offset = 2 + ws_frame_payload_byte_len(frame);
  size_t msg_len = payload_offset + frame->payload_len;
  if (frame->info.flags.mask) {
    msg_len += 4;
  }
  uint8_t *buffer = malloc((sizeof(uint8_t)*msg_len+1));
#ifdef DEBUG
  printf("reading from socket\n");
#endif
  n = recv(socket, buffer, msg_len, 0);
  if (n != msg_len) {
    fprintf(stderr, "socket read did not match message length: n=%zu; msg_len:%zu\n", n, msg_len);
    ws_frame_free(frame);
    free(frame);
    return false;
  }
#ifdef DEBUG
  printf("payload_offset:%zu msg_len-poffset:%zu, char:%c\n", payload_offset, msg_len-payload_offset, buffer[payload_offset]);
  printf("buffer: %s\n", buffer);
#endif
  err = ws_frame_read_body(frame, &buffer[payload_offset], msg_len - payload_offset);
  if (err != WS_FRAME_SUCCESS) {
    fprintf(stderr, "read body failed with code: %d\n", err);
    ws_frame_free(frame);
    return false;
  }
  if (frame->codes.flags.fin) {
    if (frame->codes.flags.opcode >= OPCODE_CLOSE) {
      struct ws_message_t *msg = malloc(sizeof(struct ws_message_t));
      msg->type = frame->codes.flags.opcode;
      msg->body = frame->payload;
      frame->payload.byte_data = NULL;
      if (!simple_queue_push(reader->msg_queue, msg)) {
        free(msg);
        ws_frame_free(frame);
        free(frame);
        return false;
      }
    } else {
      if (!simple_queue_push(reader->frame_queue, frame)) {
        ws_frame_free(frame);
        free(frame);
        return false;
      }
      construct_msg_from_frames(reader);
    }
  } else {
    if (!simple_queue_push(reader->frame_queue, frame)) {
      ws_frame_free(frame);
      free(frame);
      return false;
    }
  }
  return true;
}
struct ws_message_t* ws_reader_next_msg(struct ws_reader_t *reader) {
  struct ws_message_t *result = NULL;
  if (!simple_queue_pop(reader->msg_queue, (void**)&result)) {
    return NULL;
  }
  return result;
}

void ws_reader_destroy(struct ws_reader_t **reader) {
  simple_queue_destroy((*reader)->frame_queue);
  simple_queue_destroy((*reader)->msg_queue);
  free(*reader);
  *reader = NULL;
}

bool ws_message_init(struct ws_message_t *msg) {
  msg->type = OPCODE_CONT;
  return byte_array_init(&msg->body, 1);
};
void ws_message_free(struct ws_message_t *msg) {
  byte_array_free(&msg->body);
}

