#include "headers/http.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "base_str.h"
#include "hash_map.h"
#include "string_ops.h"
#include "unicode_str.h"

const char *http_method_get_string(enum http_method_t method) {
  switch (method) {
  case HTTP_GET:
    return HTTP_METHOD_GET;
  case HTTP_POST:
    return HTTP_METHOD_POST;
  case HTTP_PUT:
    return HTTP_METHOD_PUT;
  case HTTP_DELETE:
    return HTTP_METHOD_DELETE;
  case HTTP_OPTIONS:
    return HTTP_METHOD_OPTIONS;
  default:
    return "";
  }
}

enum http_method_t http_method_get_enum(const char *method) {
  if (strncmp(HTTP_METHOD_GET, method, strlen(HTTP_METHOD_GET)) == 0) {
    return HTTP_GET;
  }
  if (strncmp(HTTP_METHOD_POST, method, strlen(HTTP_METHOD_POST)) == 0) {
    return HTTP_POST;
  }
  if (strncmp(HTTP_METHOD_PUT, method, strlen(HTTP_METHOD_PUT)) == 0) {
    return HTTP_PUT;
  }
  if (strncmp(HTTP_METHOD_DELETE, method, strlen(HTTP_METHOD_DELETE)) == 0) {
    return HTTP_DELETE;
  }
  if (strncmp(HTTP_METHOD_OPTIONS, method, strlen(HTTP_METHOD_OPTIONS)) == 0) {
    return HTTP_OPTIONS;
  }
  return HTTP_INVALID_METHOD;
}

static char * normalized_cstr(const char *str, size_t len) {
  size_t str_len = strlen(str);
  struct unicode_str_t *uni_str = unicode_str_create();
  if (unicode_str_set_char(uni_str, str, str_len) < str_len) {
    fprintf(stderr, "failed to set header during unicode conversion.\n");
    unicode_str_destroy(uni_str);
    return false;
  }
  struct unicode_str_t *new_str = unicode_str_to_lower(uni_str);
  if (new_str == NULL) {
    fprintf(stderr, "failed to generate normalized str for header.\n");
    unicode_str_destroy(uni_str);
    return false;
  }
  char *normalized_str = unicode_str_to_cstr(new_str);
  unicode_str_destroy(uni_str);
  unicode_str_destroy(new_str);
  return normalized_str;
}

static bool http_message_init(struct http_message_t *msg) {
  msg->host = NULL;
  msg->path = NULL;
  msg->method = HTTP_GET;
  msg->port = 80;
  msg->headers = hash_map_create(1000);
  if (msg->headers == NULL) {
    return false;
  }
  if (!byte_array_init(&msg->body, 5)) {
    return false;
  }
  return true;
}

static void http_message_free(struct http_message_t *msg) {
  if (msg->host != NULL) {
    free(msg->host);
    msg->host = NULL;
  }
  if (msg->path != NULL) {
    free(msg->path);
    msg->path = NULL;
  }
  if (msg->protocol != NULL) {
    free(msg->path);
    msg->protocol = NULL;
  }
  msg->method = HTTP_INVALID_METHOD;
  hash_map_destroy(msg->headers, true);
  byte_array_free(&msg->body);
}

bool http_message_set_header(struct http_message_t *msg, const char *key,
                             char *value) {
  size_t key_len = strlen(key);
  char *normalized_key = normalized_cstr(key, key_len);
  // free previous value.
  char *out = NULL;
  http_message_get_header(msg, key, &out);
  if (out != NULL) {
    free(out);
  }
  // set new value, we copy the value to have ownership
  if (!hash_map_set(msg->headers, normalized_key,
                    str_dup(value, strlen(value)))) {
    fprintf(stderr, "failed to set header.\n");
    free(normalized_key);
    return false;
  }
  return true;
}

bool http_message_get_header(struct http_message_t *msg, const char *key,
                             char **out) {
  size_t key_len = strlen(key);
  char *normalized_key = normalized_cstr(key, key_len);
#ifdef DEBUG
  printf("normalized_key= \"%s\"\n", normalized_key);
  fflush(stdout);
#endif
  if (!hash_map_get(msg->headers, normalized_key, (void **)out)) {
    fprintf(stderr, "failed to get value from headers.\n");
    free(normalized_key);
    return false;
  }
  free(normalized_key);
  return true;
}

/**
 * Function to parse the generic portion of the http message structure.
 */
