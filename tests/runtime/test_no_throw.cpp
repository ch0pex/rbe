/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_no_throw.cpp
 * @date 05/09/2026
 * @brief Short description
 *
 * Longer description
 */

// --- Includes ---
#include <rbe/core/detail/no_throw.hpp>

// --- External dependencies ---
#include <doctest/doctest.h>

// --- STD ---
#include <stdexcept>

namespace {

constexpr void diagnose_throwing(bool throws) {
  if (throws) {
    throw std::runtime_error("throwing an error");
  }
}

constexpr void always_throws() { throw std::runtime_error("throwing an error"); }

} // namespace

TEST_CASE("no throw - runtime") {

  CHECK(rbe::detail::no_throw(diagnose_throwing, false));
  CHECK_FALSE(rbe::detail::no_throw(diagnose_throwing, true));
  CHECK_FALSE(rbe::detail::no_throw(always_throws));

  static_assert(rbe::detail::no_throw(diagnose_throwing, false));
  static_assert(not rbe::detail::no_throw(diagnose_throwing, true));
  static_assert(not rbe::detail::no_throw(always_throws));
}
