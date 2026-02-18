#include "headers/encode.h"

#include <stdlib.h>
#include <string.h>

static char encoding_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *base64_encode(const uint8_t *data, const size_t len,
                    size_t *output_length) {
  if (data == NULL || len == 0) {
    return NULL;
  }
  const int result_len = 4 * ((len + 2) / 3);
  char *encoded_string = malloc(result_len + 1);
  if (encoded_string == NULL)
    return NULL;

  for (int i = 0, j = 0; i < len;) {
    const uint32_t octet_a = i < len ? data[i++] : 0;
    const uint32_t octet_b = i < len ? data[i++] : 0;
    const uint32_t octet_c = i < len ? data[i++] : 0;

    const uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

    encoded_string[j++] = encoding_table[(triple >> 18) & 0x3F];
    encoded_string[j++] = encoding_table[(triple >> 12) & 0x3F];
    encoded_string[j++] = encoding_table[(triple >> 6) & 0x3F];
    encoded_string[j++] = encoding_table[triple & 0x3F];
  }

  // Add padding if necessary
  for (int i = 0; i < (3 - len % 3) % 3; i++) {
    encoded_string[result_len - 1 - i] = '=';
  }
  encoded_string[result_len] = '\0';
  *output_length = result_len;
  return encoded_string;
}

/**
 * Helper to map ASCII characters back to 6-bit values
 */
static uint32_t base64_char_value(char c) {
  if (c >= 'A' && c <= 'Z') {
    return c - 'A';
  }
  if (c >= 'a' && c <= 'z') {
    return c - 'a' + 26;
  }
  if (c >= '0' && c <= '9') {
    return c - '0' + 52;
  }
  if (c == '+') {
    return 62;
  }
  if (c == '/') {
    return 63;
  }
  // For padding '=' or invalid characters
  return -1;
}

uint8_t *base64_decode(const char *data, const size_t len,
                       size_t *output_length) {
  if (data == NULL || len == 0) {
    return NULL;
  }
  // Base64 must be a multiple of 4
  if ((len % 4) != 0) {
    return NULL;
  }

  // Calculate output size
  size_t result_len = (len / 4) * 3;
  // subtract any padding
  if (data[len - 1] == '=') {
    result_len--;
  }
  if (data[len - 2] == '=') {
    result_len--;
  }

  uint8_t *decoded_data = (uint8_t *)malloc(result_len + 1);
  if (decoded_data == NULL) {
    return NULL;
  }

  for (int i = 0, j = 0; i < len;) {
    // Grab 4 characters and convert to 6-bit values
    const uint32_t sexte_a = data[i] == '=' ? 0 : base64_char_value(data[i++]);
    const uint32_t sexte_b = data[i] == '=' ? 0 : base64_char_value(data[i++]);
    const uint32_t sexte_c = data[i] == '=' ? 0 : base64_char_value(data[i++]);
    const uint32_t sexte_d = data[i] == '=' ? 0 : base64_char_value(data[i++]);

    // Combine into a 24-bit triple
    const uint32_t triple =
        (sexte_a << 18) + (sexte_b << 12) + (sexte_c << 6) + sexte_d;

    // Extract the 3 bytes
    if (j < *output_length) {
      decoded_data[j++] = (triple >> 16) & 0xFF;
    }
    if (j < *output_length) {
      decoded_data[j++] = (triple >> 8) & 0xFF;
    }
    if (j < *output_length) {
      decoded_data[j++] = triple & 0xFF;
    }
  }
  decoded_data[result_len] = '\0';
  *output_length = result_len;
  return decoded_data;
}
