#include "websocket.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// network related
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers
// ^ quick overview of spec to implement

static inline void print_debug(struct ws_client_t *client);


#ifdef DEBUG
#define debug_client(client) print_debug(client)
#else
#define debug_client(client)
#endif

#define WS_INIT_HANDSHAKE                                                      \
  "GET %s HTTP/1.1\r\nHost: %s:%d\r\nUpgrade: websocket\r\n"                   \
  "Connection: Upgrade\r\nSec-WebSocket-Key: %s\r\n"                           \
  "Sec-WebSocket-Version: %d\r\n\r\n"

#define WS_PREFIX "ws://"
#define PORT_SEP ':'
#define PATH_SEP '/'
static char *empty_path = "/";

struct __ws_client_internal_t {
  int socket;
  struct sockaddr_in addr;
};

static inline void print_debug(struct ws_client_t *client) {
  if (client == NULL) return;
  char *path = empty_path;
  if (client->path != NULL) {
    path = client->path;
  }
  printf("client->host: %s:%d%s - %d\nversion: %d\n", client->host, client->port, path, client->__internal->addr.sin_addr.s_addr, client->version);
}

// TODO pull this out later has proper HTTP request construction
static char *initial_handshake(struct ws_client_t *client) {
  if (client == NULL || client->host == NULL) {
    return NULL;
  }
  char *path = empty_path;
  if (client->path != NULL) {
    path = client->path;
  }
  // use NULL destination to get formatted string length
  size_t req_len = snprintf(
      NULL, 0, WS_INIT_HANDSHAKE, path, client->host, client->port,
      "7Wrl5Wp3kkEaYOVhio4o6w==", // TODO change this to dynamic noonce
      client->version);
  char *req = malloc((sizeof(char) * req_len) + 1);
  req_len = snprintf(req, req_len + 1, WS_INIT_HANDSHAKE, path, client->host, client->port,
      "7Wrl5Wp3kkEaYOVhio4o6w==", // TODO change this to dynamic noonce
      client->version);
  if (req_len == 0) {
    free(req);
    return NULL;
  }
  req[req_len + 1] = '\0';
  return req;
}

/**
 * Create WebSocket client from the given URL.
 *
 * @param url The URL to parse.
 * @param len The length of the URL string.
 * @param client The WebSocket Client to populate.
 * @return True if successful, False otherwise.
 */
bool ws_client_from_str(const char *url, size_t len,
                        struct ws_client_t *client) {
  client->host = NULL;
  client->path = NULL;
  client->port = 80;
  client->version = 13;
  const size_t prefix_len = strlen(WS_PREFIX);
  if (len <= prefix_len) {
    fprintf(stderr, "URL len was the same size or shorter than the expected "
                    "WebSocket prefix length.\n");
    return false;
  }
  size_t offset = 0;
  if (strncmp(WS_PREFIX, url, prefix_len) != 0) {
    fprintf(stderr, "URL does not start with a valid WebSocket prefix.\n");
    return false;
  }
  offset += prefix_len;
  size_t next_offset = strcspn(&url[offset], ":/");
  const size_t host_len = next_offset;
  client->host = malloc((sizeof(char) * host_len) + 1);
  if (strncpy(client->host, &url[offset], host_len) == NULL) {
    fprintf(stderr, "failed to copy host from URL.\n");
    return false;
  }
  client->host[host_len + 1] = '\0';
  if ((next_offset + prefix_len) >= len) {
    return true;
  }
  offset += next_offset;
  if (url[offset] == PORT_SEP) {
    ++offset;
    char port_buffer[6] = {0};
    next_offset = strcspn(&url[offset], "/");
    if (strncpy(port_buffer, &url[offset], next_offset) == NULL) {
      fprintf(stderr, "failed to copy port from URL.\n");
      return false;
    }
    port_buffer[next_offset] = '\0';
    client->port = atoi(port_buffer);
    offset += next_offset;
  }
  if (offset != len && url[offset] == PATH_SEP) {
    const size_t path_len = (len - offset);
    client->path = malloc((sizeof(char) * path_len) + 1);
    if (strncpy(client->path, &url[offset], path_len) == NULL) {
      fprintf(stderr, "failed to copy path from URL.\n");
      return false;
    }
    client->path[path_len + 1] = '\0';
  }
  debug_client(client);
  return true;
}

