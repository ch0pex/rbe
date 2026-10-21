/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file reference_core.cpp
 * @brief Snippets included by docs/reference/core.md
 */

#include <rbe/annotations.hpp>
#include <rbe/core.hpp>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <vector>

namespace example {

// clang-format off
// --8<-- [start:concepts]
enum class kind : std::uint8_t { a, b };
struct Point { std::int16_t x, y; };
struct [[=rbe::pack_be]] Packed { std::uint8_t tag; std::uint32_t value; };
struct Base { std::uint8_t b; };
struct Derived : Base { std::uint8_t d; };

static_assert(rbe::wirable<std::uint32_t>);             // integers...
static_assert(rbe::wirable<kind>);                      // ...enumerations...
static_assert(rbe::wirable<std::array<kind, 4>>);       // ...arrays of integers or enumerations...
static_assert(rbe::wirable<Point>);                     // ...and classes of wirable members
static_assert(not rbe::wirable<double>);                // floating point is not supported yet
static_assert(not rbe::wirable<std::array<Point, 2>>);  // nor arrays of classes
static_assert(not rbe::wirable<std::vector<int>>);      // nor variable-size ranges
static_assert(not rbe::wirable<Derived>);               // nor base classes

class [[=rbe::friendly]] Account {                      // not an aggregate: friendly opens its private members to RBE
public:
  constexpr auto balance() const -> std::int32_t { return balance_; }
private:
  std::int32_t balance_ {};
};
static_assert(rbe::wirable<Account>);

static_assert(rbe::trivially_wirable<Point>);           // in-memory layout == wire layout: one memcpy
static_assert(not rbe::trivially_wirable<Packed>);      // packed and byte-swapped: member by member

static_assert(rbe::wirable_class<Point> and not rbe::wirable_class<kind>);
static_assert(rbe::wirable_range<std::array<kind, 4>>);
static_assert(rbe::trivially_wirable_primitive<kind>);
// --8<-- [end:concepts]

// --8<-- [start:wire_size_of]
static_assert(sizeof(Packed) == 8);
static_assert(rbe::wire_size_of<Packed>() == 5);        // consteval: usable as an array bound
static_assert(rbe::wire_size_of<Point>() == 4);
// --8<-- [end:wire_size_of]
// clang-format on

auto endian_helpers() -> bool {
  // clang-format off
  // --8<-- [start:endian]
  using rbe::endian::order;   // = std::endian

  std::uint32_t host = 0x0102'0304;
  std::uint32_t wire = rbe::endian::native_to<order::big>(host);   // byte-swapped on a little-endian host
  std::uint32_t back = rbe::endian::to_native<order::big>(wire);   // and back

  std::array<std::byte, 4> bytes {};
  rbe::endian::store<std::uint32_t, order::big>(bytes.data(), host);       // 01 02 03 04
  std::uint32_t loaded = rbe::endian::load<std::uint32_t, order::big>(bytes.data());
  // --8<-- [end:endian]
  // clang-format on
  return back == host and loaded == host and bytes[0] == std::byte {1};
}

// clang-format off
// --8<-- [start:fmt]
struct [[=rbe::fmt]] Inner { std::uint8_t x; };
struct [[=rbe::debug]] Outer { Inner inner; std::uint16_t y; };   // rbe::debug is derive<fmt>

auto show(Outer const& value) -> std::string {
  return std::format("{}", value);   // "Outer {\n  .inner = Inner {\n    .x = 1,\n  },\n  .y = 2,\n}"
}
// --8<-- [end:fmt]

auto overload_set() -> bool {
  // clang-format off
  // --8<-- [start:overload]
  auto const visit = rbe::overload {
    [](std::uint32_t) { return 1; },
    [](std::string const&) { return 2; },
  };
  // --8<-- [end:overload]
  // clang-format on
  return visit(std::uint32_t {7}) == 1 and visit(std::string {"x"}) == 2;
}

} // namespace example

auto main() -> int {
  return example::endian_helpers() and example::overload_set() and
                 example::show(example::Outer {.inner = {.x = 1}, .y = 2}).starts_with("Outer {")
             ? 0
             : 1;
}
