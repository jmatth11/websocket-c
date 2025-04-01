#include "headers/reader.h"
#include "queue.h"

struct ws_reader_t {
  struct simple_queue_t *frame_queue;
  struct simple_queue_t *msg_queue;
};

struct ws_reader_t* ws_reader_create() {
  struct ws_reader_t *result = malloc(sizeof(struct ws_reader_t));
  result->frame_queue = simple_queue_create();
  result->msg_queue = simple_queue_create();
  return result;
}
bool ws_reader_handle(struct ws_reader_t *reader, int socket) {
  // TODO implement
  return true;
}
struct ws_message_t* ws_reader_next_msg(struct ws_reader_t *reader) {
  // TODO implement
  return NULL;
}

void ws_reader_destroy(struct ws_reader_t *reader) {
  simple_queue_destroy(reader->frame_queue);
  simple_queue_destroy(reader->msg_queue);
  free(reader);
}

bool ws_message_init(struct ws_message_t *msg) __nonnull((1));
void ws_message_free(struct ws_message_t *msg) __nonnull((1));