/**
 * Establish a connection with the given WebSocket.
 *
 * @param client The WebSocket Client.
 * @return True if successful, False otherwise.
 */
bool ws_client_connect(struct ws_client_t *client) {
  if (client->host == NULL) {
    fprintf(stderr, "WebSocket client's host was null.\n");
    return false;
  }
  client->__internal = malloc(sizeof(struct __ws_client_internal_t));
  client->__internal->addr.sin_family = AF_INET;
  client->__internal->addr.sin_port = htons(client->port);
  if (inet_pton(AF_INET, client->host, &client->__internal->addr.sin_addr) != 1) {
    fprintf(stderr, "WebSocket Client host could not convert IP to addr.\n");
    free(client->__internal);
    return false;
  }
  debug_client(client);
  int sock =
      socket(client->__internal->addr.sin_family, SOCK_STREAM, IPPROTO_TCP);
  if (connect(sock, (struct sockaddr *)&client->__internal->addr,
              sizeof(client->__internal->addr)) != 0) {
    fprintf(stderr, "WebSocket client failed to connect.\n");
    free(client->__internal);
    return false;
  }
  client->__internal->socket = sock;
  char *req = initial_handshake(client);
  if (req == NULL) {
    fprintf(stderr, "WebSocket client failed to create handshake.\n");
    close(client->__internal->socket);
    free(client->__internal);
    return false;
  }
#ifdef DEBUG
  printf("sending req: \"%s\"\n", req);
#endif
  ssize_t n = send(sock, req, strlen(req), 0);
  if (n == -1) {
    fprintf(stderr, "message wasn't sent\n");
    free(req);
    close(client->__internal->socket);
    free(client->__internal);
    return false;
  }
  free(req);
  char *response = NULL;
  if (!ws_client_recv(client, &response)) {
    fprintf(stderr, "WebSocket client failed to connect.\n");
    close(client->__internal->socket);
    free(client->__internal);
    return false;
  }
#ifdef DEBUG
  printf("response: %s\n", response);
#endif
  // TODO maybe parse entire response for proper handling
  // TODO support dynamic generation of Accept value
  // TODO replace strcasestr with more portable function
  if (strcasestr(response, "Sec-Websocket-Accept: mj/2Q6QlJ3Y5pun3vzHGmTO/xgs=") == NULL) {
    fprintf(stderr, "WebSocket Client connection was rejected.\n%s\n", response);
    free(response);
    close(client->__internal->socket);
    free(client->__internal);
    return false;
  }
  free(response);
  return true;
}

/**
 * Recieve data from the WebSocket Client and store the results in the out
 * parameter. This operation blocks.
 *
 * @param client The WebSocket Client.
 * @param[out] out The recieved data.
 * @return True if successful, False otherwise.
 */
bool ws_client_recv(struct ws_client_t *client, char **out) {
  char buffer[BUFSIZ] = {0};
  const ssize_t n = recv(client->__internal->socket, buffer, BUFSIZ, 0);
  if (n == -1) {
    fprintf(stderr, "WebSocket client recieve failure.\n");
    return false;
  }
  *out = malloc((sizeof(char)*n)+1);
  if (*out == NULL) {
    fprintf(stderr, "WebSocket client failed to malloc response.\n");
    return false;
  }
  if (strncpy(*out, buffer, n) == NULL) {
    free(*out);
    *out = NULL;
    fprintf(stderr, "WebSocket client failed to copy recv buffer.\n");
    return false;
  }
  (*out)[n + 1] = '\0';
  return true;
}

/**
 * Free the internal WebSocket client data.
 *
 * @param client The WebSocket Client.
 */
void ws_client_free(struct ws_client_t *client) {
  if (client == NULL) return;
  if (client->host != NULL) {
    free(client->host);
  }
  if (client->path != NULL) {
    free(client->path);
  }
  if (client->__internal != NULL) {
    close(client->__internal->socket);
    free(client->__internal);
  }
}
