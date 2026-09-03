/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_static_array.cpp
 * @date 08/07/2026
 * @brief Doctest suite for rbe::static_array
 */

// --- Includes ---
#include <rbe/core/detail/static_array.hpp>

// --- Dependencies ---

// --- External dependencies ---
#include <doctest/doctest.h>

// --- STD ---
#include <ranges>

// --- System ---

struct SomeData {
  int a;
  char b;
};

struct SomeNumbers {
  int a;
  int b;
  int c;
  int d;
};

// --- Compile time tests ---

consteval bool construct_default() {
  rbe::static_array<int> arr;
  static_assert(std::is_same_v<decltype(arr), rbe::static_array<int>>);
  return arr.size() == 0 and arr.empty();
}

consteval bool construct_with_vec() {
  std::vector<int> vec {1, 2, 3, 4, 5};
  rbe::static_array<int> arr {std::from_range, vec};
  static_assert(std::is_same_v<decltype(arr), rbe::static_array<int>>);
  return arr.size() == 5 and arr[0] == 1 and arr[1] == 2 and arr[2] == 3 and arr[3] == 4 and arr[4] == 5;
}

consteval bool construct_with_array() {
  std::array<int, 5> arr_data {1, 2, 3, 4, 5};
  rbe::static_array arr {std::from_range, arr_data};
  static_assert(std::is_same_v<decltype(arr), rbe::static_array<int>>);
  return arr.size() == 5 and arr[0] == 1 and arr[1] == 2 and arr[2] == 3 and arr[3] == 4 and arr[4] == 5;
}

consteval bool initializer_list() {
  rbe::static_array arr {1, 2, 3, 4, 5};

  return arr.size() == 5 and arr[0] == 1 and arr[1] == 2 and arr[2] == 3 and arr[3] == 4 and arr[4] == 5;
}

consteval bool initializer_list_deduction() {
  rbe::static_array arr {
    SomeData {.a = 1, .b = 'a'},
    SomeData {.a = 2, .b = 'b'},
  };

  static_assert(std::is_same_v<decltype(arr), rbe::static_array<SomeData>>);
  return arr.size() == 2 and arr[0].a == 1 and arr[0].b == 'a' and arr[1].a == 2 and arr[1].b == 'b';
}

consteval bool static_array_random_access() {
  rbe::static_array arr {
    SomeData {.a = 1, .b = 'a'}, SomeData {.a = 2, .b = 'b'}, SomeData {.a = 3, .b = 'c'}, SomeData {.a = 4, .b = 'd'},
    SomeData {.a = 5, .b = 'e'}, SomeData {.a = 6, .b = 'f'}, SomeData {.a = 7, .b = 'g'},
  };

  bool result = arr.at(0).a == 1 and arr.at(1).a == 2 and arr.at(2).a == 3 and arr.at(3).a == 4 and arr.at(4).a == 5 and
                arr.at(5).a == 6 and arr.at(6).a == 7;

  result &= arr[0].a == 1 and arr[1].a == 2 and arr[2].a == 3 and arr[3].a == 4 and arr[4].a == 5 and arr[5].a == 6 and
            arr[6].a == 7 and result;

  result &= arr.front().a == 1 and arr.back().a == 7 and result;

  return result;
}

consteval bool array_iteration() {
  rbe::static_array arr {
    SomeData {.a = 1, .b = 'a'}, SomeData {.a = 2, .b = 'b'}, SomeData {.a = 3, .b = 'c'}, SomeData {.a = 4, .b = 'd'},
    SomeData {.a = 5, .b = 'e'}, SomeData {.a = 6, .b = 'f'}, SomeData {.a = 7, .b = 'g'},
  };

  std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7};

  for (auto const& [idx, item]: arr | std::views::enumerate) {
    if (item.a != idx + 1) {
      return false;
    }
  }

  return true;
}

