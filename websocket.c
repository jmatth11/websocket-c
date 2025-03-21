#include "websocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers
// ^ quick overview of spec to implement

#define WS_PREFIX "ws://"
#define PORT_SEP ':'
#define PATH_SEP '/'


struct __ws_client_internal_t {
  int socket;
};

/**
 * Create WebSocket client from the given URL.
 *
 * @param url The URL to parse.
 * @param len The length of the URL string.
 * @param client The WebSocket Client to populate.
 * @return True if successful, False otherwise.
 */
bool ws_client_from_str(const char *url, size_t len, struct ws_client_t *client) {
  client->version = 13;
  const size_t prefix_len = strlen(WS_PREFIX);
  if (len <= prefix_len) {
    fprintf(stderr, "URL len was the same size or shorter than the expected WebSocket prefix length.\n");
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
  client->host = malloc((sizeof(char)*host_len) + 1);
  if (strncpy(client->host, &url[offset], host_len) == NULL) {
    fprintf(stderr, "failed to copy host from URL.\n");
    return false;
  }
  client->host[host_len + 1] = '\0';
  if ((next_offset + prefix_len) >= len) {
    client->path = NULL;
    client->port = 80;
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
    client->path = malloc((sizeof(char)*path_len)+1);
    if (strncpy(client->path, &url[offset], path_len) == NULL) {
      fprintf(stderr, "failed to copy path from URL.\n");
      return false;
    }
    client->path[path_len + 1] = '\0';
  }
  return true;
}

/**
 * Establish a connection with the given WebSocket.
 *
 * @param client The WebSocket Client.
 * @return True if successful, False otherwise.
 */
bool ws_client_connect(struct ws_client_t *client) {

  return true;
}

/**
 * Recieve data from the WebSocket Client and store the results in the out parameter.
 * This operation blocks.
 *
 * @param client The WebSocket Client.
 * @param[out] out The recieved data.
 * @return True if successful, False otherwise.
 */
bool ws_client_recv(struct ws_client_t *client, char **out) {

  return true;
}

/**
 * Free the internal WebSocket client data.
 *
 * @param client The WebSocket Client.
 */
void ws_client_free(struct ws_client_t *client) {

}
