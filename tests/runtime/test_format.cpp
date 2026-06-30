/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_format.cpp
 * @date 27/06/2026
 * @brief Short description
 *
 * Longer description
 */

// --- Includes ---
#include <rbe/core/fmt.hpp>

// --- Dependencies ---

// --- External dependencies ---
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

// --- STD ---
#include <print>

// --- System ---

namespace {

struct Empty { };
struct B {
  int m0 = 0;
};
struct X {
  int m1 = 1;
};
struct Y {
  int m2 = 2;
};

class Z : public X, public Y {
  int m3 = 3;
  int m4 = 4;
};

struct Inner {
  int x = 10;
  int y = 20;
};

struct Outer {
  Inner inner {};
  int z = 30;
};

struct Deep {
  Outer outer {};
  int w = 40;
};

struct WithVector {
  std::vector<int> values {1, 2, 3};
  int extra = 42;
};

struct WithArray {
  std::array<int, 3> values {1, 2, 3};
  int extra = 42;
};

struct WithSpan {
  std::span<int> values {};
  int extra = 42;
};

struct EmptyBase { };

struct DerivedFromEmpty : public EmptyBase {
  int m0 = 0;
};

struct Base {
  int m0 = 0;
  int m1 = 1;
  int m2 = 2;
};

struct EmptyDerived : Base { };

struct UnnamedMember {
  int _ = 42;
};

struct Bits {
  unsigned int a : 3;
  unsigned int b : 5;
  unsigned int c : 8;
};


TEST_SUITE_BEGIN("Universal formatter");

TEST_CASE("Empty struct") {
  std::string const expected = "Empty {}";
  std::string const result   = std::format("{}", Empty {});
  CHECK_EQ(expected, result);
}

TEST_CASE("B struct") {
  std::string const expected = "B {\n  .m0 = 0,\n}";
  std::string const result   = std::format("{}", B {});
  CHECK_EQ(expected, result);
}

TEST_CASE("X struct") {
  std::string const expected = "X {\n  .m1 = 1,\n}";
  std::string const result   = std::format("{}", X {});
  CHECK_EQ(expected, result);
}

TEST_CASE("Y struct") {
  std::string const expected = "Y {\n  .m2 = 2,\n}";
  std::string const result   = std::format("{}", Y {});
  CHECK_EQ(expected, result);
}

TEST_CASE("Z struct with base classes") {
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
  CHECK_EQ(expected, result);
}

TEST_CASE("Struct with nested struct member") {
  std::string const expected = R"(Outer {
  .inner = Inner {
    .x = 10,
    .y = 20,
  },
  .z = 30,
})";
  std::string const result   = std::format("{}", Outer {});
  CHECK_EQ(expected, result);
}

TEST_CASE("Struct with deeply nested struct members") {
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
  CHECK_EQ(expected, result);
}

TEST_CASE("Struct with range member") {
  std::string const expected = R"(WithVector {
  .values = [1, 2, 3],
  .extra = 42,
})";
  std::string const result   = std::format("{}", WithVector {});
  CHECK_EQ(expected, result);
}

TEST_CASE("Struct with array member") {
  std::string const expected = R"(WithArray {
  .values = [1, 2, 3],
  .extra = 42,
})";
  std::string const result   = std::format("{}", WithArray {});
  CHECK_EQ(expected, result);
}

TEST_CASE("Struct with span member") {
  std::vector<int> vec {1, 2, 3};
  WithSpan ws {
    .values = std::span<int>(vec.data(), vec.size()),
  };

  std::string const expected = R"(WithSpan {
  .values = [1, 2, 3],
  .extra = 42,
})";

  std::string const result = std::format("{}", ws);
  CHECK_EQ(expected, result);
}

TEST_CASE("Struct derived from empty base class") {
  std::string const expected = R"(DerivedFromEmpty {
  EmptyBase {},
  .m0 = 0,
})";
  std::string const result   = std::format("{}", DerivedFromEmpty {});
  CHECK_EQ(expected, result);
}

TEST_CASE("Struct with unnamed member") {
  std::string const expected = R"(UnnamedMember {
  ._ = 42,
})";
  std::string const result   = std::format("{}", UnnamedMember {});
  CHECK_EQ(expected, result);
}

TEST_CASE("Struct derived from non-empty base class") {
  std::string const expected = R"(EmptyDerived {
  Base {
    .m0 = 0,
    .m1 = 1,
    .m2 = 2,
  },
})";
  std::string const result   = std::format("{}", EmptyDerived {});
  CHECK_EQ(expected, result);
}

TEST_CASE("Struct with bit-fields") {
  std::string const expected = R"(Bits {
  .a:3 = 0,
  .b:5 = 0,
  .c:8 = 0,
})";
  std::string const result   = std::format("{}", Bits {});
  CHECK_EQ(expected, result);
}

TEST_SUITE_END();

} // namespace
