#ifndef WEBSOCKETS_SIMD_H
#define WEBSOCKETS_SIMD_H

#include <stddef.h>
#include <stdint.h>

#include "headers/protocol.h"
#include "defs.h"

__BEGIN_DECLS

/**
 * Apply mask to the src buffer into the dest buffer.
 *
 * @param[in] masking_key The masking key to use.
 * @param[out] dest The destination buffer.
 * @param[in] src The source buffer.
 * @param[in] len The length of the source buffer.
 * @return WS_FRAME_SUCCESS for success.
 */
enum ws_frame_error_t apply_mask_to_buffer(
    uint8_t masking_key[4],
    uint8_t *restrict dest,
    uint8_t *restrict src,
    size_t len) __nonnull((2, 3));

__END_DECLS

#endif