bool http_message_from_str(struct http_message_t *msg, const char *str,
                           size_t len, size_t offset) {
  size_t index = offset;
  while (index < len) {
    size_t line_len = strcspn(&str[index], "\n");
    // if line_len is 1 it means we hit \r\n header ending
    if (line_len == 1) {
      // skip past the ending \r\n
      index += 2;
      break;
    }
    size_t key_len = strcspn(&str[index], ":");
    char *key = str_dup(&str[index], key_len);
#ifdef DEBUG
    printf("key=\"%s\"\n", key);
    fflush(stdout);
#endif
    // plus 2 for : and following space.
    index += key_len + 2;
    size_t value_len = strcspn(&str[index], "\r");
    char *value = str_dup(&str[index], value_len);
#ifdef DEBUG
    printf("value=\"%s\"\n", value);
    fflush(stdout);
#endif
    if (!http_message_set_header(msg, key, value)) {
      free(key);
      free(value);
      fprintf(stderr, "failed to set header.\n");
      return false;
    }
    free(key);
    free(value);
    // plus 2 for \r\n at the end of the value.
    index += value_len + 2;
  }
  // if we haven't hit the end of string the rest is the body
  for (; index < len; ++index) {
    if (!byte_array_insert(&msg->body, (uint8_t)str[index])) {
      fprintf(stderr, "failed to insert into response body.\n");
      return false;
    }
  }
  return true;
}

/**
 * Function to write out the generic portion of the http message structure.
 */
bool http_message_to_str(struct http_message_t *msg, base_str *result) {
  struct hash_map_iterator_t *it = hash_map_iterator(msg->headers);
  struct hash_map_entry_t *cur_entry = hash_map_iterator_next(it);
  while (cur_entry != NULL) {
    result->append(result, cur_entry->key, strlen(cur_entry->key));
    result->append(result, ": ", 2);
    char *value = (char *)cur_entry->value;
    result->append(result, value, strlen(value));
    result->append(result, "\r\n", 2);
  }
  free(it);
  // empty line
  result->append(result, "\r\n", 2);

  if (msg->body.len > 0) {
    for (size_t i = 0; i < msg->body.len; ++i) {
      // convert to char
      char b = msg->body.byte_data[i];
      result->append(result, &b, 1);
    }
  }
  result->append(result, "\r\n", 2);
  return true;
}

bool http_response_init(struct http_response_t *r) {
  return http_message_init(&r->message);
}

static size_t parse_response_start_line(struct http_response_t *r,
                                        const char *str, size_t len) {
  size_t index = 0;
  size_t word_len = strcspn(str, " ");
  r->message.protocol = str_dup(str, word_len);
  // plus 1 to skip empty string.
  index += word_len + 1;
  word_len = strcspn(&str[index], " ");
  char status_code[4] = {0};
  if (strncpy(status_code, &str[index], 3) == NULL) {
    fprintf(stderr, "failed to copy status code.\n");
    free(r->message.protocol);
    return 0;
  }
  r->message.status_code = atoi(status_code);
  index += word_len + 1;
  word_len = strcspn(&str[index], "\r");
  r->message.status_text = str_dup(&str[index], word_len);
  index += word_len;
  return index;
}

bool http_response_from_str(struct http_response_t *r, const char *str,
                            size_t len) {
  size_t index = 0;
  size_t line_len = strcspn(str, "\n");
  if (parse_response_start_line(r, str, line_len) == 0) {
    fprintf(stderr, "parsing response start line failed.\n");
    return false;
  }
  index += line_len + 1;
#ifdef DEBUG
  printf("index=%zu -- len=%zu\n", index, len);
  fflush(stdout);
#endif
  return http_message_from_str(&r->message, str, len, index);
}

char *http_response_to_str(struct http_response_t *r) {
  base_str result;
  if (new_base_str(&result, 20) != C_STR_NO_ERROR) {
    fprintf(stderr, "failed to generate response string.\n");
    return false;
  }
  result.append(&result, r->message.protocol, strlen(r->message.protocol));
  result.append(&result, " ", 1);
  char status_code[3] = {0};
  snprintf(status_code, 3, "%d", r->message.status_code);
  result.append(&result, status_code, 3);
  result.append(&result, " ", 1);
  result.append(&result, r->message.status_text,
                strlen(r->message.status_text));
  result.append(&result, "\r\n", 2);
  if (!http_message_to_str(&r->message, &result)) {
    fprintf(stderr, "failed to write out http message structure.\n");
    return NULL;
  }
  char *out = NULL;
  if (result.get_str(&result, &out) != C_STR_NO_ERROR) {
    free_base_str(&result);
    return NULL;
  }
  // we don't free the result object because we send the internal data out to
  // the caller.
  return out;
}

