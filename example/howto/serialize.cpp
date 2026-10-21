/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file serialize.cpp
 * @brief How-to: write a value into a byte buffer (docs/examples/howto/serialize.md)
 */

#include <rbe/annotations.hpp>
#include <rbe/srl.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace {

// clang-format off
struct [[=rbe::pack_be]] Header {
  std::uint16_t length;
  std::uint8_t  type;
};

struct [[=rbe::pack_be]] Order {
  std::uint32_t order_id;
  std::uint32_t quantity;
  std::uint64_t price;
};
// clang-format on

auto fixed_buffer() -> bool {
  // clang-format off
  // --8<-- [start:array]
  // wire_size_of<T>() is a constant expression: size the buffer exactly
  std::array<std::byte, rbe::wire_size_of<Order>()> buffer {};

  std::size_t written = rbe::serialize(buffer, Order {.order_id = 1, .quantity = 100, .price = 2500});
  // --8<-- [end:array]
  // clang-format on
  return written == buffer.size();
}

auto growing_buffer() -> bool {
  // clang-format off
  // --8<-- [start:vector]
  // Any contiguous range of std::byte converts to the std::span<std::byte> the call takes
  std::vector<std::byte> buffer(rbe::wire_size_of<Header>() + rbe::wire_size_of<Order>());

  std::size_t written = rbe::serialize(buffer, Header {.length = 19, .type = 1});
  // --8<-- [end:vector]
  // clang-format on
  return written == rbe::wire_size_of<Header>();
}

auto checked() -> bool {
  // clang-format off
  // --8<-- [start:try]
  std::array<std::byte, 8> too_small {};

  // serialize() assumes the buffer is big enough; try_serialize() checks and reports
  std::optional<std::size_t> written = rbe::try_serialize(too_small, Order {});
  if (not written) {
    // the buffer holds 8 bytes and Order needs 16: nothing was written
  }
  // --8<-- [end:try]
  // clang-format on
  return not written.has_value();
}

auto back_to_back() -> bool {
  // clang-format off
  // --8<-- [start:compose]
  // A header followed by its payload: write each part where the previous one ended
  std::vector<std::byte> datagram(rbe::wire_size_of<Header>() + rbe::wire_size_of<Order>());
  std::span<std::byte>   out = datagram;

  out = out.subspan(rbe::serialize(out, Header {.length = static_cast<std::uint16_t>(datagram.size()), .type = 1}));
  out = out.subspan(rbe::serialize(out, Order {.order_id = 1, .quantity = 100, .price = 2500}));
  // --8<-- [end:compose]
  // clang-format on
  return out.empty();
}

} // namespace

auto main() -> int { return fixed_buffer() and growing_buffer() and checked() and back_to_back() ? 0 : 1; }
