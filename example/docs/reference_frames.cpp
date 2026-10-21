/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file reference_frames.cpp
 * @brief Snippets included by docs/reference/frames.md
 */

#include <rbe/rbe.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <tuple>
#include <vector>

namespace example {

// clang-format off
// --8<-- [start:types]
struct [[=rbe::pack_be]] Header {
  [[=rbe::frame_length]] std::uint16_t length;   // where the frame ends
  [[=rbe::id]]           std::uint8_t  kind;     // which candidate follows
};

struct [[=rbe::pack_be, =rbe::id(std::uint8_t {1})]] A { std::uint32_t value; };
struct [[=rbe::pack_be, =rbe::id(std::uint8_t {2})]] B { std::uint64_t value; std::uint8_t flag; };
struct [[=rbe::id(std::uint8_t {3}), =rbe::empty]]   Ping { };

struct [[=rbe::pack_be]] CountHeader {
  [[=rbe::payload_count]] std::uint16_t count;   // how many elements the run holds
};

struct [[=rbe::pack_be]] PlainHeader { std::uint16_t sequence; };   // nothing about the payload

using single  = rbe::frames::frame<Header, A>;                              // header + one wirable type
using record  = rbe::frames::frame<Header, rbe::frames::any<A, B, Ping>>;   // header + one of several, by id
using records = rbe::frames::frame<CountHeader, rbe::frames::many<record>>; // header + a run of frames
using raw     = rbe::frames::frame<PlainHeader, rbe::frames::blob>;         // header + opaque bytes
using nested  = rbe::frames::frame<CountHeader, single>;                    // header + another frame
// --8<-- [end:types]

// --8<-- [start:associated_types]
static_assert(std::same_as<record::header_type, Header>);
static_assert(std::same_as<record::payload_type, rbe::frames::any<A, B, Ping>>);
static_assert(std::same_as<record::dsrl_type,
                           rbe::frames::dsrl::frame<Header, rbe::frames::dsrl::any<A, B, Ping>>>);
static_assert(std::same_as<record::delimiting_concept, rbe::frames::length_delimited_tag>);
// --8<-- [end:associated_types]

// --8<-- [start:delimiting]
struct [[=rbe::pack_be]] IdHeader     { [[=rbe::id]] std::uint8_t kind; };
struct [[=rbe::pack_be]] LengthHeader { [[=rbe::header_length]] std::uint8_t length; std::uint8_t flags; };

using fixed    = rbe::frames::frame<PlainHeader, A>;                              // nothing on the wire: sizes known to the code
using by_id    = rbe::frames::frame<IdHeader, rbe::frames::any<A, B, Ping>>;      // the candidate the id picks has the size
using counted  = rbe::frames::frame<CountHeader, rbe::frames::many<fixed>>;       // count * a constant stride
using extended = rbe::frames::frame<LengthHeader, A>;                             // the header may be longer than the code knows
using open     = rbe::frames::frame<PlainHeader, rbe::frames::many<record>>;      // no count, no length: runs to the end of the buffer

static_assert(rbe::frames::length_delimited<record>        and rbe::frames::constant_length<record>);
static_assert(rbe::frames::static_delimited<fixed>         and rbe::frames::static_length<fixed> == 6);
static_assert(rbe::frames::implicitly_delimited<by_id>     and not rbe::frames::static_delimited<by_id>);
static_assert(rbe::frames::static_stride_delimited<counted>and rbe::frames::constant_length<counted>);
static_assert(rbe::frames::count_delimited<records>        and not rbe::frames::constant_length<records>); // walks
static_assert(rbe::frames::implicitly_delimited<extended>  and not rbe::frames::static_delimited<extended>);
static_assert(rbe::frames::buffer_delimited<open>          and rbe::frames::constant_length<open>);
static_assert(rbe::frames::buffer_delimited<raw>);

static_assert(rbe::frames::self_delimited<record> and not rbe::frames::self_delimited<raw>);
static_assert(rbe::frames::explicitly_delimited<record> and not rbe::frames::explicitly_delimited<by_id>);
static_assert(std::same_as<rbe::frames::delimiting_concept_t<counted>, rbe::frames::static_stride_delimited_tag>);
static_assert(std::same_as<rbe::frames::frame_delimiting_tag<Header, A>, rbe::frames::length_delimited_tag>);
// --8<-- [end:delimiting]
// clang-format on

auto deserialize_frame(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:deserialize]
  record::dsrl_type                view    = rbe::deserialize<record>(bytes);       // narrow: bytes hold the frame
  std::optional<record::dsrl_type> checked = rbe::try_deserialize<record>(bytes);   // wide: nullopt if they do not
  // --8<-- [end:deserialize]
  // clang-format on
  return view.length() == bytes.size() and checked.has_value();
}