bool http_response_set_header(struct http_response_t *r, const char *key,
                              char *value) {
  return http_message_set_header(&r->message, key, value);
}

bool http_response_get_header(struct http_response_t *r, const char *key,
                              char **out) {
  return http_message_get_header(&r->message, key, out);
}

void http_response_free(struct http_response_t *r) {
  http_message_free(&r->message);
}

/// HTTP Request functions

static size_t parse_request_start_line(struct http_request_t *r,
                                       const char *str, size_t len) {
  size_t index = 0;
  size_t word_len = strcspn(str, " ");
  char *method = str_dup(str, word_len);
  r->message.method = http_method_get_enum(method);
  free(method);
  // plus 1 to skip empty string.
  index += word_len + 1;
  word_len = strcspn(&str[index], " ");
  r->message.path = str_dup(&str[index], word_len);
  index += word_len + 1;
  word_len = strcspn(&str[index], "\r");
  r->message.protocol = str_dup(&str[index], word_len);
  index += word_len + 2;
  return index;
}

bool http_request_init(struct http_request_t *r) {
  return http_message_init(&r->message);
}

bool http_request_from_str(struct http_request_t *r, const char *str,
                           size_t len) {

  size_t index = 0;
  size_t line_len = strcspn(str, "\n");
  if (parse_request_start_line(r, str, line_len) == 0) {
    fprintf(stderr, "parsing request start line failed.\n");
    return false;
  }
  index += line_len + 1;
#ifdef DEBUG
  printf("index=%zu -- len=%zu\n", index, len);
  fflush(stdout);
#endif
  return http_message_from_str(&r->message, str, len, index);
}

char *http_request_to_str(struct http_request_t *r) {
  base_str result;
  if (new_base_str(&result, 20) != C_STR_NO_ERROR) {
    fprintf(stderr, "failed to generate response string.\n");
    return false;
  }
  const char *method = http_method_get_string(r->message.method);
  result.append(&result, method, strlen(method));
  result.append(&result, " ", 1);
  result.append(&result, r->message.path, strlen(r->message.path));
  result.append(&result, " ", 1);
  result.append(&result, r->message.protocol, strlen(r->message.protocol));
  result.append(&result, "\r\n", 2);

  size_t host_len = snprintf(NULL, 0, "%s:%d", r->message.host, r->message.port) + 1;
  char *host = malloc((sizeof(char)*host_len) + 1);
  host_len = snprintf(host, host_len, "%s:%d", r->message.host, r->message.port);
  host[host_len] = '\0';
  if (!http_request_set_header(r, "Host", host)) {
    fprintf(stderr, "failed to set host header in request.\n");
    free(host);
    free_base_str(&result);
  }
  free(host);

  if (!http_message_to_str(&r->message, &result)) {
    fprintf(stderr, "failed to write out http message structure.\n");
    free_base_str(&result);
    return NULL;
  }
  char *out = NULL;
  if (result.get_str(&result, &out) != C_STR_NO_ERROR) {
    free_base_str(&result);
    return NULL;
  }
  // we don't free the result object because we send the internal data out to
  // the caller.
  return out;
}

bool http_request_set_header(struct http_request_t *r, const char *key,
                             char *value) {
  return http_message_set_header(&r->message, key, value);
}

bool http_request_get_header(struct http_request_t *r, const char *key,
                             char **out) {
  return http_message_get_header(&r->message, key, out);
}

bool http_request_write_cstr(struct http_request_t *r, const char *str,
                             size_t len) {
  for (size_t i = 0; i < len; ++i) {
    if (!byte_array_insert(&r->message.body, (uint8_t)str[i])) {
      fprintf(stderr, "failed writing c string to request body.\n");
      return false;
    }
  }
  return true;
}

bool http_request_write(struct http_request_t *r, const uint8_t *buf,
                        size_t len) {
  for (size_t i = 0; i < len; ++i) {
    if (!byte_array_insert(&r->message.body, buf[i])) {
      fprintf(stderr, "failed writing string to request body.\n");
      return false;
    }
  }
  return true;
}

void http_request_free(struct http_request_t *r) {
  http_message_free(&r->message);
}
