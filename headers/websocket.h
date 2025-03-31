#ifndef CSTD_WEBSOCKET_CLIENT_H
#define CSTD_WEBSOCKET_CLIENT_H

#include <stdbool.h>
#include <stdlib.h>

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

struct ws_message_t {
  // TODO replace with proper protocol type
  int type;
  byte_array body;
};

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
/**
 * Free the internal WebSocket client data.
 *
 * @param client The WebSocket Client.
 */
void ws_client_free(struct ws_client_t *client);

#endif