consteval bool array_reverse_iteration() {
  rbe::static_array arr = {
    SomeData {.a = 1, .b = 'a'}, SomeData {.a = 2, .b = 'b'}, SomeData {.a = 3, .b = 'c'}, SomeData {.a = 4, .b = 'd'},
    SomeData {.a = 5, .b = 'e'}, SomeData {.a = 6, .b = 'f'}, SomeData {.a = 7, .b = 'g'},
  };

  for (auto const& [idx, item]: arr | std::views::reverse | std::views::enumerate) {
    if (static_cast<std::size_t>(item.a) != arr.size() - static_cast<std::size_t>(idx)) {
      return false;
    }
  }

  return true;
}

consteval bool template_for_iteration() {
  SomeNumbers data {
    .a = 0,
    .b = 1,
    .c = 2,
    .d = 3,
  };

  int counter = 0;

  static constexpr auto ctx     = std::meta::access_context::current();
  static constexpr auto members = nonstatic_data_members_of(^^SomeNumbers, ctx) | std::ranges::to<rbe::static_array>();
  template for (constexpr auto member: members) {
    if (data.[:member:] != counter++) {
      return false;
    }
  }
  return true;
}

consteval bool static_array_first_last_subspan() {
  rbe::static_array arr {1, 2, 3, 4, 5};
  auto first_three = arr.first(3);
  auto last_two    = arr.last(2);
  auto subspan     = arr.subspan(1, 3);

  bool result = true;

  result &= first_three.size() == 3;
  result &= first_three[0] == 1;
  result &= first_three[1] == 2;
  result &= first_three[2] == 3;

  result &= last_two.size() == 2;
  result &= last_two[0] == 4;
  result &= last_two[1] == 5;

  result &= subspan.size() == 3;
  result &= subspan[0] == 2;
  result &= subspan[1] == 3;
  result &= subspan[2] == 4;
  return result;
}


static_assert(construct_default(), "construct_default test failed");
static_assert(construct_with_vec(), "construct_with_vec test failed");
static_assert(construct_with_array(), "construct_with_array test failed");
static_assert(initializer_list(), "initializer_list test failed");
static_assert(initializer_list_deduction(), "initializer_list_deduction test failed");
static_assert(static_array_random_access(), "static_array_random_access test failed");
static_assert(array_iteration(), "array_iteration test failed");
static_assert(array_reverse_iteration(), "array_reverse_iteration test failed");
static_assert(template_for_iteration(), "template_for_iteration test failed");
static_assert(static_array_first_last_subspan(), "static_array_first_last_subspan test failed");

// --- Runtime tests ---

TEST_SUITE_BEGIN("static_array");

TEST_CASE("static_array - default construction") {
  static constexpr rbe::static_array<int> arr;
  CHECK(arr.size() == 0);
  CHECK(arr.empty());
}

TEST_CASE("static_array - construct with vector") {
  constexpr std::array<int, 5> vec_data {1, 2, 3, 4, 5};
  static constexpr rbe::static_array<int> arr {std::from_range, vec_data};
  CHECK(arr.size() == 5);
  CHECK(arr[0] == 1);
  CHECK(arr[1] == 2);
  CHECK(arr[2] == 3);
  CHECK(arr[3] == 4);
  CHECK(arr[4] == 5);
}

TEST_CASE("static_array - construct with array") {
  constexpr std::array<int, 5> arr_data {1, 2, 3, 4, 5};
  static constexpr rbe::static_array arr {std::from_range, arr_data};
  CHECK(arr.size() == 5);
  CHECK(arr[0] == 1);
  CHECK(arr[1] == 2);
  CHECK(arr[2] == 3);
  CHECK(arr[3] == 4);
  CHECK(arr[4] == 5);
}

TEST_CASE("static_array - initializer list construction") {
  static constexpr rbe::static_array arr {1, 2, 3, 4, 5};
  CHECK(arr.size() == 5);
  CHECK(arr[0] == 1);
  CHECK(arr[1] == 2);
  CHECK(arr[2] == 3);
  CHECK(arr[3] == 4);
  CHECK(arr[4] == 5);
}

TEST_CASE("static_array - initializer list deduces element type") {
  static constexpr rbe::static_array arr {
    SomeData {.a = 1, .b = 'a'},
    SomeData {.a = 2, .b = 'b'},
  };
  CHECK(arr.size() == 2);
  CHECK(arr[0].a == 1);
  CHECK(arr[0].b == 'a');
  CHECK(arr[1].a == 2);
  CHECK(arr[1].b == 'b');
}

