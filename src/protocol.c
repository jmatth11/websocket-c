#include "headers/protocol.h"
#include "unicode_str.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define _SHIFT_LEN(x, n) (((uint64_t)x) << n)

bool ws_generate_mask(uint8_t *mask, size_t len) {
  // TODO change this to a better rng
  srand(time(NULL));
  size_t max = UINT8_MAX + 1;
  for (size_t i = 0; i < len; ++i) {
    mask[i] = (rand() % max);
  }
  return true;
}

bool ws_frame_init(struct ws_frame_t *frame) {
  memset(frame, 0, sizeof(struct ws_frame_t));
  return true;
}

static enum ws_frame_error_t ws_frame_extended_len(struct ws_frame_t *frame,
                                                   uint8_t *buf, size_t len,
                                                   size_t *offset) {
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

static enum ws_frame_error_t
ws_frame_write_len(struct ws_frame_t *frame, byte_array *out, size_t *offset) {
  size_t idx = *offset;
  switch (frame->info.flags.payload_len) {
  case 126: {
    out->byte_data[idx] = (frame->payload.len >> 8) & 0xFF;
    out->byte_data[idx + 1] = (frame->payload.len) & 0xFF;
    *offset = idx + 2;
    break;
  }
  case 127: {
    out->byte_data[idx] = (frame->payload.len >> 56) & 0xFF;
    out->byte_data[idx + 1] = (frame->payload.len >> 48) & 0xFF;
    out->byte_data[idx + 2] = (frame->payload.len >> 40) & 0xFF;
    out->byte_data[idx + 3] = (frame->payload.len >> 32) & 0xFF;
    out->byte_data[idx + 4] = (frame->payload.len >> 24) & 0xFF;
    out->byte_data[idx + 5] = (frame->payload.len >> 16) & 0xFF;
    out->byte_data[idx + 6] = (frame->payload.len >> 8) & 0xFF;
    out->byte_data[idx + 7] = (frame->payload.len) & 0xFF;
    *offset = idx + 8;
    break;
  }
  }
  return WS_FRAME_SUCCESS;
}

static enum ws_frame_error_t ws_frame_extract_mask(struct ws_frame_t *frame,
                                                   uint8_t *buf, size_t len,
                                                   size_t *offset) {
  if (frame->info.flags.mask) {
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

/**
 * Handle transferring payload from src to dest and apply a masking key if
 * supplied one.
 */
static enum ws_frame_error_t ws_frame_handle_payload(bool mask,
                                                     uint8_t masking_key[4],
                                                     uint8_t *restrict dest,
                                                     uint8_t *restrict src,
                                                     size_t len) {
  if (!mask) {
    for (size_t i = 0; i < len; ++i) {
      dest[i] = src[i];
    }
  } else {
    // this is a simple iteration with the mask
    // investigate simd instructions
    for (size_t i = 0; i < len; ++i) {
      // xor byte with mask. iterate through key as we count through the bytes.
      dest[i] = src[i] ^ masking_key[i & 3];
    }
  }
  return WS_FRAME_SUCCESS;
}

size_t ws_frame_output_size(struct ws_frame_t *frame) {
  // 2 comes from the first two bytes
  size_t out_len = 2 + frame->payload.len;
  if (frame->info.flags.mask) {
    out_len += 4;
  }
  if (frame->payload.len <= 125) {
    return out_len;
  } else if (frame->payload.len <= 65536) {
    return out_len + 2;
  }
  return out_len + 8;
}

uint8_t ws_frame_payload_byte_len(struct ws_frame_t *frame) {
  switch (frame->info.flags.payload_len) {
  case 126:
    return 2;
  case 127:
    return 8;
  }
  return 0;
}

enum ws_frame_error_t ws_frame_read(struct ws_frame_t *frame, uint8_t *buf,
                                    size_t len) {
  if (len == 0) {
    return WS_FRAME_ERROR_LEN;
  }
  enum ws_frame_error_t err = ws_frame_read_header(frame, buf, len);
  if (err != WS_FRAME_SUCCESS) {
    return err;
  }
  return ws_frame_read_body(frame, &buf[2], len - 2);
}

enum ws_frame_error_t ws_frame_read_header(struct ws_frame_t *frame,
                                           uint8_t *buf, size_t len) {
  if (len < 2) {
    return WS_FRAME_ERROR_LEN;
  }
  frame->codes.value = buf[0];
  frame->info.value = buf[1];
  frame->payload_len = frame->info.flags.payload_len;
  size_t offset = 2;
  enum ws_frame_error_t err = ws_frame_extended_len(frame, buf, len, &offset);
  if (err != WS_FRAME_SUCCESS) {
    fprintf(stderr, "extended len failed.\n");
    return err;
  }
  return WS_FRAME_SUCCESS;
}

enum ws_frame_error_t ws_frame_read_body(struct ws_frame_t *frame, uint8_t *buf,
                                         size_t len) {
  if (len == 0) {
    return WS_FRAME_ERROR_LEN;
  }
  size_t offset = 0;
  enum ws_frame_error_t err = ws_frame_extract_mask(frame, buf, len, &offset);
  if (err != WS_FRAME_SUCCESS) {
    fprintf(stderr, "mask failed.\n");
    return err;
  }
  if (len < (offset + frame->payload_len)) {
    fprintf(stderr, "payload len failed: %zu\n", (offset + frame->payload_len));
    return WS_FRAME_ERROR_LEN;
  }
  if (!byte_array_init(&frame->payload, frame->payload_len)) {
    return WS_FRAME_MALLOC_ERROR;
  }
  frame->payload.len = frame->payload_len;
  return ws_frame_handle_payload(frame->info.flags.mask, frame->masking_key,
                                 frame->payload.byte_data, &buf[offset],
                                 frame->payload_len);
}

enum ws_frame_error_t ws_frame_write(struct ws_frame_t *frame,
                                     byte_array *out) {
  size_t out_len = ws_frame_output_size(frame);
  if (!byte_array_init(out, out_len)) {
    return WS_FRAME_MALLOC_ERROR;
  }
  out->len = out_len;
  size_t offset = 2;
  out->byte_data[0] = frame->codes.value;
  if (frame->payload.len <= 125) {
    frame->info.flags.payload_len = frame->payload.len;
  } else if (frame->payload.len < 65536) {
    frame->info.flags.payload_len = 126;
  } else {
    frame->info.flags.payload_len = 127;
  }
  out->byte_data[1] = frame->info.value;
  enum ws_frame_error_t err = ws_frame_write_len(frame, out, &offset);
  if (err != WS_FRAME_SUCCESS) {
    byte_array_free(out);
    return err;
  }
  if (frame->info.flags.mask) {
    out->byte_data[offset] = frame->masking_key[0];
    out->byte_data[offset + 1] = frame->masking_key[1];
    out->byte_data[offset + 2] = frame->masking_key[2];
    out->byte_data[offset + 3] = frame->masking_key[3];
    offset += 4;
  }
  // ensure property matches actual length
  frame->payload_len = frame->payload.len;
  if (out->len < (offset + frame->payload_len)) {
    byte_array_free(out);
    return WS_FRAME_ERROR_LEN;
  }
  return ws_frame_handle_payload(frame->info.flags.mask, frame->masking_key,
                                 &out->byte_data[offset],
                                 frame->payload.byte_data, frame->payload.len);
}

void ws_frame_free(struct ws_frame_t *frame) {
  memset(frame, 0, sizeof(struct ws_frame_t));
  byte_array_free(&frame->payload);
}

void ws_frame_print(struct ws_frame_t *frame) {
  printf("---frame:\n");
  printf("fin:%d\n", frame->codes.flags.fin);
  printf("rsv1:%d\n", frame->codes.flags.rsv1);
  printf("rsv2:%d\n", frame->codes.flags.rsv2);
  printf("rsv3:%d\n", frame->codes.flags.rsv3);
  printf("opcode:%d\n", frame->codes.flags.opcode);
  printf("mask:%d\n", frame->info.flags.mask);
  printf("payload_len:%d\n", frame->info.flags.payload_len);
  printf("---end frame:\n");
}
