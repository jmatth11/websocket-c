#include "headers/net.h"
#include "headers/websocket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/types.h>
#include <string.h>
#include <sys/socket.h>

static SSL_CTX *ctx = NULL;

bool ws_init() {
  if (ctx == NULL) {
    SSL_library_init();
    ctx = SSL_CTX_new(TLS_client_method());
  }
  return true;
}
bool ws_connect(struct ws_client_t *client, struct net_info_t *out) {
  struct net_info_t result = {0};
  result.addr.sin_family = AF_INET;
  result.addr.sin_port = htons(client->port);
  // TODO use getaddrinfo from <netdb.h> to get IP address from hostname
  // getaddrinfo(<host>, <port>, <hints (i.e. IPv4 only or IPv4 and 6), <results>)
  if (inet_pton(AF_INET, client->host, &result.addr.sin_addr) !=
      1) {
    fprintf(stderr, "WebSocket Client host could not convert IP to addr.\n");
    return false;
  }
  int sock =
      socket(result.addr.sin_family, SOCK_STREAM, IPPROTO_TCP);
  if (connect(sock, (struct sockaddr *)&result.addr,
              sizeof(result.addr)) != 0) {
    fprintf(stderr, "WebSocket client failed to connect.\n");
    return false;
  }
#ifdef WEBC_USE_SSL
  result.ssl = SSL_new(ctx);
  if (result.ssl == NULL) {
    fprintf(stderr, "SSL could not be instantiated for client.\n");
    return false;
  }
  // TODO finish connection with SSL
#endif
  *out = result;
  return true;
}
bool ws_accept(struct ws_client_t *client) {

  return true;
}
bool ws_read(struct ws_client_t *client) {

  return true;
}
bool ws_write(struct ws_client_t *client) {

  return true;
}
void ws_close(struct ws_client_t *client) {

}
void ws_deinit() {
  if (ctx != NULL) {
    SSL_CTX_free(ctx);
    ctx = NULL;
  }
}
