#ifndef CSTD_WEBSOCKET_READER_H
#define CSTD_WEBSOCKET_READER_H

#include "defs.h"
#include "queue.h"
#include "protocol.h"
#include "unicode_str.h"

#include <stdbool.h>

struct ws_reader_t;

struct ws_message_t {
  enum ws_opcode_t type;
  byte_array body;
};

struct ws_reader_t* ws_reader_create();
bool ws_reader_handle(struct ws_reader_t *reader, int socket) __nonnull((1));
struct ws_message_t* ws_reader_next_msg(struct ws_reader_t *reader) __nonnull((1));
void ws_reader_destroy(struct ws_reader_t *reader) __nonnull((1));

bool ws_message_init(struct ws_message_t *msg) __nonnull((1));
void ws_message_free(struct ws_message_t *msg) __nonnull((1));

#endif
