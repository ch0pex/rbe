/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_serde.cpp
 * @date 13/07/2026
 * @brief Doctest suite for serialize/deserialize round-trip across eager, lazy and in-place strategies
 */

// --- Includes ---
#include "common_serde.hpp"
#include "common_structs.hpp"

#include <rbe/core/detail/context.hpp>
#include <rbe/core/detail/introspection.hpp>
#include <rbe/core/detail/static_array.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/core/trivially_wirable_concepts.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/core/wirable_primitives.hpp>
#include <rbe/dsrl/deserialize.hpp>
#include <rbe/dsrl/proxy.hpp>
#include <rbe/dsrl/tags.hpp>
#include <rbe/srl/serialize.hpp>

// --- External dependencies ---
#include <doctest/doctest.h>

// --- STD ---
#include <ranges>

namespace {

template<typename T>
auto testing_buffer() -> std::array<std::byte, rbe::wire_size_of<T>()> {
  std::array<std::byte, rbe::wire_size_of<T>()> result {};
  std::ranges::fill(result, std::byte {0xCC});
  return result;
}

namespace dsrl {

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

  CHECK(msg.value() == expected);
  CHECK(msg.length() == rbe::wire_size_of<T>());
  CHECK(msg.size() == input.size());
  CHECK(msg.size_bytes() == input.size());
  CHECK(std::ranges::equal(msg.data(), input));
  CHECK(std::ranges::equal(msg.as_span(), input.first(msg.length())));
}

template<typename Test>
constexpr void test_case(Test const& test_case) {
  REQUIRE(test_case.wire.size() == rbe::wire_size_of<typename Test::structure_type>());
  test_eager(test_case.wire, test_case.structure);

  if constexpr (rbe::trivially_wirable<typename Test::structure_type>) {
    test_inplace(test_case.wire, test_case.structure);
  }

  if constexpr (not rbe::custom_wirable<typename Test::structure_type>) {
    test_lazy(test_case.wire, test_case.structure);
  }
}

} // namespace dsrl

namespace srl {

template<typename Test>
constexpr void test_case(Test const& test_case) {
  auto buffer = testing_buffer<typename Test::structure_type>();
  REQUIRE(test_case.wire.size() == rbe::wire_size_of<typename Test::structure_type>());

  auto bytes_written = rbe::serialize(buffer, test_case.structure);
  CHECK(bytes_written == test_case.wire.size());
  CHECK(std::ranges::equal(buffer, test_case.wire, ignore_padding));
}

} // namespace srl

namespace round_trip {


template<rbe::wirable T>
constexpr auto test_eager(T const& value) -> void {
  auto buffer = testing_buffer<T>();

  auto bytes_written = rbe::serialize(buffer, value);
  T output           = rbe::deserialize<T>(buffer, rbe::dsrl::eager);

  CHECK(bytes_written == buffer.size());
  CHECK(output == value);
}

template<rbe::trivially_wirable T>
constexpr auto test_inplace(T const& value) -> void {
  auto buffer = testing_buffer<T>();

  auto bytes_written = rbe::serialize(buffer, value);
  T const& output    = rbe::deserialize<T>(buffer, rbe::dsrl::in_place);

  CHECK(bytes_written == buffer.size());
  CHECK(output == value);
}

template<rbe::wirable T>
constexpr auto test_lazy(T const& value) -> void {
  auto buffer = testing_buffer<T>();

  auto bytes_written = rbe::serialize(buffer, value);
  auto output        = rbe::deserialize<T>(buffer, rbe::dsrl::lazy);

  CHECK(bytes_written == buffer.size());
  CHECK(output.value() == value);
  template for (constexpr auto member: rbe::detail::nsdm(^^T) | std::ranges::to<rbe::static_array>()) {
    CHECK(output.template field<std::meta::identifier_of(member)>() == value.[:member:]);
  }
}

template<rbe::wirable T>
constexpr void test_case(T const& value) {
  test_eager(value);

  if constexpr (rbe::trivially_wirable<T>) {
    test_inplace(value);
  }

  if constexpr (not rbe::custom_wirable<T>) {
    test_lazy(value);
  }
}

} // namespace round_trip

#define SERDE_TEST_CASE(test_case_struct)                                                                              \
  TEST_CASE("serde - " #test_case_struct " [dsrl]") { dsrl::test_case(test_case_struct); }                             \
  TEST_CASE("serde - " #test_case_struct " [srl]") { srl::test_case(test_case_struct); }                               \
  TEST_CASE("serde - " #test_case_struct " [round_trip]") { round_trip::test_case(test_case_struct.structure); }

// Golden source testing for serialization, deserialization, and round-trip cycles
TEST_SUITE_BEGIN("serde");

SERDE_TEST_CASE(trivially_wirable_no_padding);
SERDE_TEST_CASE(trivially_wirable_with_paddings);
SERDE_TEST_CASE(wirable_custom_serder);
SERDE_TEST_CASE(packed_test);
SERDE_TEST_CASE(mixed_endian_test);
SERDE_TEST_CASE(message_with_header_test);
SERDE_TEST_CASE(common_header_pack_be_test);
SERDE_TEST_CASE(message_with_header_pack_be_test);
SERDE_TEST_CASE(common_header_pack_test);
SERDE_TEST_CASE(message_with_header_pack_test);
SERDE_TEST_CASE(common_header_member_be_test);
SERDE_TEST_CASE(message_with_header_member_be_test);
SERDE_TEST_CASE(message_with_c_array_test);
SERDE_TEST_CASE(message_with_array_test);
SERDE_TEST_CASE(message_with_array_be_test);
SERDE_TEST_CASE(nested_propagation_test);
SERDE_TEST_CASE(nested_pack_propagation_test);

// Every accessor must decode under the proxy's own context, not just field<>(): a proxy built with
// an ambient big-endian/packed context sees a 5 byte packed message, not the 8 byte native layout.
TEST_CASE("serde - proxy accessors honor the ambient context") {
  constexpr auto big_packed = rbe::detail::context {rbe::endian::order::big, rbe::alignment_mode::pack};
  constexpr auto wire       = bytes(0x11, 0x22, 0x33, 0x44, 0x55, pad, pad, pad);

  rbe::dsrl::proxy<NestedPackLeaf, big_packed> const view {wire};

  CHECK(view.field<"a">() == 0x11);
  CHECK(view.field<"b">() == 0x22334455);
  CHECK(view.value() == NestedPackLeaf {.a = 0x11, .b = 0x22334455});

  CHECK(view.length() == 5); // packed: the 3 bytes of padding a native layout would add are gone
  CHECK(view.as_span().size() == 5);
  CHECK(view.size() == wire.size()); // size is the buffer's, not the message's
  CHECK(view.size_bytes() == wire.size());
  CHECK(std::ranges::equal(view.data(), wire));
}

TEST_SUITE_END();

} // namespace
