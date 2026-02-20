#ifndef CSTD_WS_ENCODE_H
#define CSTD_WS_ENCODE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "defs.h"

__BEGIN_DECLS

/**
 * Generate base64 string from the given buffer.
 * The caller is responsible for freeing the returned string.
 *
 * @param[in] buf The input buffer.
 * @param[in] len The length of the input buffer.
 * @param[out] output_length The length of the generated base64 string.
 * @return The generated base64 string, NULL on failure.
 */
char *base64_encode(uint8_t *buf, size_t len, size_t *output_length);

/**
 * Check the response noonce with the given buffer.
 *
 * @param[in] buf The input buffer.
 * @param[in] buf_len The length of the input buffer.
 * @param[in] noonce The response noonce.
 * @param[in] noonce_len The length of the response noonce.
 * @return True if the check passes, false otherwise.
 */
bool check_response_noonce(uint8_t *buf, size_t buf_len, char *noonce, size_t noonce_len);

/**
 * Populate the given buffer with random data.
 *
 * @param[out] buf The buffer to populate.
 * @param[in] len The length of the buffer.
 */
void populate_rand(uint8_t *buf, size_t len);

__END_DECLS

#endif
