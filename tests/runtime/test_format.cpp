/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_format.cpp
 * @date 27/06/2026
 * @brief Doctest suite for the universal std::format formatter
 */

// --- Includes ---
#include "common_structs.hpp"

#include <rbe/core/fmt.hpp>

// --- External dependencies ---
#include "test_macros.hpp"

// --- STD ---
#include <print>

// --- System ---

namespace {

TEST_SUITE_BEGIN("format");

TEST_CASE("format - empty struct") {
  std::string const expected = "Empty {}";
  std::string const result   = std::format("{}", Empty {});
  RBE_CHECK_EQ(expected, result);
}

TEST_CASE("format - B struct") {
  std::string const expected = "B {\n  .m0 = 0,\n}";
  std::string const result   = std::format("{}", B {});
  RBE_CHECK_EQ(expected, result);
}

TEST_CASE("format - X struct") {
  std::string const expected = "X {\n  .m1 = 1,\n}";
  std::string const result   = std::format("{}", X {});
  RBE_CHECK_EQ(expected, result);
}

TEST_CASE("format - Y struct") {
  std::string const expected = "Y {\n  .m2 = 2,\n}";
  std::string const result   = std::format("{}", Y {});
  RBE_CHECK_EQ(expected, result);
}

TEST_CASE("format - struct with base classes (Z)") {
  std::string const expected = R"(Z {
  X {
    .m1 = 1,
  },
  Y {
    .m2 = 2,
  },
  .m3 = 3,
  .m4 = 4,
})";
  std::string const result   = std::format("{}", Z {});
  RBE_CHECK_EQ(expected, result);
}

TEST_CASE("format - struct with nested struct member") {
  std::string const expected = R"(Outer {
  .inner = Inner {
    .x = 10,
    .y = 20,
  },
  .z = 30,
})";
  std::string const result   = std::format("{}", Outer {});
  RBE_CHECK_EQ(expected, result);
}

TEST_CASE("format - struct with deeply nested struct members") {
  std::string const expected = R"(Deep {
  .outer = Outer {
    .inner = Inner {
      .x = 10,
      .y = 20,
    },
    .z = 30,
  },
  .w = 40,
})";
  std::string const result   = std::format("{}", Deep {});
  RBE_CHECK_EQ(expected, result);
}

TEST_CASE("format - struct with range member") {
  std::string const expected = R"(WithVector {
  .values = [1, 2, 3],
  .extra = 42,
})";
  std::string const result   = std::format("{}", WithVector {});
  RBE_CHECK_EQ(expected, result);
}

TEST_CASE("format - struct with array member") {
  std::string const expected = R"(WithArray {
  .values = [1, 2, 3],
  .extra = 42,
})";
  std::string const result   = std::format("{}", WithArray {});
  RBE_CHECK_EQ(expected, result);
}

TEST_CASE("format - struct with span member") {
  std::vector<int> vec {1, 2, 3};
  WithSpan ws {
    .values = std::span<int>(vec.data(), vec.size()),
  };

  std::string const expected = R"(WithSpan {
  .values = [1, 2, 3],
  .extra = 42,
})";

  std::string const result = std::format("{}", ws);
  RBE_CHECK_EQ(expected, result);
}

TEST_CASE("format - struct derived from empty base class") {
  std::string const expected = R"(DerivedFromEmpty {
  EmptyBase {},
  .m0 = 0,
})";
  std::string const result   = std::format("{}", DerivedFromEmpty {});
  RBE_CHECK_EQ(expected, result);
}

TEST_CASE("format - struct with unnamed member") {
  std::string const expected = R"(UnnamedMember {
  ._ = 42,
})";
  std::string const result   = std::format("{}", UnnamedMember {});
  RBE_CHECK_EQ(expected, result);
}

TEST_CASE("format - struct derived from non-empty base class") {
  std::string const expected = R"(EmptyDerived {
  Base {
    .m0 = 0,
    .m1 = 1,
    .m2 = 2,
  },
})";
  std::string const result   = std::format("{}", EmptyDerived {});
  RBE_CHECK_EQ(expected, result);
}

TEST_CASE("format - struct with bit-fields") {
  std::string const expected = R"(Bits {
  .a:3 = 0,
  .b:5 = 0,
  .c:8 = 0,
})";
  std::string const result   = std::format("{}", Bits {});
  RBE_CHECK_EQ(expected, result);
}

TEST_SUITE_END();

} // namespace
