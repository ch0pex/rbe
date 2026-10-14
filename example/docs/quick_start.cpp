/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file quick_start.cpp
 * @brief Full example (annotations, serde modes, framing) mirroring the README, for the Quick start page
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

  // Serialize to raw buffer
  auto size = rbe::serialize(buffer, AddOrder{});

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
  (void)size, (void)price, (void)timestamp, (void)tick;
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

auto main() -> int {
  example::serde();

  // One packet holding a single AddOrder; the run of messages ends with the buffer
  auto constexpr message_size = rbe::wire_size_of<MessageHeader>() + rbe::wire_size_of<AddOrder>();
  auto datagram               = std::vector<std::byte>(rbe::wire_size_of<PacketHeader>() + message_size);
  auto out                    = std::span {datagram};
  auto header                 = PacketHeader {};
  header.sequence             = 7;
  out                         = out.subspan(rbe::serialize(out, header));
  out = out.subspan(rbe::serialize(out, MessageHeader {.length = message_size, .message_type = msg_type::add_order}));
  rbe::serialize(out, AddOrder {});

  return example::frames(datagram) ? 0 : 1;
}
