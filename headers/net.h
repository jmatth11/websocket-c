#ifndef CSTD_WEBSOCKET_NET_H
#define CSTD_WEBSOCKET_NET_H

/**
 * Wrapper functionality for network communication.
 * This will handle regular and OpenSSL operations.
 *
 * If using OpenSSL ws_init/ws_deinit must be used appropriately.
 */

#include "defs.h"

#include <netinet/in.h>
#include <stdbool.h>
#include <sys/types.h>
#ifdef WEBC_USE_SSL
#include <openssl/types.h>
#endif

struct ws_client_t;

struct net_info_t {
  int socket;
#ifdef WEBC_USE_SSL
  SSL *ssl;
#endif
};

#ifdef WEBC_USE_SSL

/**
 * Initialize SSL library functionality.
 * Provide optional cert/path for trusted verification.
 *
 * @param cert The cert file name.
 * @param path The path to the cert file. Can be NULL if relative.
 * @return True on success, false otherwise.
 */
bool ws_init(const char * restrict cert, const char * restrict path);

/**
 * Deinitalize the SSL library functionality.
 */
void ws_deinit();

#endif

/**
 * Connect to a TCP socket and populate the given connection info in the
 * net_info_t structure.
 *
 * @param client The websocket client.
 * @param out The net_info_t structure to populate.
 * @return True on success, false otherwise.
 */
bool ws_connect(struct ws_client_t *client, struct net_info_t *out)
    __nonnull((2));

/**
 * Accept an incoming connection.
 */
bool ws_accept(struct ws_client_t *client, struct net_info_t *info);

/**
 * Peek the next buf_len bytes from the connection.
 *
 * @param client The WebSocket client.
 * @param info The net info structure.
 * @param buf The buffer to populate.
 * @param buf_len The length of the given buffer.
 * @return The number of bytes peeked, -1 on failure.
 */
ssize_t ws_peek(struct ws_client_t *client, struct net_info_t *info, void *buf,
                size_t buf_len);

/**
 * Read the next buf_len bytes from the connection.
 *
 * @param client The WebSocket client.
 * @param info The net info structure.
 * @param buf The buffer to populate.
 * @param buf_len The length of the given buffer.
 * @return The number of bytes read, -1 on failure.
 */
ssize_t ws_read(struct ws_client_t *client, struct net_info_t *info, void *buf,
                size_t buf_len);

/**
 * write the given buffer to the connection.
 *
 * @param client The WebSocket client.
 * @param info The net info structure.
 * @param buf The buffer to populate.
 * @param buf_len The length of the given buffer.
 * @return The number of bytes written, -1 on failure.
 */
ssize_t ws_write(struct ws_client_t *client, struct net_info_t *info,
                 const void *buf, size_t buf_len);

/**
 * Close the connection.
 *
 * @param info The net info structure.
 */
void ws_close(struct net_info_t *info);

#endif
