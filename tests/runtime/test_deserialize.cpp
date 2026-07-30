/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_serialize.cpp
 * @date 13/07/2026
 * @brief Short description
 *
 * Longer description
 */

// --- Includes ---
#include "common_serde.hpp"
#include "common_structs.hpp"

// --- Dependencies ---
#include <rbe/dsrl/deserialize.hpp>
#include <rbe/srl/serialize.hpp>


// --- External dependencies ---
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>


// --- STD ---

// --- System ---

namespace {

template<rbe::wirable T>
constexpr void test_eager(std::span<std::byte const> input, T const& expected) {
  T output = rbe::deserialize<T>(input, rbe::dsrl::eager);
  CHECK(output == expected);
}

template<rbe::trivially_wirable T>
constexpr void test_inplace(std::span<std::byte const> input, T const& expected) {
  T const& output = rbe::deserialize<T>(input, rbe::dsrl::in_place);
  CHECK(output == expected);
}

template<rbe::wirable T>
constexpr void test_lazy(std::span<std::byte const> input, T const& expected) {
  auto msg = rbe::deserialize<T>(input, rbe::dsrl::lazy);

  template for (constexpr auto member: rbe::detail::nsdm(^^T) | std::ranges::to<rbe::static_array>()) {
    CHECK(msg.template field<std::meta::identifier_of(member)>() == expected.[:member:]);
  }
}

template<typename Test>
constexpr void test_case(Test const& test_case) {
  test_eager(test_case.wire, test_case.structure);
  test_inplace(test_case.wire, test_case.structure);
  test_lazy(test_case.wire, test_case.structure);
}

TEST_SUITE_BEGIN("Deserialization");

TEST_CASE("Deserialize trivial") {
  test_case(trivially_wirable_no_padding);
  test_case(trivially_wirable_with_paddings);
}

TEST_SUITE_END();

} // namespace
