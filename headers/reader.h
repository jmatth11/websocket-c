#ifndef CSTD_WEBSOCKET_READER_H
#define CSTD_WEBSOCKET_READER_H

#include "defs.h"
#include "protocol.h"
#include "unicode_str.h"

#include <stdbool.h>

__BEGIN_DECLS

// forward declare
struct net_info_t;

/**
 * WebSocket Reader structure.
 */
struct ws_reader_t;

/**
 * Message Structure for a constructure WebSocket message..
 * This structure represents the final constructed message from WebSocket frames.
 */
struct ws_message_t {
  /**
   * The WebSocket OPCODE type.
   */
  enum ws_opcode_t type;
  /**
   * The full body of the WebSocket message.
   */
  byte_array body;
};

/**
 * Create a WebSocket reader.
 *
 * @return A newly allocated WebSocket reader.
 */
struct ws_reader_t* ws_reader_create();

/**
 * Handle reading from the WebSocket reader to construct messages.
 * This function blocks while waiting for data from the server and handles fragmented frames.
 * Use ws_reader_next_msg to get the messages generated from this call.
 *
 * @param[in] reader The WebSocket reader.
 * @param[in] socket The socket to listen on.
 * @return True on success, false otherwise.
 */
bool ws_reader_handle(struct ws_reader_t *reader, struct net_info_t *info) __nonnull((1));

/**
 * Get the next generated message from the WebSocket reader.
 * The caller is responsible for freeing the returned message.
 *
 * @param[in] reader The WebSocket reader.
 * @return The next generated message, NULL if message queue is empty.
 */
struct ws_message_t* ws_reader_next_msg(struct ws_reader_t *reader) __nonnull((1));

/**
 * Destroy the WebSocket reader and it's internal data.
 * The reader is automatically NULL'ed out.
 *
 * @param[out] reader The WebSocket reader.
 */
void ws_reader_destroy(struct ws_reader_t **reader) __nonnull((1));

/**
 * Initialize a given WebSocket message structure.
 *
 * @param[in] msg The WebSocket Message.
 * @return True on success, false otherwise.
 */
bool ws_message_init(struct ws_message_t *msg) __nonnull((1));

/**
 * Free the given WebSocket message.
 *
 * @param[in] msg The WebSocket Message.
 */
void ws_message_free(struct ws_message_t *msg);

__END_DECLS

#endif
