/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file any_match.cpp
 * @brief How-to: pick the message type from an id on the wire (docs/examples/howto/any-match.md)
 */

#include <rbe/rbe.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <print>
#include <span>

namespace {

// clang-format off
// --8<-- [start:types]
enum class msg_type : std::uint8_t { order = 1, cancel = 2, heartbeat = 3 };

// the header says which message follows: rbe::id marks the field
struct [[=rbe::pack_be]] Header {
  [[=rbe::id]] msg_type type;
  std::uint16_t         sequence;
};

// each candidate says which id it answers to: rbe::id(value) on the type
struct [[=rbe::pack_be, =rbe::id(msg_type::order)]]  Order  { std::uint32_t order_id; std::uint64_t price; };
struct [[=rbe::pack_be, =rbe::id(msg_type::cancel)]] Cancel { std::uint32_t order_id; };

// a message with no fields: rbe::empty, zero bytes on the wire
struct [[=rbe::id(msg_type::heartbeat), =rbe::empty]] Heartbeat { };

using message = rbe::frames::frame<Header, rbe::frames::any<Order, Cancel, Heartbeat>>;
// --8<-- [end:types]
// clang-format on

void dispatch(std::span<std::byte const> const bytes) {
  // clang-format off
  // --8<-- [start:match]
  auto msg = rbe::deserialize<message>(bytes);

  msg.payload().match(
      [](Order const& order) {               // by value: the Order is decoded, then handed over
        std::println("order {} @ {}", order.order_id, order.price);
      },
      [](rbe::dsrl::proxy<Cancel> cancel) {  // by proxy: decoded field by field, on access
        std::println("cancel {}", cancel.field<"order_id">());
      },
      [](Heartbeat) { std::println("heartbeat"); },
      [](rbe::unmatched auto other) {        // mandatory: ids the code does not know, or does not handle here
        std::println("id {} {}", std::to_underlying(other.id), other.known_id ? "not handled" : "unknown");
      }
  );
  // --8<-- [end:match]
  // clang-format on
}

auto query(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:query]
  auto payload = rbe::deserialize<message>(bytes).payload();

  bool is_order = payload.is<Order>();
  auto id       = payload.id();              // the id on the wire, as the header's field type
  bool known    = payload.known_id();        // whether one of the candidates answers to it

  std::optional<Order>                   order = payload.as<Order>(rbe::dsrl::eager);  // nullopt unless is<Order>()
  std::optional<rbe::dsrl::proxy<Order>> view  = payload.as<Order>();                  // lazy by default
  // --8<-- [end:query]
  // clang-format on
  return is_order and id == msg_type::order and known and order->price == 2500 and view->field<"price">() == 2500;
}

template<typename Payload>
auto wire(msg_type const type, Payload const& payload) {
  auto bytes = std::array<std::byte, rbe::wire_size_of<Header>() + rbe::wire_size_of<Payload>()> {};
  auto out   = std::span<std::byte> {bytes};
  out        = out.subspan(rbe::serialize(out, Header {.type = type, .sequence = 1}));
  if constexpr (rbe::wirable<Payload>) { // an empty payload has no bytes to write
    rbe::serialize(out, payload);
  }
  return bytes;
}

} // namespace

auto main() -> int {
  auto const order     = wire(msg_type::order, Order {.order_id = 7, .price = 2500});
  auto const cancel    = wire(msg_type::cancel, Cancel {.order_id = 7});
  auto const heartbeat = wire(msg_type::heartbeat, Heartbeat {});
  auto const unknown   = wire(static_cast<msg_type>(9), Cancel {.order_id = 0});

  dispatch(order);
  dispatch(cancel);
  dispatch(heartbeat);
  dispatch(unknown);

  return query(order) ? 0 : 1;
}
