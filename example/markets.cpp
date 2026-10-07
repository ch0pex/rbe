/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file markets.cpp
 * @date 06/08/2026
 * @brief Central include point aggregating the market data protocol headers
 */

// --- Includes ---

#include "markets/aquis.hpp"
#include "markets/cboe.hpp"
#include "markets/london.hpp"
#include "markets/nasdaq.hpp"
#include "markets/opra.hpp"

#include <rbe/framing/dsrl/views.hpp>

#include <cstddef>
#include <array>
#include <print>
#include <string_view>
#include <utility>
#include <span>
#include <vector>

// --- STD ---

// --- System ---

// Every protocol composes into frames, classified at compile time by how they are delimited.
static_assert(rbe::self_delimiting_frame<aquis::message>);
static_assert(rbe::buffer_delimited_frame<aquis::packet>); // PacketHeader has no length: the UDP datagram bounds it
static_assert(rbe::self_delimiting_frame<cboe::pitch::message>);
static_assert(rbe::self_delimiting_frame<cboe::pitch::packet>);
static_assert(rbe::self_delimiting_frame<lse::message>);
static_assert(rbe::self_delimiting_frame<cboe::top::message>); // the any alternatives imply their length
static_assert(rbe::dispatch_delimited_frame<cboe::top::message>);
// A custom frame built on base_frame is a frame like any other: it composes with many and classifies.
static_assert(rbe::dsrl::is_frame<cboe::top::line_view>);
static_assert(rbe::frame_serder<cboe::top::line>);
static_assert(rbe::self_delimiting_frame<cboe::top::line>);
static_assert(rbe::frame_serder_payload<cboe::top::lines>);
static_assert(rbe::self_delimiting_frame<lse::packet>);
static_assert(rbe::self_delimiting_frame<nasdaq::message>); // the any alternatives imply their length
static_assert(rbe::dispatch_delimited_frame<nasdaq::message>);
static_assert(rbe::self_delimiting_frame<nasdaq::packet>);
static_assert(rbe::self_delimiting_frame<opra::message>); // the any alternatives imply their length
static_assert(rbe::dispatch_delimited_frame<opra::message>);

// --- Usage ---

