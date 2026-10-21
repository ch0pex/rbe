/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file reference_annotations.cpp
 * @brief Snippets included by docs/reference/annotations.md and docs/explanation/annotation-system-v2.md
 */

#include <rbe/annotations.hpp>
#include <rbe/core.hpp>

#include <concepts>
#include <cstdint>
#include <meta>
#include <vector>

namespace example {

// clang-format off
// --8<-- [start:basic]
struct [[=rbe::pack, =rbe::big]] Header {
  [[=rbe::little]] std::uint16_t length;
  std::uint8_t     type;
};
// --8<-- [end:basic]

// --8<-- [start:endianness]
struct [[=rbe::big]] Big      { std::uint32_t v; };                              // =rbe::order(rbe::endian::order::big)
struct [[=rbe::little]] Little { std::uint32_t v; };                             // the default when nothing is said
struct [[=rbe::order(rbe::endian::order::native)]] Native { std::uint32_t v; };  // the host's order, whatever it is
// --8<-- [end:endianness]

// --8<-- [start:alignment]
struct [[=rbe::pack]] Packed  { std::uint8_t a; std::uint32_t b; };   // =rbe::alignment(rbe::alignment_mode::pack)
struct [[=rbe::align]] Aligned { std::uint8_t a; std::uint32_t b; };  // =rbe::alignment(rbe::alignment_mode::align)

static_assert(rbe::wire_size_of<Packed>() == 5);
static_assert(rbe::wire_size_of<Aligned>() == 8);                     // the C++ layout, padding included

struct [[=rbe::pack]] Outer {
  Aligned inner;          // the nested type said align: it keeps it
  std::uint8_t c;
};
static_assert(rbe::wire_size_of<Outer>() == 9);
// --8<-- [end:alignment]
// clang-format on

enum class msg_type_t : std::uint8_t { heartbeat = 1, quote = 2 };

// clang-format off
// --8<-- [start:id]
struct hdr { [[=rbe::id]] msg_type_t type; [[=rbe::frame_length]] std::uint16_t len; };
struct [[=rbe::id(msg_type_t::heartbeat)]] Heartbeat { hdr h; std::uint32_t timestamp; };
// --8<-- [end:id]

// --8<-- [start:id_concepts]
struct [[=rbe::id(msg_type_t::quote)]] Quote { std::uint32_t bid, ask; };

static_assert(rbe::identifiable<Quote>);                      // declares the id it answers to
static_assert(rbe::identifying<hdr>);                         // has a field marked rbe::id
static_assert(not rbe::identifying<Heartbeat>);               // only direct fields count: the id field is inside hdr
static_assert(rbe::id_like<msg_type_t>);                      // equality comparable and copyable

static_assert(rbe::id_of<Quote>() == msg_type_t::quote);      // the value, with the type it was written with
static_assert(std::same_as<rbe::id_type_of<Quote>, msg_type_t>);
// --8<-- [end:id_concepts]

// --8<-- [start:length]
struct [[=rbe::pack_le]] FrameHeader {
  [[=rbe::frame_length]] std::uint16_t length;   // header + payload. Long form: =rbe::length(rbe::length_kind::frame)
  [[=rbe::id]]           std::uint8_t  type;
};

struct [[=rbe::pack_le]] BlockHeader {
  [[=rbe::header_length]]  std::uint8_t  header_length;   // this header's own length, may exceed sizeof: =rbe::self_length
  [[=rbe::payload_length]] std::uint16_t block_length;    // the payload alone
  [[=rbe::payload_count]]  std::uint16_t count;           // how many elements the payload holds
};
// --8<-- [end:length]

// --8<-- [start:count]
struct Batch {
  std::uint8_t                              n;
  [[=rbe::count("n")]] std::vector<std::uint32_t> items;   // `n` holds the number of elements of `items`
};

static_assert(rbe::well_annotated<Batch>);   // checked today; not read by serialization yet
// --8<-- [end:count]

// --8<-- [start:empty]
struct [[=rbe::id(msg_type_t::heartbeat), =rbe::empty]] Ping { };   // a candidate with no fields: 0 bytes on the wire

static_assert(rbe::explicitly_empty<Ping>);
static_assert(rbe::wire_size_of<Ping>() == 0);
// --8<-- [end:empty]

// --8<-- [start:friendly]
class Sealed { std::uint32_t v_ {}; };            // a non-public member: not wirable, the author did not say so

class [[=rbe::friendly]] Opened {                  // the author grants RBE access to its representation
public:
  constexpr Opened() = default;
  constexpr explicit Opened(std::uint32_t const v) : v_(v) { }
  [[nodiscard]] constexpr auto v() const -> std::uint32_t { return v_; }
private:
  std::uint32_t v_ {};
};

struct [[=rbe::friendly]] Holder { Sealed inner; };   // not inherited: Sealed has to open itself

static_assert(not rbe::wirable<Sealed>);
static_assert(rbe::wirable<Opened>);
static_assert(not rbe::wirable<Holder>);
// --8<-- [end:friendly]

// --8<-- [start:fmt]
struct [[=rbe::fmt]] Debuggable { std::uint32_t v; };      // std::format("{}", Debuggable {}) works
struct [[=rbe::pack_be, =rbe::debug]] Both { std::uint32_t v; };   // rbe::debug == derive<fmt>, combines with the rest
// --8<-- [end:fmt]

// --8<-- [start:derive]
struct [[=rbe::derive<rbe::pack, rbe::little>]] Order { /* ... */ };

inline constexpr auto packed_be = rbe::derive<rbe::pack, rbe::big>;
struct [[=packed_be]] Msg { /* ... */ };
// --8<-- [end:derive]

// --8<-- [start:concepts]
struct [[=rbe::little, =rbe::big]] Twice   { std::uint32_t v; };   // two of one dimension in one range
struct [[=rbe::pack_be]] Fine { [[=rbe::little]] std::uint32_t v; };

static_assert(rbe::well_annotated<Fine>);
static_assert(not rbe::well_annotated<Twice>);

static_assert(rbe::contains_annotation<Fine, rbe::pack>);                           // anywhere in the type, in depth
static_assert(rbe::contains_annotation<Fine, rbe::little>);                         // the member's own
static_assert(rbe::contains_annotation<Fine, rbe::derive<rbe::pack, rbe::big>>);    // a list: all of them
static_assert(not rbe::contains_annotation<Fine, rbe::id>);
// --8<-- [end:concepts]
// clang-format on

enum class time_unit : std::uint8_t { milliseconds, microseconds };

// clang-format off
// --8<-- [start:new_annotation]
// 1. (optional) the dimension, if the annotation conflicts with others. The dimension and the tag
//    are plumbing -- keep them out of whatever namespace your users see, as RBE keeps its own in
//    `rbe::detail`.
struct unit_dim {
  static constexpr auto kind = rbe::detail::dimension_kind::exclusive;
};

// 2. the tag: everything the annotation knows about itself, in one place
struct unit_tag {
  using dimension  = unit_dim;      // optional -- omit for an annotation that conflicts with nothing
  using value_type = time_unit;     // optional -- omit for a pure marker like rbe::fmt

