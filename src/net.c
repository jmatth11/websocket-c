#include "headers/net.h"
#include "headers/websocket.h"
#include "magic.h"
#include "string_ops.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef BUFSIZ
  #define BUFSIZ 4096
#endif

#ifdef WEBC_USE_SSL
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/types.h>

static SSL_CTX *ctx = NULL;
/**
 * Init
 */
bool ws_init(const char * restrict cert, const char * restrict path) {
  if (ctx == NULL) {
    SSL_library_init();
    ctx = SSL_CTX_new(TLS_client_method());
    if (cert != NULL) {
      if (!SSL_CTX_load_verify_locations(ctx, cert, path)) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        ctx = NULL;
        return false;
      }
    }
  }
  return true;
}
/**
 * Deinit
 */
void ws_deinit() {
  if (ctx != NULL) {
    SSL_CTX_free(ctx);
    ctx = NULL;
  }
}
#endif

/**
 * Structure to track result for function cleanup.
 */
struct net_result_t {
  bool error_triggered;
  void *ctx;
};

/**
 * Generate default net result.
 */
struct net_result_t gen_net_result() {
  return (struct net_result_t){
      .error_triggered = false,
      .ctx = NULL,
  };
}

/**
 * cleanup function for connect
 */
static void ws_connect_cleanup(struct net_result_t *result) {
  if (!result->error_triggered) {
    return;
  }
  if (result->ctx != NULL) {
    struct net_info_t *info = (struct net_info_t *)result->ctx;
#ifdef WEBC_USE_SSL
    if (info->ssl != NULL) {
      // print ssl errors and shutdown and free.
      ERR_print_errors_fp(stderr);
      SSL_shutdown(info->ssl);
      SSL_free(info->ssl);
    }
#endif
    close(info->socket);
  }
}

/**
 * Connect
 */
bool ws_connect(struct ws_client_t *client, struct net_info_t *out) {
  if (client == NULL) {
    return false;
  }
  DEFER(ws_connect_cleanup) struct net_result_t context = gen_net_result();
  struct net_info_t result = {0};
  result.socket = -1;
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  char AUTO_C *port = to_str(client->port);
  if (!getaddrinfo(client->host, port, &hints, &res)) {
    fprintf(stderr, "failed to get address info.\n");
    return false;
  }
  int sock = -1;
  for (struct addrinfo *rp = res; rp != NULL; rp = rp->ai_next) {
    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sock == -1) {
      continue;
    }
    if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
      /* success */
      result.socket = sock;
      break;
    }
    close(sock);
    sock = -1;
  }
  if (sock == -1) {
    fprintf(stderr, "no valid socket connection found.\n");
    context.error_triggered = true;
    return false;
  }
  // set the context to the result for if an error triggers.
  // setting it here lets us know we entered a successful connection if an error
  // occurred.
  context.ctx = &result;
#ifdef WEBC_USE_SSL
  result.ssl = SSL_new(ctx);
  if (result.ssl == NULL) {
    fprintf(stderr, "SSL could not be instantiated for client.\n");
    context.error_triggered = true;
    return false;
  }
  (void)SSL_set_fd(result.ssl, result.socket);
  // optional feature so we don't flag as an error.
  if (!SSL_set_tlsext_host_name(result.ssl, client->host)) {
    fprintf(stderr, "SSL set host name failed.\n");
  }
  if (!SSL_connect(result.ssl)) {
    fprintf(stderr, "SSL connect failed.\n");
    context.error_triggered = true;
    return false;
  }
#endif
  *out = result;
  return true;
}

/**
 * Accept
 * TODO finish implementation.
 */
bool ws_accept(struct ws_client_t *client, struct net_info_t *info) { return true; }

ssize_t ws_peek(struct ws_client_t *client, struct net_info_t *info, void *buf, size_t buf_len) {
  if (client == NULL || info == NULL || buf == NULL) {
    return false;
  }
#ifdef WEBC_USE_SSL
  if (client->use_tls) {
    const ssize_t n = SSL_peek(info->ssl, buf, buf_len);
    if (n < 0) {
      ERR_print_errors_fp(stderr);
    }
    return n;
  }
#endif
  const ssize_t n = recv(info->socket, buf, buf_len, MSG_PEEK);
  if (n < 0) {
    fprintf(stderr, "WebSocket recv failed.\n");
  }
  return n;
}

ssize_t ws_read(struct ws_client_t *client, struct net_info_t *info, void *buf, size_t buf_len) {
  if (client == NULL || info == NULL || buf == NULL) {
    return false;
  }
#ifdef WEBC_USE_SSL
  if (client->use_tls) {
    const ssize_t n = SSL_read(info->ssl, buf, buf_len);
    if (n < 0) {
      ERR_print_errors_fp(stderr);
    }
    return n;
  }
#endif
  return recv(info->socket, buf, buf_len, 0);
}

/**
 * Write
 */
ssize_t ws_write(struct ws_client_t *client, struct net_info_t *info, const void *buf, size_t buf_len) {
  if (client == NULL || info == NULL || buf == NULL) {
    return false;
  }
#ifdef WEBC_USE_SSL
  if (client->use_tls) {
    const ssize_t n = SSL_write(info->ssl, buf, buf_len);
    if (n < 0) {
      ERR_print_errors_fp(stderr);
    }
    return n;
  }
#endif
  const ssize_t n = send(info->socket, buf, buf_len, 0);
  if (n < 0) {
    fprintf(stderr, "WebSocket client send failure.\n");
  }
  return n;
}

/**
 * Close
 */
void ws_close(struct net_info_t *info) {
  if (info == NULL) return;
#ifdef WEBC_USE_SSL
  if (info->ssl != NULL) {
    SSL_shutdown(info->ssl);
    SSL_free(info->ssl);
    info->ssl = NULL;
  }
#endif
  close(info->socket);
}