auto frame_view(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:frame_view]
  auto frame = rbe::deserialize<record>(bytes);

  auto header  = frame.header();          // rbe::frames::dsrl::header<Header>
  auto payload = frame.payload();         // rbe::frames::dsrl::any<A, B, Ping>
  auto [h, p]  = frame;                   // the same two, as a structured binding

  std::size_t                length  = frame.length();        // header + payload, only if constant_length
  std::span<std::byte const> whole   = frame.as_span();       // the length() bytes from data()
  std::span<std::byte const> head    = frame.header_span();
  std::span<std::byte const> body    = frame.payload_span();
  std::span<std::byte const> handed  = frame.buffer();        // the buffer the view was built over, untrimmed
  std::byte const*           first   = frame.data();
  // --8<-- [end:frame_view]
  // clang-format on
  return length == whole.size() and head.size() + body.size() == length and handed.data() == first and
         h.id() == header.id() and p.id() == payload.id();
}

auto header_view(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:header_view]
  rbe::frames::dsrl::header<Header> header = rbe::deserialize<record>(bytes).header();

  auto kind   = header.field<"kind">();      // everything a proxy offers: field, value, operator*, size, buffer, data
  auto id     = header.id();                 // the field marked rbe::id            (requires has_id)
  auto frame  = header.frame_length();       // header + payload, from the wire      (has_frame_length or has_payload_length)
  auto body   = header.payload_length();     // frame_length() - length() here       (has_payload_length or has_frame_length)
  auto length = header.length();             // the header_length field, else size()

  static_assert(decltype(header)::has_frame_length and decltype(header)::has_id);
  static_assert(decltype(header)::delimits_payload);    // has_frame_length or has_payload_length

  bool extended = header.is_extended();       // the wire declared more header bytes than Header has fields for
  auto known    = header.known_span();        // the bytes Header has fields for
  auto extra    = header.extension_span();    // the rest, empty unless is_extended()
  // --8<-- [end:header_view]
  // clang-format on
  return kind == id and frame == length + body and not extended and known.size() == length and extra.empty();
}

auto any_view(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:any_view]
  rbe::frames::dsrl::any<A, B, Ping> payload = rbe::deserialize<record>(bytes).payload();

  // one callback per candidate, by value (decoded) or by proxy (decoded on access), plus the mandatory fallback
  auto result = payload.match(
      [](A const& a) { return a.value; },
      [](rbe::dsrl::proxy<B> b) { return static_cast<std::uint32_t>(b.field<"value">()); },
      [](Ping) { return std::uint32_t {0}; },                       // an empty candidate is always taken by value
      [](rbe::unmatched auto u) { return u.known_id ? std::uint32_t {1} : std::uint32_t {2}; }  // u.id, u.known_id, u.data
  );

  bool                        is_a  = payload.is<A>();
  std::optional<rbe::dsrl::proxy<A>> lazy  = payload.as<A>();                    // lazy by default
  std::optional<A>            eager = payload.as<A>(rbe::dsrl::eager);
  std::optional<Ping>         ping  = payload.as<Ping>();                        // nullopt here

  auto id    = payload.id();          // the id on the wire, as the header field's type
  bool known = payload.known_id();    // a candidate answers to it
  auto size  = payload.size();        // the candidate's wire size, or the rest of the buffer for an unknown id
  auto span  = payload.as_span();
  // --8<-- [end:any_view]
  // clang-format on
  return result == 7 and is_a and lazy.has_value() and eager->value == 7 and not ping and id == 1 and known and
         size == span.size();
}

