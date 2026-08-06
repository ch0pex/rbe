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

  if constexpr (rbe::trivially_wirable<typename Test::structure_type>) {
    test_inplace(test_case.wire, test_case.structure);
  }

  if constexpr (not rbe::custom_wirable<typename Test::structure_type>) {
    test_lazy(test_case.wire, test_case.structure);
  }
}

TEST_SUITE_BEGIN("Deserialization");

TEST_CASE("Deserialize trivially no padding") { test_case(trivially_wirable_no_padding); }
TEST_CASE("Deserialize trivially with paddings") { test_case(trivially_wirable_with_paddings); }
TEST_CASE("Deserialize custom serder") { test_case(wirable_custom_serder); }
TEST_CASE("Deserialize packed") { test_case(packed_test); }
TEST_CASE("Deserialize mixed endian") { test_case(mixed_endian_test); }
TEST_CASE("Deserialize message with header") { test_case(message_with_header_test); }
TEST_CASE("Deserialize header pack big endian") { test_case(common_header_pack_be_test); }
TEST_CASE("Deserialize msg-hdr pack be") { test_case(message_with_header_pack_be_test); }
TEST_CASE("Deserialize hdr pack") { test_case(common_header_pack_test); }
TEST_CASE("Deserialize msg hdr pack") { test_case(message_with_header_pack_test); }
TEST_CASE("Deserialize header nsdm be") { test_case(common_header_member_be_test); }
TEST_CASE("Deserialize msg - header nsdm be") { test_case(message_with_header_member_be_test); }
TEST_CASE("Deserialize msg c array") { test_case(message_with_c_array_test); }
TEST_CASE("Deserialize msg c++ array") { test_case(message_with_array_test); }
TEST_CASE("Deserialize msg c array be") { test_case(message_with_array_be_test); }
TEST_CASE("Deserialize") {
  // test_case(no_aggregate_test);
}

TEST_SUITE_END();

} // namespace
