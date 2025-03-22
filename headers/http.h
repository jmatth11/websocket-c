#ifndef CSTD_HTTP_H
#define CSTD_HTTP_H

#include "hash_map.h"
#include "unicode_str.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HTTP_METHOD_GET "GET"
#define HTTP_METHOD_POST "POST"
#define HTTP_METHOD_PUT "PUT"
#define HTTP_METHOD_DELETE "DELETE"
#define HTTP_METHOD_OPTIONS "OPTIONS"

enum http_method_t {
  HTTP_GET = 0,
  HTTP_POST,
  HTTP_PUT,
  HTTP_DELETE,
  HTTP_OPTIONS,
};

char* http_method_get_string(enum http_method_t method);
enum http_method_t http_method_get_enum(const char *method);

struct http_message_t {
  char *protocol;
  enum http_method_t method;
  char *host;
  char *path;
  char *status_text;
  uint16_t port;
  uint16_t status_code;
  struct hash_map_t *headers;
  byte_array body;
};
struct http_response_t {
  struct http_message_t message;
};
struct http_request_t {
  struct http_message_t message;
};

bool http_response_init(struct http_response_t *r);

bool http_response_from_str(struct http_response_t *r, const char *str, size_t len);

char *http_response_to_str(struct http_response_t *r);

bool http_response_set_header(struct http_response_t *r, const char *key, char *value);

bool http_response_get_header(struct http_response_t *r, const char *key, char **out);

void http_response_free(struct http_response_t *r);

#endif
