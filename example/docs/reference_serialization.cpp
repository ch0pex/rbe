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
#include <vector>

namespace example {

template<rbe::wirable T>
auto signature(std::span<std::byte> buffer, T const& value) {
  // --8<-- [start:signature]
  std::size_t bytes_written = rbe::serialize(buffer, value);
  // --8<-- [end:signature]
  return bytes_written;
}

// --8<-- [start:example_type]
struct[[= rbe::derive<rbe::pack, rbe::little>]] Order {
  std::uint32_t order_id;
  std::uint32_t price;
};
// --8<-- [end:example_type]

auto run() -> std::size_t {
  // --8<-- [start:example_call]
  Order order {.order_id = 12345, .price = 15050};

  std::vector<std::byte> buffer(rbe::wire_size_of<Order>());
  std::size_t n = rbe::serialize(buffer, order);
  // --8<-- [end:example_call]
  return n;
}

} // namespace example

auto main() -> int { return example::run() == rbe::wire_size_of<example::Order>() ? 0 : 1; }
