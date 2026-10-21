/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file flatten.cpp
 * @brief How-to: reach every header of a nested frame at once (docs/examples/howto/flatten.md)
 */

#include <rbe/rbe.hpp>

#include <cstddef>
#include <cstdint>
#include <print>
#include <span>
#include <vector>

namespace {

// clang-format off
// --8<-- [start:types]
enum class msg_type : std::uint8_t { order = 1, cancel = 2 };

// transport: a length-prefixed block...
struct [[=rbe::pack_be]] TransportHeader {
  [[=rbe::payload_length]] std::uint16_t length;
  std::uint32_t                          session;
};

// ...holding one message, whose own header picks the type
struct [[=rbe::pack_be]] MessageHeader {
  [[=rbe::id]] msg_type type;
  std::uint32_t         sequence;
};

struct [[=rbe::pack_be, =rbe::id(msg_type::order)]]  Order  { std::uint32_t order_id; std::uint64_t price; };
struct [[=rbe::pack_be, =rbe::id(msg_type::cancel)]] Cancel { std::uint32_t order_id; };

using message = rbe::frames::frame<MessageHeader, rbe::frames::any<Order, Cancel>>;
using block   = rbe::frames::frame<TransportHeader, message>;   // a frame whose payload is a frame
// --8<-- [end:types]
// clang-format on

auto nested(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:nested]
  auto blk = rbe::deserialize<block>(bytes);

  auto session  = blk.header().field<"session">();
  auto sequence = blk.payload().header().field<"sequence">();   // one level down
  auto payload  = blk.payload().payload();                      // two levels down: the any
  // --8<-- [end:nested]
  // clang-format on
  return session == 9 and sequence == 1 and payload.is<Order>();
}

auto flat(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:flatten]
  auto blk = rbe::deserialize<block>(bytes);

  // every header on the way down, then the innermost payload
  auto const [transport, header, payload] = blk.flatten();

  std::println("session {} sequence {}", transport.field<"session">(), header.field<"sequence">());
  payload.match(
      [](Order const& order) { std::println("order {} @ {}", order.order_id, order.price); },
      [](Cancel const& cancel) { std::println("cancel {}", cancel.order_id); },
      [](rbe::unmatched auto) { std::println("unknown"); }
  );

  // the block knows its extent: the transport header declares the payload length
  std::size_t length = blk.length();                    // header plus payload
  std::size_t same   = rbe::frames::length(blk);        // the function object, for generic code
  // --8<-- [end:flatten]
  // clang-format on
  return length == bytes.size() and same == length;
}

template<typename... Ts>
auto wire(Ts const&... parts) -> std::vector<std::byte> {
  auto out    = std::vector<std::byte>((rbe::wire_size_of<Ts>() + ...));
  auto offset = std::size_t {0};
  ((offset += rbe::serialize(std::span {out}.subspan(offset), parts)), ...);
  return out;
}

} // namespace

auto main() -> int {
  constexpr std::uint16_t payload_length = rbe::wire_size_of<MessageHeader>() + rbe::wire_size_of<Order>();

  auto const bytes = wire(
      TransportHeader {.length = payload_length, .session = 9}, //
      MessageHeader {.type = msg_type::order, .sequence = 1}, //
      Order {.order_id = 7, .price = 2500}
  );

  return nested(bytes) and flat(bytes) ? 0 : 1;
}