namespace {

/// Serializes the parts back to back: what a feed handler would find in a datagram.
template<typename... Ts>
auto wire(Ts const&... parts) -> std::vector<std::byte> {
  auto out    = std::vector<std::byte>((rbe::wire_size_of<Ts>() + ...));
  auto offset = std::size_t {0};
  ((offset += rbe::serialize(std::span {out}.subspan(offset), parts)), ...);
  return out;
}

/// Aquis: a UDP payload is a `PacketHeader` followed by `count` messages. The packet is parsed once and its
/// payload is a `many`: iterate it, and let every frame dispatch on the id its header carries.
void aquis_packet() {
  using namespace aquis;
  constexpr auto header_size = rbe::wire_size_of<Header>();

  auto const datagram = wire(
      PacketHeader {.count = 3}, //
      Header {.msg_type = message_type_t::heartbeat, .length = header_size, .seq_no = 1},
      Header {.msg_type = message_type_t::order_cancel, .length = header_size + rbe::wire_size_of<OrderCancel>(), .seq_no = 2},
      OrderCancel {.security_id = 7, .order_ref = 42, .timestamp = 1},
      Header {.msg_type = message_type_t::order_add, .length = header_size + rbe::wire_size_of<OrderAdd>(), .seq_no = 3},
      OrderAdd {.security_id = 7, .side = side_t::buy, .quantity = 100, .price = 2500, .order_ref = 43, .timestamp = 2}
  );

  auto const packet = aquis::packet::dsrl_type::make(datagram).value();
  std::println("aquis: packet of {} messages", packet.header().field<"count">());

  for (auto const [header, payload]: packet.payload()) {
    std::print("  seq {}: ", header.field<"seq_no">());
    payload.match(
        [](Heartbeat const&) { std::println("heartbeat"); },
        [](OrderAdd const& add) { std::println("order add, {} @ {}", add.quantity, add.price); },
        [](rbe::unmatched auto const& other) { std::println("not handled, id known: {}", other.known_id); }
    );
  }
}


/// Nasdaq: SoupBinTCP wraps exactly one ITCH message and its `payload_length` says how long it is. The ITCH
/// message has no length of its own, the enclosing packet bounds it.
void nasdaq_packet() {
  using namespace nasdaq;

  auto const stream = wire(
      SoupHeader {.length = rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<AddOrder>()},
      ItchHeader {.msg_type = message_type_t::add_order, .stock_locate = 1, .tracking_number = 2},
      AddOrder {
          .order_reference_number = 1001,
          .buy_sell_indicator     = buy_sell_t::buy,
          .shares                 = 300,
          .stock                  = {'A', 'C', 'M', 'E', ' ', ' ', ' ', ' '},
          .price                  = 1'250'000
      }
  );

  auto const packet = nasdaq::packet::dsrl_type::make(stream).value();
  std::println("nasdaq: soup packet of {} bytes", packet.length());

  packet.payload().payload().match(
      [](AddOrder const& add) { std::println("  add order {}: {} shares @ {}", add.order_reference_number, add.shares, add.price); },
      [](rbe::unmatched auto const& other) { std::println("  not handled, id known: {}", other.known_id); }
  );
}

/// London: like Aquis, but every message carries its own length, so the packet payload is a `many` that can be
/// filtered with the views before touching a single field: `with_ids` keeps the ids you ask for, `known_ids` the ones
/// the code knows.
void london_packet() {
  using namespace lse;
  constexpr auto header_size = rbe::wire_size_of<Header>();
  constexpr auto delete_size = header_size + rbe::wire_size_of<OrderDelete>();
  constexpr auto event_size  = header_size + rbe::wire_size_of<SystemEvent>();

  auto const delete_of = [](order_id_t const id) {
    auto del     = OrderDelete {};
    del.order_id = id;
    return del;
  };

  auto const datagram = wire(
      UnitHeader {.length = rbe::wire_size_of<UnitHeader>() + 2 * delete_size + event_size, .message_count = 3},
      Header {.length = delete_size, .msg_type = message_type_t::order_delete}, delete_of(1),
      Header {.length = event_size, .msg_type = message_type_t::system_event}, SystemEvent {},
      Header {.length = delete_size, .msg_type = message_type_t::order_delete}, delete_of(2)
  );

  auto const packet = lse::packet::dsrl_type::make(datagram).value();
  std::println("london: packet of {} messages, {} bytes", packet.header().field<"message_count">(), packet.length());

  for (auto const frame: packet.payload() | rbe::views::with_ids(message_type_t::order_delete)) {
    frame.payload().match(
        [](OrderDelete const& del) { std::println("  order {} deleted", del.order_id); },
        [](rbe::unmatched auto const&) { }
    );
  }
}

/// Cboe TOP: a line-oriented ASCII feed. `line` is a custom frame that delimits by the LF, so `many<line>` walks
/// the stream line by line and an unknown message type is yielded as an unknown id and iteration goes on.
void cboe_top_lines() {
  using namespace cboe::top;

  auto const stream = wire(
      Header {.msg_type = message_type_t::seconds}, Seconds {.seconds = {'3', '6', '0', '0', '0'}}, std::array {'\n'},
      Header {.msg_type = static_cast<message_type_t>('?')}, std::array {'x', 'y', '\n'},
      Header {.msg_type = message_type_t::milliseconds}, Milliseconds {.milliseconds = {'0', '4', '2'}}, std::array {'\n'}
  );

  std::println("cboe top:");
  for (auto const line: stream | rbe::views::many<line_view>()) {
    line.payload().match(
        [](Seconds const& s) { std::println("  seconds {}", std::string_view {s.seconds.data(), s.seconds.size()}); },
        [](Milliseconds const& ms) { std::println("  milliseconds {}", std::string_view {ms.milliseconds.data(), ms.milliseconds.size()}); },
        [](rbe::unmatched auto const& other) { std::println("  skipped a line, id known: {}", other.known_id); }
    );
  }
}

/// OPRA: the message header carries no length and the category picks the layout. `Control` is a header-only message,
/// an `rbe::empty` alternative with a wire size of 0.
void opra_message() {
  using namespace opra;

  auto const datagram = wire(Header {.msg_category = msg_category_t::control, .msg_type = std::to_underlying(control_type_t::start_of_day)});

  auto const message = opra::message::dsrl_type::make(datagram).value();
  message.payload().match(
      [&](Control const&) { std::println("opra: control, type {}", message.header().field<"msg_type">()); },
      [](rbe::unmatched auto const& other) { std::println("opra: not handled, id known: {}", other.known_id); }
  );
}

} // namespace

int main() {
  aquis_packet();
  nasdaq_packet();
  london_packet();
  cboe_top_lines();
  opra_message();
  return 0;
}
