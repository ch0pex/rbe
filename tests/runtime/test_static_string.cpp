/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_static_string.cpp
 * @date 17/07/2026
 * @brief Short description
 *
 * Longer description
 */

// --- Includes ---
#include <rbe/core/static_string.hpp>

// --- Dependencies ---

// --- External dependencies ---
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

// --- STD ---

// --- System ---

namespace {

template<rbe::static_string Str>
struct X {
  static constexpr auto& value = Str.value;
};


constexpr auto test() -> bool {
  using namespace std::string_literals;
  using namespace std::string_view_literals;
  X<"hello"s> a;
  X<"hello"s> b;
  X<"other"s> c;
  return std::same_as<decltype(a), decltype(b)> //
         and not std::same_as<decltype(a), decltype(c)> //
         and not std::same_as<decltype(b), decltype(c)> //
         and a.value.size() == 5 //
         and a.value.data()[5] == '\0' //
         and a.value == "hello"sv //
         and c.value == "other"sv;
}

static_assert(test());

} // namespace


TEST_SUITE_BEGIN("static_string");

TEST_CASE("static string") { CHECK(test()); }


TEST_SUITE_END();
