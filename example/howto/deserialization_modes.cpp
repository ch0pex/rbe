/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file deserialization_modes.cpp
 * @brief How-to: read the same bytes eagerly, lazily or in place (docs/examples/howto/deserialization-modes.md)
 */

#include <rbe/annotations.hpp>
#include <rbe/dsrl.hpp>
#include <rbe/srl.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace {

// clang-format off
// --8<-- [start:types]
// Big-endian on the wire, so every integer is byte-swapped on a little-endian host: a conversion per field
struct [[=rbe::pack_be]] Trade {
  std::uint32_t trade_id;
  std::uint32_t quantity;
  std::uint64_t price;
  std::uint64_t timestamp;
};

// No annotations: the wire layout is the in-memory layout, so there is nothing to convert
struct Tick {
  std::uint64_t timestamp;
  std::uint32_t price;
  std::uint32_t quantity;
};

static_assert(not rbe::trivially_wirable<Trade>);
static_assert(rbe::trivially_wirable<Tick>);
// --8<-- [end:types]
// clang-format on

auto eager(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:eager]
  // Every field is decoded now. The result owns its data: the buffer can be reused or freed afterwards.
  Trade trade = rbe::deserialize<Trade>(bytes, rbe::dsrl::eager);
  // --8<-- [end:eager]
  // clang-format on
  return trade.price == 2500;
}

auto lazy(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:lazy]
  // Nothing is decoded until a field is read. The view borrows the buffer, which must outlive it.
  rbe::dsrl::proxy<Trade> view = rbe::deserialize<Trade>(bytes, rbe::dsrl::lazy);

  auto price = view.field<"price">();   // this field only, converted on access
  // --8<-- [end:lazy]
  // clang-format on
  return price == 2500;
}

auto in_place(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:in_place]
  // The buffer *is* the object: no copy, no conversion. Only for trivially wirable types, and the buffer
  // must be aligned for T (alignas(Tick) below) or the behavior is undefined.
  Tick const& tick = rbe::deserialize<Tick>(bytes, rbe::dsrl::in_place);
  // --8<-- [end:in_place]
  // clang-format on
  return tick.price == 100;
}

auto in_place_mut(std::span<std::byte> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:in_place_mut]
  // Same as in_place over a mutable buffer: writing through the reference writes the buffer
  Tick& tick = rbe::deserialize<Tick>(bytes, rbe::dsrl::in_place_mut);
  tick.quantity += 1;
  // --8<-- [end:in_place_mut]
  // clang-format on
  return rbe::deserialize<Tick>(bytes, rbe::dsrl::eager).quantity == 11;
}

} // namespace

auto main() -> int {
  std::array<std::byte, rbe::wire_size_of<Trade>()> trade_bytes {};
  rbe::serialize(trade_bytes, Trade {.trade_id = 1, .quantity = 10, .price = 2500, .timestamp = 0});

  alignas(Tick) std::array<std::byte, rbe::wire_size_of<Tick>()> tick_bytes {};
  rbe::serialize(tick_bytes, Tick {.timestamp = 0, .price = 100, .quantity = 10});

  return eager(trade_bytes) and lazy(trade_bytes) and in_place(tick_bytes) and in_place_mut(tick_bytes) ? 0 : 1;
}
