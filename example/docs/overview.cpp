/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file overview.cpp
 * @brief The README tour (annotations, serde modes, framing), included by docs/getting-started/overview.md
 */

// clang-format off
// --8<-- [start:types]
#include <rbe/annotations.hpp>

enum class msg_type : std::uint8_t { add_order = 0x21, reduce_size = 0x25, tick = 0x2A };

// struct will travel packed and big-endian over the wire
struct [[=rbe::pack, =rbe::big]] PacketHeader {
    std::uint16_t length;
    std::uint8_t  count;
    std::uint8_t  unit;
    std::uint32_t sequence;
};

// =rbe::pack_be is a preset for `=rbe::pack, =rbe::big`
struct [[=rbe::pack_be]] MessageHeader {
    [[=rbe::frame_length]] std::uint8_t length;
    [[=rbe::id]]           msg_type     message_type;
};

struct [[=rbe::pack_be, =rbe::id(msg_type::add_order)]] AddOrder {
    std::uint32_t time_offset;
    std::uint32_t order_id;
    std::uint8_t  side_indicator;
    std::uint32_t quantity;
    std::uint64_t symbol;
    std::uint32_t price;
};

struct [[=rbe::pack_be, =rbe::id(msg_type::reduce_size)]] ReduceSize {
    std::uint32_t time_offset;
    std::uint64_t order_id;
    std::uint32_t cancelled_shares;
};

// trivially_wirable example, wire and host representation are identical
struct [[=rbe::id(msg_type::tick)]] Tick {
    std::uint64_t timestamp;
    std::uint32_t price;
    std::uint32_t quantity;
};
// --8<-- [end:types]
// clang-format on

#include <rbe/frames.hpp>
#include <rbe/rbe.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace example {

void serde() {
  // clang-format off
  // --8<-- [start:serde]
  alignas(Tick) std::array<std::byte, 1500> buffer{};

  // Serialize to raw buffer: returns the number of bytes written
  auto size = rbe::serialize(buffer, AddOrder{});

  // Wider contract: std::nullopt instead of undefined behavior when the buffer is too small
  auto checked = rbe::try_serialize(std::span{buffer}.first(4), AddOrder{});

  // Eager: decode every field up front, get the struct back
  auto order = rbe::deserialize<AddOrder>(buffer, rbe::dsrl::eager);
  auto price = order.price;

  // Lazy: get a proxy over the buffer, each field decoded on access
  auto view      = rbe::deserialize<AddOrder>(buffer, rbe::dsrl::lazy);
  auto timestamp = view.field<"time_offset">(); // ✅ correct
  // auto invalid   = view.field<"invalid">(); // ❌ compile-time error: no such field

  // In-place: type pun the buffer into a struct, no decoding at all.
  // The type must be `trivially_wirable` and the buffer must meet alignment requirements.
  auto& tick = rbe::deserialize<Tick>(buffer, rbe::dsrl::in_place);
  // --8<-- [end:serde]
  // clang-format on
  (void)size, (void)checked, (void)price, (void)timestamp, (void)tick;
}

// clang-format off
// --8<-- [start:frame_type]
using packet = rbe::frames::frame<
    PacketHeader,                                         // packet header
    rbe::frames::many<                                    // ...followed by a run of messages
        rbe::frames::frame<
            MessageHeader,                                // message header
            rbe::frames::any<AddOrder, ReduceSize, Tick>  // one of these
        >
    >
>;
// --8<-- [end:frame_type]
// clang-format on

auto frames(std::span<std::byte const> buffer) -> bool {
  // clang-format off
  // --8<-- [start:frame_read]
  // Read it back as a view over the buffer
  auto packet_view = rbe::deserialize<packet>(buffer);
  auto sequence    = packet_view.header().field<"sequence">();

  for (auto [hdr, payload] : packet_view.payload()) {
    payload.match( // match supports both eager and lazy dispatch
      [](AddOrder const&) { /* ... */ },
      [](ReduceSize const&) { /* ... */ },
      [](rbe::dsrl::proxy<Tick>) { /* ... */ },
      [](rbe::unmatched auto) { /* unknown message type, handle error */ }
    );
  }
  // --8<-- [end:frame_read]
  // clang-format on
  return sequence == 7;
}

} // namespace example

namespace example {

auto write() -> std::vector<std::byte> {
  // clang-format off
  // --8<-- [start:frame_write]
  // One packet holding a single AddOrder: the parts are written one after the other
  constexpr auto message_size = rbe::wire_size_of<MessageHeader>() + rbe::wire_size_of<AddOrder>();

  auto datagram = std::vector<std::byte>(rbe::wire_size_of<PacketHeader>() + message_size);
  auto out      = std::span {datagram};

  out = out.subspan(rbe::serialize(out, PacketHeader {.length = 0, .count = 1, .unit = 1, .sequence = 7}));
  out = out.subspan(rbe::serialize(out, MessageHeader {.length = message_size, .message_type = msg_type::add_order}));
  out = out.subspan(rbe::serialize(out, AddOrder {.time_offset = 1, .order_id = 42, .side_indicator = 'B',
                                                  .quantity = 100, .symbol = 0x4143'4d45, .price = 2500}));
  // --8<-- [end:frame_write]
  // clang-format on
  return datagram;
}

} // namespace example

auto main() -> int {
  example::serde();
  return example::frames(example::write()) ? 0 : 1;
}
