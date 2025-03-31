#ifndef CSTD_WEBSOCKET_READER_H
#define CSTD_WEBSOCKET_READER_H

#include "defs.h"
#include <stdbool.h>
struct ws_reader_queue_t;

struct ws_reader_t {
  struct ws_reader_queue_t *queue;
};

bool ws_reader_init(struct ws_reader_t *reader) __nonnull((1));
void ws_reader_free(struct ws_reader_t *reader) __nonnull((1));

#endif