  // optional: the annotation's own correctness rule, checked by `rbe::well_annotated`
  static consteval auto check(rbe::detail::annotation_info const, std::meta::info const entity) -> bool {
    return is_integral_type(rbe::detail::normalize_type(entity));
  }
};

// 3. the factory object, plus one alias per value you want to name
inline constexpr rbe::detail::annotation_kind<unit_tag> unit {};

inline constexpr auto millis = unit(time_unit::milliseconds);
inline constexpr auto micros = unit(time_unit::microseconds);
// --8<-- [end:new_annotation]
// clang-format on

// clang-format off
// --8<-- [start:new_annotation_use]
struct Tick {
  [[=millis]] std::uint32_t timestamp;          // or [[=unit(time_unit::milliseconds)]]
};
// --8<-- [end:new_annotation_use]
// clang-format on

static_assert(rbe::well_annotated<Header>);
static_assert(rbe::well_annotated<Heartbeat>);
static_assert(rbe::well_annotated<FrameHeader>);
static_assert(rbe::well_annotated<BlockHeader>);
static_assert(rbe::well_annotated<Order>);
static_assert(rbe::well_annotated<Msg>);
static_assert(rbe::well_annotated<Tick>);
static_assert(rbe::well_annotated<Ping>);
static_assert(rbe::well_annotated<Opened>);

} // namespace example

auto main() -> int { }
