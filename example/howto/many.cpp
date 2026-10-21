/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file many.cpp
 * @brief How-to: walk a run of messages packed one after the other (docs/examples/howto/many.md)
 */

#include <rbe/rbe.hpp>

#include <cstddef>
#include <cstdint>
#include <print>
#include <ranges>
#include <span>
#include <vector>

namespace {

// clang-format off
// --8<-- [start:types]
enum class msg_type : std::uint8_t { order = 1, cancel = 2 };

// every message carries its own length, so a run of them can be walked message by message
struct [[=rbe::pack_be]] MessageHeader {
  [[=rbe::frame_length]] std::uint8_t length;
  [[=rbe::id]]           msg_type     type;
};

struct [[=rbe::pack_be, =rbe::id(msg_type::order)]]  Order  { std::uint32_t order_id; std::uint64_t price; };
struct [[=rbe::pack_be, =rbe::id(msg_type::cancel)]] Cancel { std::uint32_t order_id; };

using message = rbe::frames::frame<MessageHeader, rbe::frames::any<Order, Cancel>>;

// the packet header says how many messages follow
struct [[=rbe::pack_be]] PacketHeader {
  std::uint32_t                        sequence;
  [[=rbe::payload_count]] std::uint8_t count;
};

using packet = rbe::frames::frame<PacketHeader, rbe::frames::many<message>>;
// --8<-- [end:types]
// clang-format on

auto iterate(std::span<std::byte const> const datagram) -> bool {
  // clang-format off
  // --8<-- [start:iterate]
  auto [header, messages] = rbe::deserialize<packet>(datagram);   // a frame decomposes into header and payload

  std::println("packet {}: {} messages", header.field<"sequence">(), header.field<"count">());

  for (auto const [msg_header, payload]: messages) {               // each element is a message frame
    payload.match(
        [](Order const& order) { std::println("  order {} @ {}", order.order_id, order.price); },
        [](Cancel const& cancel) { std::println("  cancel {}", cancel.order_id); },
        [](rbe::unmatched auto) { std::println("  unknown message"); }
    );
  }
  // --8<-- [end:iterate]
  // clang-format on
  return header.field<"count">() == 3;
}

auto filter(std::span<std::byte const> const datagram) -> bool {
  // clang-format off
  // --8<-- [start:filter]
  auto pkt = rbe::deserialize<packet>(datagram);

  // a many is single-pass: iterating it advances it. payload() hands out a fresh one each time, so start
  // every pipeline from it. The adaptors compose with std::views.
  auto cancels = std::ranges::distance(pkt.payload() | rbe::views::with_ids(msg_type::cancel));  // some ids
  auto known   = std::ranges::distance(pkt.payload() | rbe::views::known_ids());                 // ids the code knows
  auto unknown = std::ranges::distance(pkt.payload() | rbe::views::unknown_ids());               // ids it does not

  for (auto const [msg_header, payload]: pkt.payload() | rbe::views::with_ids(msg_type::order)) {
    std::println("  order id {}", payload.as<Order>()->field<"order_id">());
  }
  // --8<-- [end:filter]
  // clang-format on
  return cancels == 1 and known == 3 and unknown == 0;
}

auto stream(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:stream]
  // A run with no packet header around it: pipe the bytes straight into a many over the message view
  auto messages = bytes | rbe::views::many<message::dsrl_type>();

  auto seen = std::size_t {0};
  for (auto const frame: messages) {
    std::println("  {} byte message", frame.length());
    ++seen;
  }

  // iteration stops at the first message that is not whole; the rest is kept for the next read
  bool                       partial = messages.has_seen_partial();
  std::span<std::byte const> rest    = messages.remainder();
  // --8<-- [end:stream]
  // clang-format on
  return seen == 2 and partial and rest.size() == 3;
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
  constexpr std::uint8_t order_size  = rbe::wire_size_of<MessageHeader>() + rbe::wire_size_of<Order>();
  constexpr std::uint8_t cancel_size = rbe::wire_size_of<MessageHeader>() + rbe::wire_size_of<Cancel>();

  auto const datagram = wire(
      PacketHeader {.sequence = 1, .count = 3}, //
      MessageHeader {.length = order_size, .type = msg_type::order}, Order {.order_id = 1, .price = 2500},
      MessageHeader {.length = cancel_size, .type = msg_type::cancel}, Cancel {.order_id = 1},
      MessageHeader {.length = order_size, .type = msg_type::order}, Order {.order_id = 2, .price = 2600}
  );

  // two whole messages and the first 3 bytes of a third
  auto const messages = wire(
      MessageHeader {.length = order_size, .type = msg_type::order}, Order {.order_id = 1, .price = 2500},
      MessageHeader {.length = cancel_size, .type = msg_type::cancel}, Cancel {.order_id = 1},
      MessageHeader {.length = order_size, .type = msg_type::order}, std::uint8_t {0}
  );

  return iterate(datagram) and filter(datagram) and stream(messages) ? 0 : 1;
}
