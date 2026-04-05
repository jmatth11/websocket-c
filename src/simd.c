#include "headers/simd.h"

#include "defs.h"
#include <string.h>

static enum ws_frame_error_t
apply_mask_to_buffer_serial(uint8_t masking_key[4], uint8_t *restrict dest,
                            uint8_t *restrict src, size_t len, size_t offset) {
  if (len == 0) {
    return WS_FRAME_SUCCESS;
  }
  for (size_t index = offset; index < len; index++) {
    dest[index] = src[index] ^ masking_key[index & 3];
  }
  return WS_FRAME_SUCCESS;
}

/**
 * SIMD is only supported on certain platforms
 * Supported platforms in this block:
 * - x86
 * - x86_64
 * - arm64
 * - arm32 7A
 */
#if (defined(__i386__) || defined(__x86_64__) || defined(__aarch64__) ||       \
     (defined(__arm__) && defined(__ARM_ARCH_7A__))) &&                        \
    !defined(DISABLE_SIMD)

// we prioritize the clang/gcc vector extensions
#if defined(__clang__) || defined(__GNUC__)
// Generic clang/gcc SIMD approach (preferred)

/**
 * 16 element u8 vector.
 */
typedef uint8_t v16u8 __vector_size(16);

static enum ws_frame_error_t apply_mask_to_buffer_simd(uint8_t masking_key[4],
                                                       uint8_t *restrict dest,
                                                       uint8_t *restrict src,
                                                       size_t len) {
  if (len == 0) {
    return WS_FRAME_SUCCESS;
  }
  if (len <= 15) {
    return apply_mask_to_buffer_serial(masking_key, dest, src, len, 0);
  }
  // operate on 16 bytes at a time.
  size_t offset = 15;
  const size_t cutoff = len;
  const uint8_t m1 = masking_key[0];
  const uint8_t m2 = masking_key[1];
  const uint8_t m3 = masking_key[2];
  const uint8_t m4 = masking_key[3];
  // repeat the mask 4 times.
  v16u8 mask_simd = {m1, m2, m3, m4, m1, m2, m3, m4,
                     m1, m2, m3, m4, m1, m2, m3, m4};

  while (offset < cutoff) {
    // grab the pointer to the offset and cast it to a vector type
    // need to copy data out and in since I can't guarantee memory alignment
    // coming from the uint8_t array.
    v16u8 src_vec;
    memcpy(&src_vec, &src[offset - 15], sizeof(src_vec));
    src_vec ^= mask_simd;
    memcpy(&dest[offset - 15], &src_vec, sizeof(src_vec));
    offset += 16;
  }
  // convert the remaining bytes.
  return apply_mask_to_buffer_serial(masking_key, dest, src, len, offset - 15);
}
#elif defined(__ARM_NEON) && !defined(DISABLE_SIMD)

// ARM SIMD

#include <arm_neon.h>

/**
 * ARM SIMD implementation of mask handling.
 */
