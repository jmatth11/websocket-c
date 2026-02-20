#include "headers/encode.h"
#include "magic.h"
#include "string_ops.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef WEBC_USE_SSL
// these are only for if we compile without OpenSSL
#define REQUEST_NOONCE "7Wrl5Wp3kkEaYOVhio4o6w=="
#define RESPONSE_NOONCE "mj/2Q6QlJ3Y5pun3vzHGmTO/xgs="
#else

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/types.h>

// Value comes from the RFC page 19 bullet number 4.
// https://datatracker.ietf.org/doc/html/rfc6455#page-19
#define WS_KEY_UUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#endif


void populate_rand(uint8_t *buf, size_t len) {
  if (buf == NULL || len == 0) {
    return;
  }
  // TODO maybe change to have better RNG
  srand(time(NULL));
  size_t max = UINT8_MAX + 1;
  for (size_t i = 0; i < len; ++i) {
    buf[i] = (rand() % max);
  }
}

char *base64_encode(uint8_t *buf, size_t len, size_t *output_length) {
  if (buf == NULL || len == 0) {
    return NULL;
  }
#ifdef WEBC_USE_SSL
  BIO *bio = NULL;
  BIO *b64 = NULL;
  b64 = BIO_new(BIO_f_base64());
  bio = BIO_new(BIO_s_mem());
  bio = BIO_push(b64, bio);
  // Ignore newlines - write everything in one line
  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
  (void)BIO_write(bio, buf, len);
  (void)BIO_flush(bio);

  BUF_MEM *buffer_ptr;
  (void)BIO_get_mem_ptr(bio, &buffer_ptr);

  const int encodedSize = 4 * ceil((double)len / 3);
  char *buffer = (char *)malloc(encodedSize + 1);
  memcpy(buffer, buffer_ptr, encodedSize);
  buffer[encodedSize] = '\0';
  *output_length = encodedSize;
  // free all
  BIO_free_all(bio);
  return buffer;
#else
  *output_length = strlen(REQUEST_NOONCE);
  return REQUEST_NOONCE;
#endif
}

bool check_response_noonce(uint8_t *buf, size_t buf_len, char *noonce,
                           size_t noonce_len) {
  if (buf == NULL || buf_len == 0 || noonce == NULL || noonce_len == 0) {
    return false;
  }
#ifdef WEBC_USE_SSL
  size_t req_len = 0;
  // grab request noonce
  char AUTO_C *req_noonce = base64_encode(buf, buf_len, &req_len);
  if (req_noonce == NULL)
    return false;

  // recreate responce noonce
  size_t resp_noonce_len = 0;
  char AUTO_C *resp_noonce = concat(req_noonce, WS_KEY_UUID, &resp_noonce_len);
  if (resp_noonce == NULL || resp_noonce_len == 0) {
    return false;
  }

  // sha1 the response noonce
  uint8_t digest[SHA_DIGEST_LENGTH];
  if (SHA1((const unsigned char *)resp_noonce, resp_noonce_len, digest) ==
      NULL) {
    return false;
  }
  size_t base64_resp_len = 0;
  char AUTO_C *base64_resp_noonce =
      base64_encode(digest, SHA_DIGEST_LENGTH, &base64_resp_len);
  if (base64_resp_noonce == NULL || base64_resp_len == 0) {
    return false;
  }
  return strncmp(base64_resp_noonce, noonce, noonce_len) == 0;
#else
  return strncmp(noonce, RESPONSE_NOONCE, strlen(RESPONSE_NOONCE)) == 0;
#endif
}
