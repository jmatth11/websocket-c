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
  struct unicode_str_t *uni_key = unicode_str_create();
  if (unicode_str_set_char(uni_key, key, key_len) < key_len) {
    fprintf(stderr, "failed to set header during unicode conversion.\n");
    unicode_str_destroy(uni_key);
    return false;
  }
  struct unicode_str_t *new_key = unicode_str_to_lower(uni_key);
  if (new_key == NULL) {
    fprintf(stderr, "failed to generate normalized key for header.\n");
    return false;
  }
  if (!hash_map_set(msg->headers, unicode_str_to_cstr(new_key), value)) {
    fprintf(stderr, "failed to set header.\n");
    unicode_str_destroy(uni_key);
    unicode_str_destroy(new_key);
    return false;
  }
  unicode_str_destroy(uni_key);
  unicode_str_destroy(new_key);
  return true;
}

bool http_message_get_header(struct http_message_t *msg, const char *key,
                              char **out) {
  size_t key_len = strlen(key);
  struct unicode_str_t *uni_key = unicode_str_create();
  if (unicode_str_set_char(uni_key, key, key_len) < key_len) {
    fprintf(stderr, "failed to set header during unicode conversion.\n");
    unicode_str_destroy(uni_key);
    return false;
  }
  struct unicode_str_t *new_key = unicode_str_to_lower(uni_key);
  if (new_key == NULL) {
    fprintf(stderr, "failed to generate normalized key for header.\n");
    return false;
  }
  char *normalized_key = unicode_str_to_cstr(new_key);
#ifdef DEBUG
  printf("normalized_key= \"%s\"\n", normalized_key);
  fflush(stdout);
#endif
  if (!hash_map_get(msg->headers, normalized_key, (void **)out)) {
    fprintf(stderr, "failed to get value from headers.\n");
    free(normalized_key);
    unicode_str_destroy(uni_key);
    unicode_str_destroy(new_key);
    return false;
  }
  free(normalized_key);
  unicode_str_destroy(uni_key);
  unicode_str_destroy(new_key);
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
  while (index < len) {
    line_len = strcspn(&str[index], "\n");
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
    if (!http_response_set_header(r, key, value)) {
      free(key);
      free(value);
      fprintf(stderr, "failed to set header.\n");
      return false;
    }
    // plus 2 for \r\n at the end of the value.
    index += value_len + 2;
    free(key);
  }
  // if we haven't hit the end of string the rest is the body
  for (; index < len; ++index) {
    if (!byte_array_insert(&r->message.body, (uint8_t)str[index])) {
      fprintf(stderr, "failed to insert into response body.\n");
      return false;
    }
  }
  return true;
}

char *http_response_to_str(struct http_response_t *r) {
  // TODO implement
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
  struct hash_map_iterator_t *it = hash_map_iterator(r->message.headers);
  struct hash_map_entry_t *cur_entry = hash_map_iterator_next(it);
  while (cur_entry != NULL) {
    result.append(&result, cur_entry->key, strlen(cur_entry->key));
    result.append(&result, ": ", 2);
    char *value = (char *)cur_entry->value;
    result.append(&result, value, strlen(value));
    result.append(&result, "\r\n", 2);
  }
  free(it);
  result.append(&result, "\r\n", 2);
  if (r->message.body.len > 0) {
    for (size_t i = 0; i < r->message.body.len; ++i) {
      // convert to char
      char b = r->message.body.byte_data[i];
      result.append(&result, &b, 1);
    }
  }
  result.append(&result, "\r\n", 2);
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
