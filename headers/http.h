#ifndef CSTD_HTTP_H
#define CSTD_HTTP_H

#include "hash_map.h"
#include "unicode_str.h"
#include <stdbool.h>
#include <stddef.h>

#define HTTP_GET "GET"
#define HTTP_POST "POST"
#define HTTP_PUT "PUT"
#define HTTP_DELETE "DELETE"
#define HTTP_OPTIONS "OPTIONS"

struct http_payload_t {
  char *method;
  struct hash_map_t *headers;
  byte_array body;
};

bool http_payload_init(struct http_payload_t *r);
bool http_payload_parse_string(struct http_payload_t *r, const char *str, size_t len);
bool http_payload_set_header(struct http_payload_t *r, const char *key, const char *value);
bool http_payload_get_header(struct http_payload_t *r, const char *key, char **out);
void http_payload_free(struct http_payload_t *r);

#endif