TEST_CASE("static_array - random access") {
  static constexpr rbe::static_array arr {
    SomeData {.a = 1, .b = 'a'}, SomeData {.a = 2, .b = 'b'}, SomeData {.a = 3, .b = 'c'}, SomeData {.a = 4, .b = 'd'},
    SomeData {.a = 5, .b = 'e'}, SomeData {.a = 6, .b = 'f'}, SomeData {.a = 7, .b = 'g'},
  };

  CHECK(arr.at(0).a == 1);
  CHECK(arr.at(1).a == 2);
  CHECK(arr.at(2).a == 3);
  CHECK(arr.at(3).a == 4);
  CHECK(arr.at(4).a == 5);
  CHECK(arr.at(5).a == 6);
  CHECK(arr.at(6).a == 7);

  CHECK(arr[0].a == 1);
  CHECK(arr[1].a == 2);
  CHECK(arr[2].a == 3);
  CHECK(arr[3].a == 4);
  CHECK(arr[4].a == 5);
  CHECK(arr[5].a == 6);
  CHECK(arr[6].a == 7);

  CHECK(arr.front().a == 1);
  CHECK(arr.back().a == 7);
}

TEST_CASE("static_array - forward iteration") {
  static constexpr rbe::static_array arr {
    SomeData {.a = 1, .b = 'a'}, SomeData {.a = 2, .b = 'b'}, SomeData {.a = 3, .b = 'c'}, SomeData {.a = 4, .b = 'd'},
    SomeData {.a = 5, .b = 'e'}, SomeData {.a = 6, .b = 'f'}, SomeData {.a = 7, .b = 'g'},
  };

  for (auto const& [idx, item]: arr | std::views::enumerate) {
    CHECK(item.a == static_cast<int>(idx) + 1);
  }
}

TEST_CASE("static_array - reverse iteration") {
  static constexpr rbe::static_array arr {
    SomeData {.a = 1, .b = 'a'}, SomeData {.a = 2, .b = 'b'}, SomeData {.a = 3, .b = 'c'}, SomeData {.a = 4, .b = 'd'},
    SomeData {.a = 5, .b = 'e'}, SomeData {.a = 6, .b = 'f'}, SomeData {.a = 7, .b = 'g'},
  };

  for (auto const& [idx, item]: arr | std::views::reverse | std::views::enumerate) {
    CHECK(static_cast<std::size_t>(item.a) == arr.size() - static_cast<std::size_t>(idx));
  }
}

TEST_CASE("static_array - iteration via template for") {
  static constexpr SomeNumbers data {
    .a = 0,
    .b = 1,
    .c = 2,
    .d = 3,
  };

  constexpr static auto ctx     = std::meta::access_context::current();
  constexpr static auto members = nonstatic_data_members_of(^^SomeNumbers, ctx) | std::ranges::to<rbe::static_array>();

  int counter = 0;
  template for (constexpr auto member: members) { CHECK(data.[:member:] == counter++); }
}

TEST_CASE("static_array - first(n)") {
  static constexpr rbe::static_array arr {1, 2, 3, 4, 5};
  static constexpr auto first_three = arr.first(3);

  CHECK(first_three.size() == 3);
  CHECK(first_three[0] == 1);
  CHECK(first_three[1] == 2);
  CHECK(first_three[2] == 3);
}

TEST_CASE("static_array - last(n)") {
  static constexpr rbe::static_array arr {1, 2, 3, 4, 5};
  static constexpr auto last_two = arr.last(2);

  CHECK(last_two.size() == 2);
  CHECK(last_two[0] == 4);
  CHECK(last_two[1] == 5);
}

TEST_CASE("static_array - subspan(offset, count)") {
  static constexpr rbe::static_array arr {1, 2, 3, 4, 5};
  static constexpr auto subspan = arr.subspan(1, 3);

  CHECK(subspan.size() == 3);
  CHECK(subspan[0] == 2);
  CHECK(subspan[1] == 3);
  CHECK(subspan[2] == 4);
}

TEST_SUITE_END();
