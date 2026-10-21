/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file lazy_fields.cpp
 * @brief How-to: read fields through a lazy view (docs/examples/howto/lazy-fields.md)
 */

#include <rbe/annotations.hpp>
#include <rbe/dsrl.hpp>
#include <rbe/srl.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace {

// clang-format off
// --8<-- [start:types]
struct [[=rbe::pack_be]] Header {
  [[=rbe::id]] std::uint8_t type;
  std::uint16_t             length;

  auto operator==(Header const&) const -> bool = default;
};

struct [[=rbe::pack_be]] Quote {
  Header              header;
  std::array<char, 8> symbol;
  std::uint32_t       bid;
  std::uint32_t       ask;

  auto operator==(Quote const&) const -> bool = default;
};
// --8<-- [end:types]
// clang-format on

auto by_name_and_index(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:access]
  auto quote = rbe::deserialize<Quote>(bytes, rbe::dsrl::lazy);

  auto bid  = quote.field<"bid">();          // by name: a typo is a compile-time error
  auto ask  = quote.field<3>();              // by declaration index
  auto name = quote.field<"symbol">().value(); // a std::array member is a view too: value() decodes it
  // --8<-- [end:access]
  // clang-format on
  return bid == 100 and ask == 101 and name[0] == 'A';
}

auto nested(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:nested]
  auto quote = rbe::deserialize<Quote>(bytes, rbe::dsrl::lazy);

  auto header = quote.field<"header">();                      // a struct member is another view (use auto:
  auto length = header.field<"length">();                     // its type carries the inherited annotations)
  auto type   = quote.field<"header", "type">();              // or reached in one go with a path
  auto same   = quote.field<rbe::id>();                       // or by the annotation it carries, searching nested types
  // --8<-- [end:nested]
  // clang-format on
  return length == 19 and type == 7 and same == 7;
}

auto whole_value(std::span<std::byte const> const bytes) -> bool {
  // clang-format off
  // --8<-- [start:value]
  auto view = rbe::deserialize<Quote>(bytes, rbe::dsrl::lazy);

  Quote quote = view.value();                     // decode everything, same result as rbe::dsrl::eager
  Quote again = *view;                            // same thing

  std::span<std::byte const> raw = view.as_span(); // exactly the bytes Quote occupies: view.size() of them
  // --8<-- [end:value]
  // clang-format on
  return quote.bid == 100 and again.ask == 101 and raw.size() == rbe::wire_size_of<Quote>() and view == quote;
}

} // namespace

auto main() -> int {
  std::array<std::byte, rbe::wire_size_of<Quote>()> bytes {};
  rbe::serialize(
      bytes,
      Quote {
        .header = {.type = 7, .length = 19}, .symbol = {'A', 'C', 'M', 'E', ' ', ' ', ' ', ' '}, .bid = 100, .ask = 101
      }
  );
  return by_name_and_index(bytes) and nested(bytes) and whole_value(bytes) ? 0 : 1;
}
