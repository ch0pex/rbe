/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file common_serde.hpp
 * @version 1.0
 * @date 30/07/2026
 * @brief Short description
 *
 * Longer description
 */
#pragma once

#include "common_structs.hpp"

#include <rbe/core/concepts.hpp>

#include <array>

inline constexpr auto pad = std::byte {0xCC};

template<typename... Args>
constexpr std::array<std::byte, sizeof...(Args)> bytes(Args... args) {
  return std::array {static_cast<std::byte>(args)...};
}

template<rbe::wirable T, std::size_t N>
struct TestCase {
  T structure;
  std::array<std::byte, N> wire;
};


constexpr TestCase trivially_wirable_no_padding {
  .structure = NonPaddedStruct {.a = 1, .b = 2, .c = 'a'},
  .wire      = bytes(
      0x01, 0x00, 0x00, 0x00, // a
      0x02, 0x00, 0x00, 0x00, // b
      0x61, 0x00, 0x00, 0x00, // c
      0x00, 0x00, 0x00, 0x00 // padding
  ),
};

constexpr TestCase trivially_wirable_with_paddings {
  .structure =
      ReduceSize {
        .length           = 0xAA,
        .message_type     = 0xBB,
        .time_offset      = 0x00112233,
        .order_id         = 0x0011223344556677,
        .cancelled_shares = 0x00112233,
      },
  .wire = bytes(
      0xAA, // length
      0xBB, // Msg type
      pad, pad, // padding
      0x33, 0x22, 0x11, 0x00, // time_offset
      0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, // order_id
      0x33, 0x22, 0x11, 0x00, // cancelled shares
      pad, pad, pad, pad // padding
  ),
};
