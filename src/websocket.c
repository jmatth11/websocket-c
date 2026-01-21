#include "headers/websocket.h"
#include "headers/http.h"
#include "headers/protocol.h"
#include "headers/reader.h"
#include "headers/net.h"
#include "unicode_str.h"
#include "magic.h"
#include "string_ops.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// network related
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

// https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers
// ^ quick overview of spec to implement

#define REQUEST_NOONCE "7Wrl5Wp3kkEaYOVhio4o6w=="
#define RESPONSE_NOONCE "mj/2Q6QlJ3Y5pun3vzHGmTO/xgs="

#define WS_PREFIX "ws://"
#define WSS_PREFIX "wss://"
#define PORT_SEP ':'
#define PATH_SEP '/'
#define PROTOCOL "HTTP/1.1"
static char *empty_path = "/";

struct __ws_client_internal_t {
  struct ws_reader_t *reader;
  struct net_info_t info;
};

#ifdef DEBUG
static inline void print_debug(struct ws_client_t *client) {
  if (client == NULL)
    return;
  char *path = empty_path;
  if (client->path != NULL) {
    path = client->path;
  }
  if (client->host == NULL) {
    fprintf(stderr, "host is null\n");
    return;
  }
  printf("client->host: %s:%d%s\nversion: %d\n", client->host,
         client->port, path, client->version);
}

#define debug_client(client) print_debug(client)
#else
#define debug_client(client)
#endif

static bool ws_check_internals(struct ws_client_t *client) {
  return client->__internal != NULL && client->__internal->reader != NULL;
}

/**
 * Recieve data from the WebSocket Client and store the results in the out
 * parameter. This operation blocks. The caller is responsible for freeing the
 * out variable.
 *
 * @param client The WebSocket Client.
 * @param[out] out The recieved data.
 * @return True if successful, False otherwise.
 */
static bool ws_client_recv(struct ws_client_t *client, byte_array *out);

static bool ws_client_write_blob(struct ws_client_t *client, byte_array *msg) {
  ssize_t n = net_write(&client->__internal->info, msg->byte_data, msg->len);
  if (n == -1) {
    fprintf(stderr, "WebSocket client send failure.\n");
    return false;
  }
  return true;
}

static bool set_handshake_headers(struct http_request_t *req,
                                  struct ws_client_t *client) {
  if (!http_request_set_header(req, "Upgrade", "websocket")) {
    fprintf(stderr, "failed to set request header.\n");
    return false;
  }
  if (!http_request_set_header(req, "Connection", "Upgrade")) {
    fprintf(stderr, "failed to set request header.\n");
    return false;
  }
  if (!http_request_set_header(req, "sec-websocket-key", REQUEST_NOONCE)) {
    fprintf(stderr, "failed to set request header.\n");
    return false;
  }
  char AUTO_C *version = to_str(client->version);
  if (version == NULL) {
    fprintf(stderr, "failed to convert 'version' to a string.\n");
    return false;
  }
  if (!http_request_set_header(req, "Sec-WebSocket-Version", version)) {
    fprintf(stderr, "failed to set request header.\n");
    return false;
  }
  return true;
}

static char *initial_handshake(struct ws_client_t *client) {
  if (client == NULL || client->host == NULL) {
    return NULL;
  }
  char *path = empty_path;
  if (client->path != NULL) {
    path = client->path;
  }
  struct http_request_t DEFER(http_request_free) req;
  if (!http_request_init(&req)) {
    fprintf(stderr, "failed to initialize HTTP request structure.\n");
    return NULL;
  }
  req.message.path = path;
  req.message.protocol = PROTOCOL;
  req.message.host = client->host;
  req.message.port = client->port;
  if (!set_handshake_headers(&req, client)) {
    fprintf(stderr, "failed to set WebSocket handshake headers.\n");
    req.message.protocol = NULL;
    req.message.path = NULL;
    req.message.host = NULL;
    return NULL;
  }
  char *result = http_request_to_str(&req);
  req.message.protocol = NULL;
  req.message.path = NULL;
  req.message.host = NULL;
  return result;
}

