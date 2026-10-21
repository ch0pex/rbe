/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file annotations.cpp
 * @brief How-to: describe the wire format of a type with annotations (docs/examples/howto/annotations.md)
 */

#include <rbe/annotations.hpp>
#include <rbe/core.hpp>

#include <array>
#include <cstdint>

namespace {

// clang-format off
// --8<-- [start:struct_level]
// Every member travels packed and big-endian
struct [[=rbe::pack, =rbe::big]] Header {
  std::uint16_t length;
  std::uint8_t  type;
  std::uint32_t sequence;
};

static_assert(sizeof(Header) == 8);                  // the struct keeps its C++ layout...
static_assert(rbe::wire_size_of<Header>() == 7);     // ...only the bytes on the wire are packed
// --8<-- [end:struct_level]

// --8<-- [start:member_override]
// A member's own annotation wins over the one it inherits from the struct
struct [[=rbe::pack, =rbe::big]] Record {
  std::uint32_t                  timestamp;  // big-endian, inherited
  [[=rbe::little]] std::uint32_t crc;        // little-endian, explicit
};
// --8<-- [end:member_override]

// --8<-- [start:nested]
struct Point {                                // no annotations of its own...
  std::int16_t x;
  std::int16_t y;
};

struct [[=rbe::pack_be]] Shape {
  Point         origin;                       // ...so it inherits pack and big from Shape
  std::uint8_t  sides;
};

struct [[=rbe::little]] LegacyFooter {        // a nested type with its own annotations keeps them
  std::uint32_t checksum;
};

struct [[=rbe::pack_be]] Message {
  Shape        shape;                         // packed, big-endian
  LegacyFooter footer;                        // packed (inherited), little-endian (its own)
};

static_assert(rbe::wire_size_of<Shape>() == 5);
static_assert(rbe::wire_size_of<Message>() == 9);
// --8<-- [end:nested]

// --8<-- [start:presets]
struct [[=rbe::pack_le]] A { std::uint32_t v; };            // same as [[=rbe::pack, =rbe::little]]
struct [[=rbe::pack_be]] B { std::uint32_t v; };            // same as [[=rbe::pack, =rbe::big]]

// your own preset: group annotations once, reuse them by name
inline constexpr auto wire_format = rbe::derive<rbe::pack, rbe::big, rbe::fmt>;

struct [[=wire_format]] C { std::uint32_t v; };
// --8<-- [end:presets]

// --8<-- [start:defaults]
// No annotation at all: little-endian and the regular C++ layout, padding included
struct Plain {
  std::uint8_t  flag;
  std::uint32_t value;
};

static_assert(rbe::wire_size_of<Plain>() == sizeof(Plain));  // 8, not 5
static_assert(rbe::trivially_wirable<Plain>);                 // so it is copied with one memcpy
// --8<-- [end:defaults]

// --8<-- [start:checks]
// Two annotations of the same dimension in one place is a mistake the compiler reports
struct [[=rbe::little, =rbe::big]] Conflicting { std::uint32_t v; };

static_assert(rbe::well_annotated<Message>);
static_assert(not rbe::well_annotated<Conflicting>);
// --8<-- [end:checks]
// clang-format on

} // namespace

auto main() -> int { return 0; }
