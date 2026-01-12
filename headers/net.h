#ifndef CSTD_WEBSOCKET_NET_H
#define CSTD_WEBSOCKET_NET_H

/**
 * Wrapper close for network communication.
 * This will handle regular and OpenSSL operations.
 */

#include "defs.h"

#include <netinet/in.h>
#include <stdbool.h>
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

bool ws_init();
bool ws_connect(struct ws_client_t *client, struct net_info_t *out) __nonnull((2));
bool ws_accept(struct ws_client_t *client);
bool ws_read(struct ws_client_t *client);
bool ws_write(struct ws_client_t *client);
void ws_close(struct net_info_t *info);
void ws_deinit();

#endif
