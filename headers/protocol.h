#ifndef CSTD_WS_PROTOCOL_H
#define CSTD_WS_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "unicode_str.h"
#include "defs.h"

#define FIRST_BIT(x) ((x) & (1<<7))

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

struct __attribute__((packed)) ws_frame_t  {
  /**
   * Final frame flag.
   */
  uint8_t fin : 1;
  /**
   * Reserved bits, must be zero unless an extension is negotiated.
   * If a nonzero value is received with no extensions the endpoint MUST fail.
   */
  uint8_t res1 : 1;
  uint8_t res2 : 1;
  uint8_t res3 : 1;
  /**
   * Opcode for frame.
   * Unknown opcodes cause WebSocket failure.
   */
  enum ws_opcode_t opcode : 4;
  /**
   * Masking key flag.
   * All frames from client to server must have mask flag set.
   */
  uint8_t mask : 1;
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
  uint32_t masking_key;
  /**
   * The frame payload.
   */
  byte_array payload;
};

/**
 * Initialize WebSocket Frame structure.
 *
 * @param[in] frame The WebSocket frame structure.
 * @return True on success, False otherwise.
 */
bool ws_frame_init(struct ws_frame_t *frame) __nonnull((1));

/**
 * Read WebSocket frame.
 *
 * @param[in] frame The WebSocket frame structure.
 * @param[in] buf The buffered data.
 * @param[in] len The buffered data length.
 * @return True on success, False otherwise.
 */
bool ws_frame_read(struct ws_frame_t *frame, uint8_t *buf, size_t len) __nonnull((1, 2));

/**
 * Free internals of WebSocket Frame structure.
 *
 * @param[in] frame The WebSocket frame structure.
 */
void ws_frame_free(struct ws_frame_t *frame) __nonnull((1));

#endif