auto many_view(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:many_view]
  auto frame = rbe::deserialize<records>(bytes);

  // a many is a single-pass input range of frame views, handed out by value
  for (auto const [h, p]: frame.payload()) {
    static_cast<void>(h), static_cast<void>(p);
  }

  // the same bytes, stepped by hand. payload() hands out a fresh many each time, iteration advances the copy
  rbe::frames::dsrl::many<record::dsrl_type> run = frame.payload();
  while (not run.done()) {
    record::dsrl_type current = run.current();
    run.next();
    static_cast<void>(current);
  }

  bool                       partial = run.has_seen_partial();   // stopped on an incomplete element
  std::span<std::byte const> left    = run.remainder();          // what iteration did not consume
  // --8<-- [end:many_view]
  // clang-format on
  return not partial and left.empty();
}

auto adaptors(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:adaptors]
  // views::many builds a many over a bare run, no frame around it. Each pipeline starts from a fresh one.
  auto as     = std::ranges::distance(bytes | rbe::views::many<record::dsrl_type>() | rbe::views::with_ids(std::uint8_t {1}));
  auto known  = std::ranges::distance(bytes | rbe::views::many<record::dsrl_type>() | rbe::views::known_ids());
  auto others = std::ranges::distance(bytes | rbe::views::many<record::dsrl_type>() | rbe::views::unknown_ids());
  // --8<-- [end:adaptors]
  // clang-format on
  return as == 1 and known == 3 and others == 1;
}

auto length_and_distance(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:length_distance]
  auto frame = rbe::deserialize<records>(bytes);

  // records is count-delimited with a variable stride: no length(), the extent is measured element by element
  static_assert(not rbe::frames::constant_length<records>);
  std::size_t walked = rbe::frames::distance(frame);          // header plus every element, O(n)

  auto first = frame.payload().current();
  std::size_t direct = rbe::frames::length(first);            // a record declares its length: O(1)
  // --8<-- [end:length_distance]
  // clang-format on
  return walked == bytes.size() and direct == first.length();
}

template<typename... Ts>
auto wire(Ts const&... parts) -> std::vector<std::byte> {
  auto out    = std::vector<std::byte>((rbe::wire_size_of<Ts>() + ...));
  auto offset = std::size_t {0};
  auto append = [&]<typename T>(T const& part) {
    if constexpr (rbe::wirable<T>) {
      offset += rbe::serialize(std::span {out}.subspan(offset), part);
    }
  };
  (append(parts), ...);
  return out;
}

} // namespace example

auto main() -> int {
  using namespace example;
  constexpr std::uint16_t a_size    = rbe::wire_size_of<Header>() + rbe::wire_size_of<A>();
  constexpr std::uint16_t b_size    = rbe::wire_size_of<Header>() + rbe::wire_size_of<B>();
  constexpr std::uint16_t ping_size = rbe::wire_size_of<Header>();

  auto const one = wire(Header {.length = a_size, .kind = 1}, A {.value = 7});
  auto const run = wire(
      CountHeader {.count = 4}, //
      Header {.length = a_size, .kind = 1}, A {.value = 7}, //
      Header {.length = b_size, .kind = 2}, B {.value = 8, .flag = 1}, //
      Header {.length = ping_size, .kind = 3}, Ping {}, //
      Header {.length = a_size, .kind = 9}, A {.value = 0} // unknown kind, the length still bounds it
  );
  auto const bare = std::span {run}.subspan(rbe::wire_size_of<CountHeader>());

  return deserialize_frame(one) and frame_view(one) and header_view(one) and any_view(one) and many_view(run) and
                 adaptors(bare) and length_and_distance(run)
             ? 0
             : 1;
}
