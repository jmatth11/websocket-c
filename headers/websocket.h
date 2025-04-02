#ifndef CSTD_WEBSOCKET_CLIENT_H
#define CSTD_WEBSOCKET_CLIENT_H

#include <stdbool.h>
#include <stdlib.h>

#include "headers/protocol.h"
#include "headers/reader.h"
#include "unicode_str.h"

struct __ws_client_internal_t;
struct ws_client_t {
  struct __ws_client_internal_t *__internal;
  /**
   * The path of the URL.
   */
  char *path;
  /**
   * The Host portion of the URL.
   */
  char *host;
  /**
   * The port.
   */
  unsigned int port;
  /**
   * The web socket version to use.
   * Default is 13.
   */
  unsigned short version;
};

typedef bool(on_message_callback)(struct ws_client_t *client, struct ws_message_t *msg);

/**
 * Create WebSocket client from the given URL.
 *
 * @param url The URL to parse.
 * @param len The length of the URL string.
 * @param client The WebSocket Client to populate.
 * @return True if successful, False otherwise.
 */
bool ws_client_from_str(const char *url, size_t len, struct ws_client_t *client);
/**
 * Establish a connection with the given WebSocket.
 *
 * @param client The WebSocket Client.
 * @return True if successful, False otherwise.
 */
bool ws_client_connect(struct ws_client_t *client);
/**
 * Recieve data from the WebSocket Client and store the results in the out parameter.
 * This operation blocks.
 * The caller is responsible for freeing the out variable.
 *
 * @param client The WebSocket Client.
 * @param[out] out The recieved data.
 * @return True if successful, False otherwise.
 */
bool ws_client_recv(struct ws_client_t *client, byte_array *out);

bool ws_client_next_msg(struct ws_client_t *client, struct ws_message_t **out) __nonnull((1));

bool ws_client_on_msg(struct ws_client_t *client, on_message_callback cb) __nonnull((1));
/**
 * Free the internal WebSocket client data.
 *
 * @param client The WebSocket Client.
 */
void ws_client_free(struct ws_client_t *client);

#endif
