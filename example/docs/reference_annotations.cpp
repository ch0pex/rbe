/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file reference_annotations.cpp
 * @brief Snippets included by docs/reference/annotations.md
 */

#include <rbe/annotations.hpp>

#include <cstdint>
#include <meta>

namespace example {

// clang-format off
// --8<-- [start:basic]
struct [[=rbe::pack, =rbe::big]] Header {
  [[=rbe::little]] std::uint16_t length;
  std::uint8_t     type;
};
// --8<-- [end:basic]
// clang-format on

enum class msg_type_t : std::uint8_t { heartbeat = 1 };

// clang-format off
// --8<-- [start:id]
struct hdr { [[=rbe::id]] msg_type_t type; [[=rbe::frame_length]] std::uint16_t len; };
struct [[=rbe::id(msg_type_t::heartbeat)]] Heartbeat { hdr h; std::uint32_t timestamp; };
// --8<-- [end:id]
// clang-format on

// clang-format off
// --8<-- [start:derive]
struct [[=rbe::derive<rbe::pack, rbe::little>]] Order { /* ... */ };

inline constexpr auto packed_be = rbe::derive<rbe::pack, rbe::big>;
struct [[=packed_be]] Msg { /* ... */ };
// --8<-- [end:derive]
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
static_assert(rbe::well_annotated<Order>);
static_assert(rbe::well_annotated<Msg>);
static_assert(rbe::well_annotated<Tick>);

} // namespace example

auto main() -> int { }
