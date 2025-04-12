#ifndef CSTD_WEBSOCKET_CLIENT_H
#define CSTD_WEBSOCKET_CLIENT_H

#include <stdbool.h>
#include <stdlib.h>

#include "headers/protocol.h"
#include "headers/reader.h"
#include "unicode_str.h"

/**
 * Internal WebSocket client data.
 */
struct __ws_client_internal_t;

/**
 * WebSocket Client structure.
 */
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

/**
 * Callback definition for a client's on_msg listener.
 *
 * @param[in] client The WebSocket client.
 * @param[in] msg The WebSocket message from the server.
 * @return True on success, false otherwise. False value also stops
 *  the internal listener loop.
 */
typedef bool(on_message_callback)(struct ws_client_t *client,
                                  struct ws_message_t *msg);

/**
 * Create WebSocket client from the given URL.
 *
 * @param url The URL to parse.
 * @param len The length of the URL string.
 * @param client The WebSocket Client to populate.
 * @return True if successful, False otherwise.
 */
bool ws_client_from_str(const char *url, size_t len,
                        struct ws_client_t *client);
/**
 * Establish a connection with the given WebSocket.
 *
 * @param client The WebSocket Client.
 * @return True if successful, False otherwise.
 */
bool ws_client_connect(struct ws_client_t *client);

/**
 * Listen for the next WebSocket message for the client and populate the out
 * message param. By default, this function blocks while waiting for a message.
 *
 * @param[in] client The WebSocket client.
 * @param[out] out The WebSocket message.
 * @return True on success, False otherwise.
 */
bool ws_client_next_msg(struct ws_client_t *client, struct ws_message_t **out)
    __nonnull((1));

/**
 * Set a callback to be a listener for the client's WebSocket messages.
 * By default, this function blocks until the internal loop exits.
 * This function handles responding to PING and CLOSE messages.
 *
 * Return false within the callback to exit the internal loop.
 *
 * @param[in] client The WebSocket client.
 * @param[in] cb The callback function.
 * @return True on successful exit, False otherwise.
 */
bool ws_client_on_msg(struct ws_client_t *client, on_message_callback cb)
    __nonnull((1));

/**
 * Write a message out to the server.
 *
 * @param[in] client The WebSocket client.
 * @param[in] type The OPCODE type of the message.
 * @param[in] body The body of the message.
 * @return True on success, False otherwise.
 */
bool ws_client_write(struct ws_client_t *client, enum ws_opcode_t type,
                     byte_array body) __nonnull((1));

/**
 * Write a message out to the server.
 *
 * @param[in] client The WebSocket client.
 * @param[in] type The OPCODE type of the message.
 * @param[in] body The body of the message.
 * @return True on success, False otherwise.
 */
bool ws_client_write_msg(struct ws_client_t *client, struct ws_message_t *msg)
    __nonnull((1, 2));

/**
 * Free the internal WebSocket client data.
 *
 * @param client The WebSocket Client.
 */
void ws_client_free(struct ws_client_t *client);

#endif
