/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file format.cpp
 * @brief How-to: print an annotated type with std::format (docs/examples/howto/format.md)
 */

#include <rbe/annotations.hpp>
#include <rbe/core.hpp>

#include <cstdint>
#include <format>
#include <iostream>
#include <print>
#include <string>

// clang-format off
// --8<-- [start:types]
// rbe::fmt opts a type into the formatter. A nested struct needs it too, or a std::formatter of its own.
struct [[=rbe::fmt]] Price {
  std::int64_t mantissa;
  std::uint8_t exponent;
};

// rbe::debug is the preset for rbe::fmt, and composes with the wire annotations
struct [[=rbe::pack_be, =rbe::debug]] Order {
  std::uint32_t order_id;
  std::uint32_t quantity;
  Price         price;
};
// --8<-- [end:types]
// clang-format on

auto main() -> int {
  // clang-format off
  // --8<-- [start:print]
  Order const order {.order_id = 7, .quantity = 100, .price = {.mantissa = 2500, .exponent = 2}};

  std::println("{}", order);                         // std::print and std::println
  std::string text = std::format("{}", order);       // std::format
  std::cout << order << '\n';                        // and streams

  // Order {
  //   .order_id = 7,
  //   .quantity = 100,
  //   .price = Price {
  //     .mantissa = 2500,
  //     .exponent = 2,
  //   },
  // }
  // --8<-- [end:print]
  // clang-format on
  return text.starts_with("Order {") ? 0 : 1;
}
