/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file wide_contracts.cpp
 * @brief How-to: check the buffer instead of trusting it (docs/examples/howto/wide-contracts.md)
 */

#include <rbe/rbe.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace {

// clang-format off
struct [[=rbe::pack_be]] Header {
  [[=rbe::frame_length]] std::uint16_t length;
  [[=rbe::id]]           std::uint8_t  type;
};

struct [[=rbe::pack_be, =rbe::id(std::uint8_t {1})]] Order  { std::uint32_t order_id; std::uint64_t price; };
struct [[=rbe::pack_be, =rbe::id(std::uint8_t {2})]] Cancel { std::uint32_t order_id; };

using message = rbe::frames::frame<Header, rbe::frames::any<Order, Cancel>>;

struct Tick { std::uint64_t timestamp; std::uint32_t price; };
// clang-format on

auto short_buffer(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:try_deserialize]
  // deserialize() assumes the buffer holds a whole Order; try_deserialize() checks it first
  std::optional<Order> order = rbe::try_deserialize<Order>(bytes, rbe::dsrl::eager);

  auto truncated = bytes.first(bytes.size() - 1);
  auto missing   = rbe::try_deserialize<Order>(truncated, rbe::dsrl::eager);  // std::nullopt, one byte short

  // the same for a lazy view, which is only safe to read if the whole value is there
  std::optional<rbe::dsrl::proxy<Order>> view = rbe::try_deserialize<Order>(bytes, rbe::dsrl::lazy);
  // --8<-- [end:try_deserialize]
  // clang-format on
  return order.has_value() and not missing.has_value() and view.has_value();
}

auto misaligned(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:alignment]
  // in_place also needs the buffer aligned for T: try_deserialize checks that too
  auto aligned    = rbe::try_deserialize<Tick>(bytes, rbe::dsrl::in_place);              // a Tick const&
  auto misaligned = rbe::try_deserialize<Tick>(bytes.subspan(1), rbe::dsrl::in_place);   // std::nullopt
  // --8<-- [end:alignment]
  // clang-format on
  return aligned.has_value() and not misaligned.has_value();
}

auto frames(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:frame]
  // A frame checks its header, then that the payload the header announces fits
  std::optional<message::dsrl_type> msg = rbe::try_deserialize<message>(bytes);

  auto cut = rbe::try_deserialize<message>(bytes.first(bytes.size() - 2));  // std::nullopt: the Order is cut
  // --8<-- [end:frame]
  // clang-format on
  return msg.has_value() and msg->payload().is<Order>() and not cut.has_value();
}

auto writing(std::span<std::byte> const out) -> bool {
  // clang-format off
  // --8<-- [start:try_serialize]
  std::optional<std::size_t> written = rbe::try_serialize(out, Order {.order_id = 1, .price = 2500});
  // --8<-- [end:try_serialize]
  // clang-format on
  return written.has_value();
}

} // namespace

auto main() -> int {
  constexpr auto length = rbe::wire_size_of<Header>() + rbe::wire_size_of<Order>();
  auto buffer           = std::array<std::byte, length> {};
  auto out              = std::span<std::byte> {buffer};
  out                   = out.subspan(rbe::serialize(out, Header {.length = length, .type = 1}));
  rbe::serialize(out, Order {.order_id = 7, .price = 2500});

  alignas(Tick) std::array<std::byte, sizeof(Tick) + 1> tick_bytes {};

  return short_buffer(std::span {buffer}.subspan(rbe::wire_size_of<Header>())) and misaligned(tick_bytes) and
                 frames(buffer) and writing(out)
             ? 0
             : 1;
}
