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
#include <rbe/core/detail/throw_check.hpp>

// --- External dependencies ---
#include "test_macros.hpp"

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

  RBE_CHECK(rbe::detail::no_throw(diagnose_throwing, false));
  RBE_CHECK_FALSE(rbe::detail::no_throw(diagnose_throwing, true));
  RBE_CHECK_FALSE(rbe::detail::no_throw(always_throws));

  static_assert(rbe::detail::no_throw(diagnose_throwing, false));
  static_assert(not rbe::detail::no_throw(diagnose_throwing, true));
  static_assert(not rbe::detail::no_throw(always_throws));
}

TEST_CASE("throws- runtime") {

  RBE_CHECK_FALSE(rbe::detail::throws(diagnose_throwing, false));
  RBE_CHECK(rbe::detail::throws(diagnose_throwing, true));
  RBE_CHECK(rbe::detail::throws(always_throws));

  static_assert(not rbe::detail::throws(diagnose_throwing, false));
  static_assert(rbe::detail::throws(diagnose_throwing, true));
  static_assert(rbe::detail::throws(always_throws));
}
