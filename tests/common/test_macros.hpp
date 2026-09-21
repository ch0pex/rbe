/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file macro.hpp
 * @date 14/09/2026
 * @brief RBE testing macros
 *
 * This macros wraps doctests macros in order to test both compile and runtime
 */

#pragma once

// --- Includes --
#include <doctest/doctest.h>
#include <exception>

#include <doctest/doctest.h>

#include <doctest/doctest.h>
#include <stdexcept>


namespace rbe {

struct compiletime_error : std::logic_error {
  using std::logic_error::logic_error;
};

} // namespace rbe


// --- Helper macros to bake File & Line into string literals ---
#define RBE_STRINGIFY_IMPL(x) #x
#define RBE_STRINGIFY(x) RBE_STRINGIFY_IMPL(x)
#define RBE_LOCATION __FILE__ ":" RBE_STRINGIFY(__LINE__) " -> "

// 1. RBE_RUN_TEST_CASE
#define RBE_RUN_TEST_CASE(test_func, ...)                                                                              \
  consteval { test_func(__VA_OPT__(__VA_ARGS__)); }                                                                    \
  test_func(__VA_OPT__(__VA_ARGS__));

// 2. RBE_TEST_CASE
#define RBE_TEST_CASE(name, test_func, ...)                                                                            \
  TEST_CASE(name) { RBE_RUN_TEST_CASE(test_func __VA_OPT__(, ) __VA_ARGS__) }

// 3. RBE_CHECK
#define RBE_CHECK(...)                                                                                                 \
  do {                                                                                                                 \
    if consteval {                                                                                                     \
      if (!(__VA_ARGS__)) {                                                                                            \
        throw rbe::compiletime_error(RBE_LOCATION "Compile-time check failed: " #__VA_ARGS__);                         \
      }                                                                                                                \
    }                                                                                                                  \
    else {                                                                                                             \
      CHECK(__VA_ARGS__);                                                                                              \
    }                                                                                                                  \
  }                                                                                                                    \
  while (false)

// 4. RBE_REQUIRE
#define RBE_REQUIRE(...)                                                                                               \
  do {                                                                                                                 \
    if consteval {                                                                                                     \
      if (!(__VA_ARGS__)) {                                                                                            \
        throw rbe::compiletime_error(RBE_LOCATION "Compile-time require failed: " #__VA_ARGS__);                       \
      }                                                                                                                \
    }                                                                                                                  \
    else {                                                                                                             \
      REQUIRE(__VA_ARGS__);                                                                                            \
    }                                                                                                                  \
  }                                                                                                                    \
  while (false)

// 5. RBE_CHECK_EQ
#define RBE_CHECK_EQ(lhs, rhs)                                                                                         \
  do {                                                                                                                 \
    if consteval {                                                                                                     \
      if (!((lhs) == (rhs))) {                                                                                         \
        throw rbe::compiletime_error(RBE_LOCATION "Compile-time check_eq failed: " #lhs " == " #rhs);                  \
      }                                                                                                                \
    }                                                                                                                  \
    else {                                                                                                             \
      CHECK_EQ(lhs, rhs);                                                                                              \
    }                                                                                                                  \
  }                                                                                                                    \
  while (false)

// 6. RBE_CHECK_FALSE
#define RBE_CHECK_FALSE(...)                                                                                           \
  do {                                                                                                                 \
    if consteval {                                                                                                     \
      if (__VA_ARGS__) {                                                                                               \
        throw rbe::compiletime_error(                                                                                  \
            RBE_LOCATION "Compile-time check_false failed: "                                                           \
                         "expected false for " #__VA_ARGS__                                                            \
        );                                                                                                             \
      }                                                                                                                \
    }                                                                                                                  \
    else {                                                                                                             \
      CHECK_FALSE(__VA_ARGS__);                                                                                        \
    }                                                                                                                  \
  }                                                                                                                    \
  while (false)

#define RBE_FAIL(msg)                                                                                                  \
  do {                                                                                                                 \
    if consteval {                                                                                                     \
      throw rbe::compiletime_error(RBE_LOCATION "Compile-time fail: " #msg);                                           \
    }                                                                                                                  \
    else {                                                                                                             \
      FAIL(msg);                                                                                                       \
    }                                                                                                                  \
  }                                                                                                                    \
  while (false)
