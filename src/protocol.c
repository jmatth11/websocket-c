#include "headers/protocol.h"
#include "unicode_str.h"
#include <stdint.h>
#include <string.h>

#define _BV(x) (1 << (x))
#define _SHIFT_LEN(x, n) (((uint64_t)x) << n)

bool ws_frame_init(struct ws_frame_t *frame) {
  memset(frame, 0, sizeof(struct ws_frame_t));
  return byte_array_init(&frame->payload, 125);
}

static enum ws_frame_error_t ws_frame_extended_len(struct ws_frame_t *frame,
                                                   uint8_t *buf, size_t len, size_t *offset) {
  uint8_t extend_len = 0;
  switch (frame->payload_len) {
  case 126:
    extend_len = 2;
    break;
  case 127:
    extend_len = 8;
    break;
  default:
    break;
  }
  // plus 2 for the initial 2 bytes
  if (len < (extend_len + 2)) {
    return WS_FRAME_ERROR_LEN;
  }
  switch (extend_len) {
  case 2: {
    frame->payload_len = _SHIFT_LEN(buf[3], 0) | _SHIFT_LEN(buf[2], 8);
    break;
  }
  case 8: {
    frame->payload_len = (_SHIFT_LEN(buf[9], 0) | _SHIFT_LEN(buf[8], 8) |
                          _SHIFT_LEN(buf[7], 16) | _SHIFT_LEN(buf[6], 24) |
                          _SHIFT_LEN(buf[5], 32) | _SHIFT_LEN(buf[4], 40) |
                          _SHIFT_LEN(buf[3], 48) | _SHIFT_LEN(buf[2], 56));
    break;
  }
  }
  *offset += extend_len;
  return WS_FRAME_SUCCESS;
}

static enum ws_frame_error_t ws_frame_extract_mask(struct ws_frame_t *frame, uint8_t *buf, size_t len, size_t *offset) {
  if (frame->mask) {
    size_t idx = *offset;
    if (len < (idx + 4)) {
      return WS_FRAME_ERROR_LEN;
    }
    memcpy(frame->masking_key, &buf[idx], 4);
    idx += 4;
    *offset = idx;
  }
  return WS_FRAME_SUCCESS;
}

static enum ws_frame_error_t ws_frame_extract_payload(struct ws_frame_t *frame, uint8_t *buf, size_t len, size_t *offset) {
  if (!frame->mask) {
    for (size_t i = *offset; i < len; ++i) {
      if (!byte_array_insert(&frame->payload, buf[i])) {
        return WS_FRAME_INVALID;
      }
    }
  } else {
    // this is a simple iteration with the mask
    // investigate simd instructions
    for (size_t i = *offset, mask_idx = 0; i < len; ++i, ++mask_idx) {
      // xor byte with mask. iterate through key as we count through the bytes.
      uint8_t val = buf[i] ^ frame->masking_key[mask_idx & 3];
      if (!byte_array_insert(&frame->payload, val)) {
        return WS_FRAME_INVALID;
      }
    }
  }
  *offset = len;
  return WS_FRAME_SUCCESS;
}

enum ws_frame_error_t ws_frame_read(struct ws_frame_t *frame, uint8_t *buf,
                                    size_t len) {
  if (len == 0) {
    return WS_FRAME_ERROR_LEN;
  }
  size_t offset = 2;
  uint8_t byte1 = buf[0];
  uint8_t byte2 = buf[1];
  frame->fin = byte1 & _BV(7);
  frame->rsv1 = byte1 & _BV(6);
  frame->rsv2 = byte1 & _BV(5);
  frame->rsv3 = byte1 & _BV(4);
  frame->opcode = byte1 & 0b00001111;
  frame->mask = byte2 & _BV(7);
  frame->payload_len = byte2 & 0b01111111;
  enum ws_frame_error_t err = ws_frame_extended_len(frame, buf, len, &offset);
  if (err != WS_FRAME_SUCCESS) {
    return err;
  }
  err = ws_frame_extract_mask(frame, buf, len, &offset);
  if (err != WS_FRAME_SUCCESS) {
    return err;
  }
  return ws_frame_extract_payload(frame, buf, len, &offset);
}

void ws_frame_free(struct ws_frame_t *frame) {
  memset(frame, 0, sizeof(struct ws_frame_t));
  byte_array_free(&frame->payload);
}
