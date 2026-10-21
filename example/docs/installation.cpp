/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file installation.cpp
 * @brief The smallest program that proves the toolchain works, included by docs/getting-started/installation.md
 */

// clang-format off
// --8<-- [start:verify]
#include <rbe/rbe.hpp>

#include <array>
#include <print>

struct [[=rbe::pack_be]] Header {
  std::uint16_t length;
  std::uint8_t  type;
};

auto main() -> int {
  std::array<std::byte, rbe::wire_size_of<Header>()> buffer {};
  rbe::serialize(buffer, Header {.length = 3, .type = 1});

  std::println("{} bytes on the wire, {} in memory", buffer.size(), sizeof(Header)); // 3 bytes on the wire, 4 in memory
}
// --8<-- [end:verify]
// clang-format on
