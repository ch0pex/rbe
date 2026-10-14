/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file reference_deserialization.cpp
 * @brief Snippets included by docs/reference/deserialization.md
 */

#include <rbe/rbe.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace example {

template<rbe::trivially_wirable T>
void signatures(std::span<std::byte const> buffer) {
  // --8<-- [start:signatures]
  T output                 = rbe::deserialize<T>(buffer, rbe::dsrl::eager);
  rbe::dsrl::proxy<T> view = rbe::deserialize<T>(buffer, rbe::dsrl::lazy);
  T const& ref             = rbe::deserialize<T>(buffer, rbe::dsrl::in_place);
  // --8<-- [end:signatures]
  (void)output, (void)view, (void)ref;
}

struct[[= rbe::pack_le]] Order {
  std::uint32_t order_id;
  std::uint32_t price;
};

void strategies(std::span<std::byte const> buffer) {
  {
    // --8<-- [start:eager]
    auto order = rbe::deserialize<Order>(buffer, rbe::dsrl::eager);
    // --8<-- [end:eager]
    (void)order;
  }
  {
    // --8<-- [start:lazy]
    auto view   = rbe::deserialize<Order>(buffer, rbe::dsrl::lazy);
    auto price  = view.field<"price">(); // decoded now, by field name (compile-time checked)
    auto second = view.field<1>(); // decoded now, by declaration index
    // --8<-- [end:lazy]
    (void)price, (void)second;
  }
  {
    // --8<-- [start:in_place]
    Order const& order = rbe::deserialize<Order>(buffer, rbe::dsrl::in_place);
    // --8<-- [end:in_place]
    (void)order;
  }
}

} // namespace example

auto main() -> int {
  alignas(example::Order) std::byte buffer[sizeof(example::Order)] {};
  example::signatures<example::Order>(buffer);
  example::strategies(buffer);
}
