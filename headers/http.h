#ifndef CSTD_HTTP_H
#define CSTD_HTTP_H

#include "defs.h"
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

/**
 * Enum for HTTP methods.
 */
enum http_method_t {
  HTTP_INVALID_METHOD = -1,
  HTTP_GET = 0,
  HTTP_POST,
  HTTP_PUT,
  HTTP_DELETE,
  HTTP_OPTIONS,
};

/**
 * Get the string representation of the given HTTP method enum.
 *
 * @param[in] method The HTTP method enum.
 * @return The string representation of the HTTP method.
 */
const char *http_method_get_string(enum http_method_t method);
/**
 * Get the HTTP method enum value from the given string.
 *
 * @param[in] method The HTTP method string.
 * @return The HTTP method enum that matches, HTTP_INVALID_METHOD otherwise.
 */
enum http_method_t http_method_get_enum(const char *method) __nonnull((1));

/**
 * HTTP Message structure.
 */
struct http_message_t {
  /**
   * Protocol of message. (i.e. HTTP/1.1)
   */
  char *protocol;
  /**
   * Method. (i.e. GET, POST, etc)
   */
  enum http_method_t method;
  /**
   * The host. (i.e. 127.0.0.1)
   * Currently only supports IPv4 addresses.
   */
  char *host;
  /**
   * The URL's path. (i.e. /index)
   * Currently does not support query params.
   */
  char *path;
  /**
   * The status text in the response message.
   */
  char *status_text;
  /**
   * The port of the address.
   */
  uint16_t port;
  /**
   * The status code in the response message.
   */
  uint16_t status_code;
  /**
   * The HTTP headers.
   */
  struct hash_map_t *headers;
  /**
   * The HTTP body.
   */
  byte_array body;
};

/**
 * HTTP Response structure.
 */
struct http_response_t {
  struct http_message_t message;
};
/**
 * HTTP Request structure.
 */
struct http_request_t {
  struct http_message_t message;
};

/// HTTP Message functions.

/**
 * Set Header in the HTTP message structure.
 *
 * @param[in] r The HTTP message structure.
 * @param[in] key The Header key, expects a null-terminated string.
 * @param[in] value The Header value, expects a null-terminated string.
 * @return True on success, False otherwise.
 */
bool http_message_set_header(struct http_message_t *msg, const char *key,
                             char *value) __nonnull((1, 2, 3));

/**
 * Get Header from the HTTP message structure.
 *
 * @param[in] r The HTTP message structure.
 * @param[in] key The Header key, expects a null-terminated string.
 * @param[out] out The Header value to populate.
 * @return True on success, False otherwise.
 */
bool http_message_get_header(struct http_message_t *msg, const char *key,
                             char **out) __nonnull((1, 2));

/// HTTP Response functions.

/**
 * Initialize internals of HTTP response.
 *
 * @param[in] r The HTTP response structure.
 * @return True on success, False otherwise.
 */
bool http_response_init(struct http_response_t *r) __nonnull((1));

/**
 * Populate HTTP response with given response string.
 *
 * @param[in] r The HTTP response structure.
 * @param[in] str The response string.
 * @param[in] len The length of the response string.
 * @return True on success, False otherwise.
 */
bool http_response_from_str(struct http_response_t *r, const char *str,
                            size_t len) __nonnull((1, 2));

/**
 * Get the HTTP response string from the given HTTP response structure.
 * The caller is responsible for freeing the returned string.
 *
 * @param[in] r The HTTP response structure.
 * @return The HTTP response string, NULL if failure.
 */
char *http_response_to_str(struct http_response_t *r) __nonnull((1));

/**
 * Set Header in the HTTP response structure.
 *
 * @param[in] r The HTTP response structure.
 * @param[in] key The Header key, expects a null-terminated string.
 * @param[in] value The Header value, expects a null-terminated string.
 * @return True on success, False otherwise.
 */
bool http_response_set_header(struct http_response_t *r, const char *key,
                              char *value) __nonnull((1, 2, 3));

/**
 * Get Header from the HTTP response structure.
 *
 * @param[in] r The HTTP response structure.
 * @param[in] key The Header key, expects a null-terminated string.
 * @param[out] out The Header value to populate.
 * @return True on success, False otherwise.
 */
bool http_response_get_header(struct http_response_t *r, const char *key,
                              char **out) __nonnull((1, 2));

/**
 * Free the internals of the given HTTP response structure.
 *
 * @param[in] r The HTTP response structure.
 */
void http_response_free(struct http_response_t *r);

/// HTTP Request functions.

/**
 * Initialize internals of HTTP request.
 *
 * @param[in] r The HTTP request structure.
 * @return True on success, False otherwise.
 */
bool http_request_init(struct http_request_t *r) __nonnull((1));

/**
 * Populate HTTP request with given request string.
 *
 * @param[in] r The HTTP request structure.
 * @param[in] str The request string.
 * @param[in] len The length of the request string.
 * @return True on success, False otherwise.
 */
bool http_request_from_str(struct http_request_t *r, const char *str,
                           size_t len) __nonnull((1, 2));

/**
 * Get the HTTP request string from the given HTTP request structure.
 * The caller is responsible for freeing the returned string.
 *
 * @param[in] r The HTTP request structure.
 * @return The HTTP request string, NULL if failure.
 */
char *http_request_to_str(struct http_request_t *r) __nonnull((1));

/**
 * Set Header in the HTTP request structure.
 *
 * @param[in] r The HTTP request structure.
 * @param[in] key The Header key, expects a null-terminated string.
 * @param[in] value The Header value, expects a null-terminated string.
 * @return True on success, False otherwise.
 */
bool http_request_set_header(struct http_request_t *r, const char *key,
                             char *value) __nonnull((1, 2, 3));

/**
 * Get Header from the HTTP request structure.
 *
 * @param[in] r The HTTP request structure.
 * @param[in] key The Header key, expects a null-terminated string.
 * @param[out] out The Header value to populate.
 * @return True on success, False otherwise.
 */
bool http_request_get_header(struct http_request_t *r, const char *key,
                             char **out) __nonnull((1, 2));

/**
 * Write C string to request body.
 * This operation copies the given bytes.
 *
 * @param[in] r The HTTP request structure.
 * @param[in] str The C string to write.
 * @param[in] len The length of the C string.
 * @return True on success, False otherwise.
 */
bool http_request_write_cstr(struct http_request_t *r, const char *str,
                             size_t len) __nonnull((1, 2));

/**
 * Write buffer to request body.
 * This operation copies the given bytes.
 *
 * @param[in] r The HTTP request structure.
 * @param[in] buf The buffer to write.
 * @param[in] len The length of the C string.
 * @return True on success, False otherwise.
 */
bool http_request_write(struct http_request_t *r, const uint8_t *buf,
                        size_t len) __nonnull((1, 2));

/**
 * Free the internals of the given HTTP request structure.
 *
 * @param[in] r The HTTP request structure.
 */
void http_request_free(struct http_request_t *r);

#endif
