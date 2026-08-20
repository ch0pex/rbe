/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_memcpy_constexpr.cpp
 * @date 27/06/2026
 * @brief Doctest suite for memcpy_constexpr and load helpers
 */

// --- Includes ---
#include "common_structs.hpp"

// --- Dependencies ---
#include <rbe/core/detail/memcpy_constexpr.hpp>

// --- External dependencies ---
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

// --- STD ---
#include <algorithm>
#include <cstddef>

// --- System ---

using rbe::detail::memcpy_constexpr;

namespace {


// --- Compile-time tests for memcpy_constexpr ---
// Golden bytes are little-endian IEEE 754 representations verified at build time.

static_assert([] {
  using B = std::byte;
  constexpr NonPaddedStruct src {.a = 1, .b = 2, .c = 3.0};
  std::array<std::byte, sizeof(NonPaddedStruct)> dst {};
  memcpy_constexpr(dst, src);
  if (dst != std::array {
               B {0x01}, B {0x00}, B {0x00}, B {0x00}, // a = 1
               B {0x02}, B {0x00}, B {0x00}, B {0x00}, // b = 2
               B {0x00}, B {0x00}, B {0x00}, B {0x00}, // c = 3.0 (0x4008000000000000 LE)
               B {0x00}, B {0x00}, B {0x08}, B {0x40}
             })
    return false;
  return std::bit_cast<NonPaddedStruct>(dst) == src;
}());

static_assert([] {
  using B = std::byte;
  constexpr NonPaddedStruct src {.a = 1, .b = 2, .c = 0.0};
  std::array<std::byte, sizeof(NonPaddedStruct)> dst {};
  memcpy_constexpr(dst, src);
  if (dst != std::array {
               B {0x01}, B {0x00}, B {0x00}, B {0x00}, // a = 1
               B {0x02}, B {0x00}, B {0x00}, B {0x00}, // b = 2
               B {0x00}, B {0x00}, B {0x00}, B {0x00}, // c = 0.0
               B {0x00}, B {0x00}, B {0x00}, B {0x00}
             })
    return false;
  return std::bit_cast<NonPaddedStruct>(dst) == src;
}());

// NOTE: Due to indeterminate bytes, consteval use with PaddedStruct and EmptyStruct fails to compile
// static_assert([]{...memcpy_constexpr(dst, PaddedStruct{...})...}());
// static_assert([] {
//   using B = std::byte;
//   std::array<std::byte, sizeof(EmptyStruct)> dst {};
//   memcpy_constexpr(dst, EmptyStruct {});
//   return dst.size() == sizeof(EmptyStruct);
// }());

TEST_CASE("memcpy_constexpr produces correct bytes for types without padding") {
  using B = std::byte;
  NonPaddedStruct src {.a = 42, .b = 84, .c = 3.14};
  std::array<std::byte, sizeof(NonPaddedStruct)> dst {};
  memcpy_constexpr(dst, src);
  REQUIRE(
      dst == std::array {
               B {0x2A}, B {0x00}, B {0x00}, B {0x00}, // a = 42
               B {0x54}, B {0x00}, B {0x00}, B {0x00}, // b = 84
               B {0x1F}, B {0x85}, B {0xEB}, B {0x51}, // c = 3.14 (0x40091EB851EB851F LE)
               B {0xB8}, B {0x1E}, B {0x09}, B {0x40}
             }
  );
  REQUIRE(std::bit_cast<NonPaddedStruct>(dst) == src);
}

TEST_CASE("memcpy_constexpr handles empty structs") {
  EmptyStruct src {};
  std::array<std::byte, sizeof(EmptyStruct)> dst {};
  memcpy_constexpr(dst, src);
  // EmptyStruct has no members; its single byte is a compiler artifact with no defined content
  REQUIRE(std::bit_cast<EmptyStruct>(dst) == src);
}

TEST_CASE("memcpy_constexpr copies field bytes correctly for padded types") {
  using B = std::byte;
  PaddedStruct src {.a = 1, .b = 2.0, .c = 'c'};
  std::array<B, sizeof(PaddedStruct)> dst {};
  memcpy_constexpr(dst, src);

  // Padding bytes are indeterminate; only check fields at their known offsets
  constexpr std::size_t a_off = offsetof(PaddedStruct, a);
  constexpr std::size_t b_off = offsetof(PaddedStruct, b);
  constexpr std::size_t c_off = offsetof(PaddedStruct, c);

  std::array<B, 4> a_bytes {};
  std::array<B, 8> b_bytes {};
  std::ranges::copy_n(std::next(std::ranges::begin(dst), a_off), 4, std::ranges::begin(a_bytes));
  std::ranges::copy_n(std::next(std::ranges::begin(dst), b_off), 8, std::ranges::begin(b_bytes));

  REQUIRE(a_bytes == std::array {B {0x01}, B {0x00}, B {0x00}, B {0x00}}); // a = 1
  REQUIRE(
      b_bytes == std::array {
                   B {0x00}, B {0x00}, B {0x00}, B {0x00}, // b = 2.0
                   B {0x00}, B {0x00}, B {0x00}, B {0x40}
                 }
  );
  REQUIRE(dst[c_off] == B {0x63}); // c = 'c'
  REQUIRE(std::bit_cast<PaddedStruct>(dst) == src);
}

} // namespace