static enum ws_frame_error_t apply_mask_to_buffer_simd(uint8_t masking_key[4],
                                                       uint8_t *restrict dest,
                                                       uint8_t *restrict src,
                                                       size_t len) {
  if (len == 0) {
    return WS_FRAME_SUCCESS;
  }
  if (len <= 15) {
    return ws_frame_handle_payload_serial(masking_key, dest, src, len, 0);
  }
  // operate on 16 bytes at a time.
  size_t offset = 15;
  const size_t cutoff = len;
  const uint8_t m1 = masking_key[0];
  const uint8_t m2 = masking_key[1];
  const uint8_t m3 = masking_key[2];
  const uint8_t m4 = masking_key[3];
  // repeat the mask 4 times.
  const uint8_t mask_buf[16] = {m4, m3, m2, m1, m4, m3, m2, m1,
                                m4, m3, m2, m1, m4, m3, m2, m1};
  // load mask into register
  uint8x16_t mask_simd = vld1q_u8(mask_buf);

  while (offset < cutoff) {
    // loading them in reverse order so we can store them directly.
    const uint8_t vec_buf = {
        src[offset],      src[offset - 1],  src[offset - 2],  src[offset - 3],
        src[offset - 4],  src[offset - 5],  src[offset - 6],  src[offset - 7],
        src[offset - 8],  src[offset - 9],  src[offset - 10], src[offset - 11],
        src[offset - 12], src[offset - 13], src[offset - 14], src[offset - 15]};
    uint8x16_t vec1 = vld1q_u8(vec_buf);
    uint8x16_t result = veorq_u8(vec1, mask_simd);
    vst1q_u8(&dest[offset - 15], result);
    offset += 16;
  }
  // convert the remaining bytes.
  return ws_frame_handle_payload_serial(masking_key, dest, src, len,
                                        offset - 15);
}
#elif defined(__SSE2__) && !defined(DISABLE_SIMD)

// Intel SIMD SSE2

#include <immintrin.h>

/**
 * Intel SIMD implementation of mask handling.
 */
static enum ws_frame_error_t apply_mask_to_buffer_simd(uint8_t masking_key[4],
                                                       uint8_t *restrict dest,
                                                       uint8_t *restrict src,
                                                       size_t len) {
  if (len == 0) {
    return WS_FRAME_SUCCESS;
  }
  if (len <= 15) {
    return ws_frame_handle_payload_serial(masking_key, dest, src, len, 0);
  }
  // operate on 16 bytes at a time.
  size_t offset = 15;
  const size_t cutoff = len;
  const uint8_t m1 = masking_key[0];
  const uint8_t m2 = masking_key[1];
  const uint8_t m3 = masking_key[2];
  const uint8_t m4 = masking_key[3];
  // repeat the mask 4 times.
  __m128i mask_simd = _mm_set_epi8(m4, m3, m2, m1, m4, m3, m2, m1, m4, m3, m2,
                                   m1, m4, m3, m2, m1);

  while (offset < cutoff) {
    // loading them in reverse order so we can store them directly.
    __m128i vec1 = _mm_set_epi8(
        src[offset], src[offset - 1], src[offset - 2], src[offset - 3],
        src[offset - 4], src[offset - 5], src[offset - 6], src[offset - 7],
        src[offset - 8], src[offset - 9], src[offset - 10], src[offset - 11],
        src[offset - 12], src[offset - 13], src[offset - 14], src[offset - 15]);
    __m128i result = _mm_xor_si128(vec1, mask_simd);
    _mm_storeu_si128((__m128i *)&dest[offset - 15], result);
    offset += 16;
  }
  // convert the remaining bytes.
  return ws_frame_handle_payload_serial(masking_key, dest, src, len,
                                        offset - 15);
}

#else

// for some reason there is no supported SIMD functionality so default to serial
static enum ws_frame_error_t apply_mask_to_buffer_simd(uint8_t masking_key[4],
                                                       uint8_t *restrict dest,
                                                       uint8_t *restrict src,
                                                       size_t len) {
  return ws_frame_handle_payload_serial(masking_key, dest, src, len, 0);
}

#endif

#endif

enum ws_frame_error_t apply_mask_to_buffer(uint8_t masking_key[4],
                                           uint8_t *restrict dest,
                                           uint8_t *restrict src, size_t len) {
  enum ws_frame_error_t result = WS_FRAME_SUCCESS;
#if (defined(__i386__) || defined(__x86_64__) || defined(__aarch64__) ||       \
     (defined(__arm__) && defined(__ARM_ARCH_7A__))) &&                        \
    !defined(DISABLE_SIMD)
  result = apply_mask_to_buffer_simd(masking_key, dest, src, len);
#else
  result = apply_mask_to_buffer_serial(masking_key, dest, src, len, 0);
#endif
  return result;
}
