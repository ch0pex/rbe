/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file reference_serialization.cpp
 * @brief Snippets included by docs/reference/serialization.md
 */

#include <rbe/rbe.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace example {

template<rbe::wirable T>
auto signature(std::span<std::byte> buffer, T const& value) {
  // clang-format off
  // --8<-- [start:signature]
  std::size_t                bytes_written = rbe::serialize(buffer, value);       // narrow contract
  std::optional<std::size_t> checked       = rbe::try_serialize(buffer, value);   // wide contract
  // --8<-- [end:signature]
  // clang-format on
  return bytes_written == checked;
}

// clang-format off
// --8<-- [start:example_type]
struct [[=rbe::derive<rbe::pack, rbe::little>]] Order {
  std::uint32_t order_id;
  std::uint32_t price;
};
// --8<-- [end:example_type]
// clang-format on

auto run() -> std::size_t {
  // clang-format off
  // --8<-- [start:example_call]
  Order order {.order_id = 12345, .price = 15050};

  std::vector<std::byte> buffer(rbe::wire_size_of<Order>());
  std::size_t n = rbe::serialize(buffer, order);   // n == rbe::wire_size_of<Order>() == 8
  // --8<-- [end:example_call]
  // clang-format on
  return n;
}

auto checked() -> bool {
  // clang-format off
  // --8<-- [start:try_serialize]
  std::vector<std::byte> small(4);

  std::optional<std::size_t> n = rbe::try_serialize(small, Order {});   // std::nullopt: 4 < 8, nothing written
  // --8<-- [end:try_serialize]
  // clang-format on
  return not n.has_value();
}

// clang-format off
struct [[=rbe::pack_le]] Header { std::uint16_t length; std::uint8_t type; };
// clang-format on

auto compose() -> bool {
  // clang-format off
  // --8<-- [start:compose]
  // A header and its payload: each call writes at offset 0 of the span it is given
  std::vector<std::byte> out(rbe::wire_size_of<Header>() + rbe::wire_size_of<Order>());
  std::span<std::byte>   cursor = out;

  cursor = cursor.subspan(rbe::serialize(cursor, Header {.length = 11, .type = 1}));
  cursor = cursor.subspan(rbe::serialize(cursor, Order {.order_id = 1, .price = 2}));
  // --8<-- [end:compose]
  // clang-format on
  return cursor.empty();
}

} // namespace example

auto main() -> int {
  std::vector<std::byte> buffer(rbe::wire_size_of<example::Order>());
  return example::signature(buffer, example::Order {}) and example::run() == rbe::wire_size_of<example::Order>() and
                 example::checked() and example::compose()
             ? 0
             : 1;
}
