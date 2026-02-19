#ifndef CSTD_WS_ENCODE_H
#define CSTD_WS_ENCODE_H

#include <stddef.h>
#include <stdint.h>

#include "defs.h"

__BEGIN_DECLS

/**
 * Encode data into Base64.
 *
 * @param data The data to encode.
 * @param len The length of the data.
 * @param[out] output_length The output length of the encoded string.
 * @return The Base64 encoded string, NULL on failure.
 */
char *base64_encode(const uint8_t *data, const size_t len,
                    size_t *output_length);

/**
 * Decode Base64 string.
 *
 * @param data The Base64 data.
 * @param len The length of the data.
 * @param[out] output_length The output length of the decoded data.
 * @return The decoded data, NULL on failure.
 */
uint8_t *base64_decode(const char *data, const size_t len,
                       size_t *output_length);

/**
 * Populate the given buffer with random data.
 *
 * @param[out] buf The buffer to populate.
 * @param[in] len The length of the buffer.
 */
void populate_rand(uint8_t *buf, size_t len);

__END_DECLS

#endif
