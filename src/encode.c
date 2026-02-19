#include "headers/encode.h"
#include "magic.h"

#include <math.h>
#include <openssl/types.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include <openssl/sha.h>

#ifndef WEBC_USE_SSL
// these are only for if we compile without OpenSSL
#define REQUEST_NOONCE "7Wrl5Wp3kkEaYOVhio4o6w=="
#define RESPONSE_NOONCE "mj/2Q6QlJ3Y5pun3vzHGmTO/xgs="
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

char *generate_noonce(uint8_t *buf, size_t len, size_t *output_length) {
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

  const int encodedSize = 4*ceil((double)len/3);
  char *buffer = (char *)malloc(encodedSize+1);
  memcpy(buffer, buffer_ptr, encodedSize);
  *output_length = encodedSize;
  // free all
  BIO_free_all(bio);
  return buffer;
#else
  return REQUEST_NOONCE;
#endif
}

// TODO finish
bool check_response_noonce(uint8_t *buf, size_t buf_len, char *noonce, size_t noonce_len) {
  if (buf == NULL || buf_len == 0 || noonce == NULL || noonce_len == 0) {
    return NULL;
  }
#ifdef WEBC_USE_SSL
  size_t req_len = 0;
  char AUTO_C *req_noonce = generate_noonce(buf, buf_len, &req_len);
  if (req_noonce == NULL) return false;

  // TODO concat req_noonce with this UUID string "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
  // then SHA1 it and compare to response noonce.

  return true;
#else
  return strncmp(noonce, RESPONSE_NOONCE, strlen(RESPONSE_NOONCE)) == 0;
#endif
}
