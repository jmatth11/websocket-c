#ifndef CSTD_WS_PROTOCOL_H
#define CSTD_WS_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "unicode_str.h"
#include "defs.h"

__BEGIN_DECLS

enum ws_frame_error_t {
  WS_FRAME_SUCCESS = 0,
  WS_FRAME_INVALID,
  WS_FRAME_ERROR_LEN,
  WS_FRAME_ERROR_RSV_SET,
  WS_FRAME_ERROR_OPCODE,
  WS_FRAME_MALLOC_ERROR,
};

enum ws_opcode_t {
  /**
   * Continuation frame.
   */
  OPCODE_CONT = 0x0,
  /**
   * Text frame.
   */
  OPCODE_TEXT = 0x1,
  /**
   * Binary frame.
   */
  OPCODE_BIN = 0x2,
  /**
   * Non-control reserved op codes.
   */
  OPCODE_NC_RES1 = 0x3,
  OPCODE_NC_RES2 = 0x4,
  OPCODE_NC_RES3 = 0x5,
  OPCODE_NC_RES4 = 0x6,
  OPCODE_NC_RES5 = 0x7,
  /**
   * Denotes a connection close.
   */
  OPCODE_CLOSE = 0x8,
  /**
   * Denotes a ping.
   */
  OPCODE_PING = 0x9,
  /**
   * Denotes a pong.
   */
  OPCODE_PONG = 0xA,
  /**
   * Control reserved op codes.
   */
  OPCODE_C_RES1 = 0xB,
  OPCODE_C_RES2 = 0xC,
  OPCODE_C_RES3 = 0xD,
  OPCODE_C_RES4 = 0xE,
  OPCODE_C_RES5 = 0xF,
};

/**
 * WebSocket Frame structure.
 */
struct ws_frame_t {
  /**
   * The frame specific codes.
   */
  union {
    uint8_t value;
    struct {
      /**
       * Opcode for frame.
       * Unknown opcodes cause WebSocket failure.
       */
      enum ws_opcode_t opcode : 4;
      /**
       * Reserved bits, must be zero unless an extension is negotiated.
       * If a nonzero value is received with no extensions the endpoint MUST
       * fail.
       *
       * rsv1 is for compression.
       * rsv2 and rsv3 currently should never be set.
       */
      uint8_t rsv3 : 1;
      uint8_t rsv2 : 1;
      uint8_t rsv1 : 1;
      /**
       * Final frame flag.
       */
      uint8_t fin : 1;
    } flags;
  } codes;

  /**
   * The payload information.
   */
  union {
    uint8_t value;
    struct {
      /**
       * Payload Length.
       * 3 scenarios.
       * - 0-125 (length is as is)
       * - 126 (length is 16 bits)
       * - 127 (length is 64 bits)
       */
      uint8_t payload_len : 7;
      /**
       * Masking key flag.
       * All frames from client to server must have mask flag set.
       */
      uint8_t mask : 1;
    } flags;
  } info;
  /**
   * Payload Length.
   * 3 scenarios.
   * - 0-125 (length is as is)
   * - 126 (length is 16 bits)
   * - 127 (length is 64 bits)
   */
  uint64_t payload_len;
  /**
   * Masking key used with payload, if mask flag was set.
   */
  uint8_t masking_key[4];
  /**
   * The frame payload.
   */
  byte_array payload;
};

/**
 * Generate a mask and set it to the given uint8_t array.
 *
 * @param[in/out] mask The mask array to populate.
 * @param[in] len The length of the mask.
 * @return True for success, false otherwise.
 */
bool ws_generate_mask(uint8_t *mask, size_t len) __nonnull((1));

/**
 * Initialize WebSocket Frame structure.
 *
 * @param[in] frame The WebSocket frame structure.
 * @return True on success, False otherwise.
 */
bool ws_frame_init(struct ws_frame_t *frame) __nonnull((1));

/**
 * Get the calculated output buffer size of the frame.
 *
 * @param[in] frame The WebSocket frame.
 * @return The output buffer size.
 */
size_t ws_frame_output_size(struct ws_frame_t *frame) __nonnull((1));

/**
 * Get the byte length of the header payload value.
 *
 * @param[in] frame The WebSocket frame.
 * @return The byte length.
 */
uint8_t ws_frame_payload_byte_len(struct ws_frame_t *frame) __nonnull((1));

/**
 * Read WebSocket frame.
 *
 * @param[in] frame The WebSocket frame structure.
 * @param[in] buf The buffered data.
 * @param[in] len The buffered data length.
 * @return True on success, False otherwise.
 */
enum ws_frame_error_t ws_frame_read(struct ws_frame_t *frame, uint8_t *buf,
                                    size_t len) __nonnull((1, 2));

/**
 * Read the header from the given buffer into the WebSocket frame.
 *
 * @param[in/out] frame The WebSocket frame to populate.
 * @param[in] buf The raw buffer of a WebSocket frame.
 * @param[in] len The length of the given buffer.
 * @return The ws_frame_error_t enum result, WS_FRAME_SUCCESS for success.
 */
enum ws_frame_error_t ws_frame_read_header(struct ws_frame_t *frame, uint8_t *buf,
                                      size_t len) __nonnull((1, 2));
/**
 * Read the body from the given buffer into the WebSocket frame.
 *
 * @param[in/out] frame The WebSocket frame to populate.
 * @param[in] buf The raw buffer of a WebSocket frame.
 * @param[in] len The length of the given buffer.
 * @return The ws_frame_error_t enum result, WS_FRAME_SUCCESS for success.
 */
enum ws_frame_error_t ws_frame_read_body(struct ws_frame_t *frame, uint8_t *buf,
                                      size_t len) __nonnull((1, 2));
/**
 * Write WebSocket frame.
 *
 * @param[in] frame The WebSocket frame structure.
 * @param[out] out The byte array to populate.
 * @return True on success, False otherwise.
 */
enum ws_frame_error_t ws_frame_write(struct ws_frame_t *frame, byte_array *out)
    __nonnull((1, 2));

/**
 * Free internals of WebSocket Frame structure.
 *
 * @param[in] frame The WebSocket frame structure.
 */
void ws_frame_free(struct ws_frame_t *frame);

/**
 * Print the given frame for debugging purposes.
 *
 * @param[in] frame The WebSocket frame.
 */
void ws_frame_print(struct ws_frame_t *frame) __nonnull((1));

__END_DECLS

#endif
