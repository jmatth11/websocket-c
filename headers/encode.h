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

__END_DECLS

#endif