bool ws_client_init(struct ws_client_t* client) {
  client->host = NULL;
  client->path = NULL;
  client->port = 80;
  client->version = 13;
  client->__internal = NULL;
#ifdef WEBC_USE_SSL
  client->use_tls = false;
#endif
  return true;
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
#ifdef WEBC_USE_SSL
  client->use_tls = false;
#endif
  const size_t ws_prefix_len = strlen(WS_PREFIX);
  if (len <= ws_prefix_len) {
    fprintf(stderr, "URL len was the same size or shorter than the expected "
                    "WebSocket prefix length.\n");
    return false;
  }
  const size_t wss_prefix_len = strlen(WSS_PREFIX);
  if (len <= wss_prefix_len) {
    fprintf(stderr, "URL len was the same size or shorter than the expected "
                    "WebSocket Secure prefix length.\n");
    return false;
  }
  size_t offset = ws_prefix_len;
  if (strncmp(WS_PREFIX, url, ws_prefix_len) != 0) {
    // check for wss
    if (strncmp(WSS_PREFIX, url, wss_prefix_len) == 0) {
#ifdef WEBC_USE_SSL
      client->use_tls = true;
      offset = wss_prefix_len;
#else
      fprintf(stderr, "Library was not built with SSL support.\nBuild with \"make USE_SSL=1\" or \"zig build -Duse_ssl\".\n");
      return false;
#endif
    } else {
      fprintf(stderr, "URL does not start with a valid WebSocket prefix.\n");
      return false;
    }
  }
  size_t next_offset = strcspn(&url[offset], ":/");
  const size_t host_len = next_offset;
  client->host = malloc((sizeof(char) * host_len) + 1);
  if (strncpy(client->host, &url[offset], host_len) == NULL) {
    fprintf(stderr, "failed to copy host from URL.\n");
    return false;
  }
  client->host[host_len] = '\0';
  if ((next_offset + ws_prefix_len) >= len) {
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
    client->path[path_len] = '\0';
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
  if (client->host == NULL) {
    fprintf(stderr, "WebSocket client's host was null.\n");
    return false;
  }
  struct net_info_t result;
  memset(&result, 0, sizeof(result));
  char AUTO_C *port_str = to_str(client->port);
  if (!net_connect(client->host, port_str, &result)) {
    fprintf(stderr, "WebSocket client could not connect.");
    return false;
  }
  client->__internal = malloc(sizeof(struct __ws_client_internal_t));
  client->__internal->info = result;
  client->__internal->reader = ws_reader_create();
  char AUTO_C *req = initial_handshake(client);
  if (req == NULL) {
    fprintf(stderr, "WebSocket client failed to create handshake.\n");
    net_close(&client->__internal->info);
    free(client->__internal);
    return false;
  }
#ifdef DEBUG
  printf("sending req: \"%s\"\n", req);
#endif
  ssize_t n = net_write(&client->__internal->info, req, strlen(req));
  if (n == -1) {
    fprintf(stderr, "message wasn't sent\n");
    net_close(&client->__internal->info);
    free(client->__internal);
    return false;
  }
  byte_array DEFER(byte_array_free) response;
  if (!ws_client_recv(client, &response)) {
    fprintf(stderr, "WebSocket client failed to connect.\n");
    net_close(&client->__internal->info);
    free(client->__internal);
    return false;
  }
#ifdef DEBUG
  printf("resp.len: %lu -- response: %s\n", response.len, response.byte_data);
#endif
  struct http_response_t DEFER(http_response_free) resp;
  if (!http_response_init(&resp)) {
    fprintf(stderr, "failed to initialize HTTP response structure.\n");
    net_close(&client->__internal->info);
    free(client->__internal);
    return false;
  }
  char AUTO_C *resp_cstr = malloc((sizeof(char) * response.len) + 1);
  memcpy(resp_cstr, response.byte_data, response.len);
  resp_cstr[response.len] = '\0';
  if (!http_response_from_str(&resp, resp_cstr, response.len)) {
    fprintf(stderr, "failed to parse HTTP response message.\n");
    net_close(&client->__internal->info);
    free(client->__internal);
    return false;
  }
  // TODO support dynamic generation of Accept value
  char *noonce = NULL;
  if (!http_response_get_header(&resp, "sec-websocket-accept", &noonce)) {
    fprintf(stderr, "failed to get HTTP response header value.\n");
    net_close(&client->__internal->info);
    free(client->__internal);
    return false;
  }
  if (strncmp(noonce, RESPONSE_NOONCE, strlen(RESPONSE_NOONCE)) != 0) {
    fprintf(stderr, "WebSocket Client connection was rejected.\n%s\n",
            resp.message.status_text);
    net_close(&client->__internal->info);
    free(client->__internal);
    return false;
  }
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
bool ws_client_recv(struct ws_client_t *client, byte_array *out) {
  // TODO rework to ensure we've read the entire http message
  if (!ws_check_internals(client)) {
    return false;
  }
  uint8_t buffer[BUFSIZ] = {0};
  const ssize_t n = net_read(&client->__internal->info, buffer, BUFSIZ);
  if (n == -1) {
    fprintf(stderr, "WebSocket client recieve failure.\n");
    return false;
  }
  if (!byte_array_init(out, n)) {
    fprintf(stderr, "WebSocket failed to initialize received bytes.\n");
    return false;
  }
  out->len = n;
  return memcpy(out->byte_data, buffer, n) != NULL;
}

bool ws_client_next_msg(struct ws_client_t *client, struct ws_message_t **out) {
  if (!ws_reader_handle(client->__internal->reader,
                        &client->__internal->info)) {
    return false;
  }
  *out = ws_reader_next_msg(client->__internal->reader);
  return true;
}

bool ws_client_on_msg(struct ws_client_t *client, on_message_callback cb, void *context) {
  bool running = true;
  bool close_sock = false;
  while (running) {
    struct ws_message_t *msg = NULL;
    if (!ws_client_next_msg(client, &msg)) {
      fprintf(stderr, "client failed to recv.\n");
      break;
    }
    if (msg == NULL) {
      fprintf(stderr, "message was null\n");
      running = false;
      break;
    }
    switch (msg->type) {
    case OPCODE_CLOSE: {
      running = false;
      close_sock = true;
      break;
    }
    case OPCODE_PING: {
      if (!ws_client_write(client, OPCODE_PONG, msg->body)) {
        fprintf(stderr, "writing pong failed.\n");
        running = false;
      }
      break;
    }
    case OPCODE_BIN:
      /* fall through */
    case OPCODE_TEXT: {
      if (!cb(client, msg, context)) {
        running = false;
      }
      break;
    }
    default:
      break;
    }
    ws_message_free(msg);
    free(msg);
  }
  if (close_sock) {
    ws_client_free(client);
  }
  return true;
}

bool ws_client_write(struct ws_client_t *client, enum ws_opcode_t type, byte_array body) {
  struct ws_frame_t DEFER(ws_frame_free) frame;
  // always returns true
  (void)ws_frame_init(&frame);
  frame.codes.flags.opcode = type;
  // TODO when we support fragmenting messages, make this conditional
  frame.codes.flags.fin = 1;
  // client is required to use a mask
  frame.info.flags.mask = 1;
  // always true
  (void)ws_generate_mask(frame.masking_key, 4);
  if (body.len > 0) {
    if (!byte_array_init(&frame.payload, body.len)) {
      return false;
    }
    if (frame.payload.cap != body.len) {
      // TODO send better error message
      return false;
    }
    if (memcpy(frame.payload.byte_data, body.byte_data, frame.payload.cap) == NULL) {
      return false;
    }
    frame.payload.len = frame.payload.cap;
  }
  byte_array DEFER(byte_array_free) out;
  memset(&out, 0, sizeof(byte_array));
  enum ws_frame_error_t err = ws_frame_write(&frame, &out);
  if (err != WS_FRAME_SUCCESS) {
    return false;
  }
  if (!ws_client_write_blob(client, &out)) {
    return false;
  }
  return true;
}

bool ws_client_write_msg(struct ws_client_t *client, struct ws_message_t *msg) {
  return ws_client_write(client, msg->type, msg->body);
}

bool ws_client_set_net_info(struct ws_client_t *client, struct net_info_t *info) {
  if (client->__internal == NULL) return false;
  client->__internal->info = *info;
  return true;
}

/**
 * Free the internal WebSocket client data.
 *
 * @param client The WebSocket Client.
 */
void ws_client_free(struct ws_client_t *client) {
  if (client == NULL) {
    return;
  }
  if (client->host != NULL) {
    free(client->host);
    client->host = NULL;
  }
  if (client->path != NULL) {
    free(client->path);
    client->path = NULL;
  }
  if (client->__internal != NULL) {
    net_close(&client->__internal->info);
    if (client->__internal->reader != NULL) {
      ws_reader_destroy(&client->__internal->reader);
    }
    free(client->__internal);
    client->__internal = NULL;
  }
}
