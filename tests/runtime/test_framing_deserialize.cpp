/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_framing_deserialize.cpp
 * @date 07/10/2026
 * @brief Tests for rbe::deserialize and rbe::try_deserialize over vocabulary frames
 */

// --- Includes ---
#include "common_frame.hpp"
#include "common_serde.hpp"
#include "test_macros.hpp"

#include <rbe/dsrl/deserialize.hpp>
#include <rbe/framing/deserialize.hpp>
#include <rbe/framing/frame.hpp>

// --- STD ---
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>

namespace {

/// 4 byte header, then a u32: 8 bytes in all
using flat = rbe::frame<PlainHeader, std::uint32_t>;
/// The length comes from the header: 3 byte header (type, length), payload up to the declared length
using sized = rbe::frame<FrameLengthHeader, rbe::blob>;

static_assert(std::is_same_v<decltype(rbe::deserialize<flat>(bytes(0))), flat::dsrl_type>);
static_assert(std::is_same_v<decltype(rbe::try_deserialize<flat>(bytes(0))), std::optional<flat::dsrl_type>>);

// The two families of overloads live side by side without overlapping: the framing ones take no strategy, the wirable
// ones always do, and each is constrained to its own kind of type.
static_assert(not rbe::wirable<flat>);
static_assert(not rbe::frame_serder<MessageWithHeader>);

// Written as concepts over T: outside of a template a failed call is a hard error instead of a false requires
template<typename T>
concept deserializable_bare = requires(std::span<std::byte const> b) { rbe::deserialize<T>(b); };
template<typename T>
concept try_deserializable_bare = requires(std::span<std::byte const> b) { rbe::try_deserialize<T>(b); };
template<typename T>
concept deserializable_with_strategy = requires(std::span<std::byte const> b) { rbe::deserialize<T>(b, rbe::dsrl::eager); };
template<typename T>
concept try_deserializable_with_strategy = requires(std::span<std::byte const> b) { rbe::try_deserialize<T>(b, rbe::dsrl::lazy); };

// a frame is viewed without a strategy, and a wirable is never taken for a frame
static_assert(deserializable_bare<flat>);
static_assert(try_deserializable_bare<flat>);
static_assert(not deserializable_bare<MessageWithHeader>);
static_assert(not try_deserializable_bare<MessageWithHeader>);

// a wirable keeps requiring its strategy, and a frame is never taken for a wirable
static_assert(deserializable_with_strategy<MessageWithHeader>);
static_assert(try_deserializable_with_strategy<MessageWithHeader>);
static_assert(not deserializable_with_strategy<flat>);
static_assert(not try_deserializable_with_strategy<flat>);

constexpr auto flat_bytes  = bytes(0x01, 0x00, 0x02, 0x00, 0x11, 0x22, 0x33, 0x44);
constexpr auto sized_bytes = bytes(0x05, 0x05, 0x00, 0xAA, 0xBB);

constexpr auto deserialize_views_the_frame() {
  auto const frame = rbe::deserialize<flat>(flat_bytes);

  RBE_CHECK(frame.length() == 8);
  RBE_CHECK(frame.header().field<"type">() == 1);
  RBE_CHECK(frame.header().field<"sequence">() == 2);
  RBE_CHECK(frame.data() == flat_bytes.data());
}

constexpr auto try_deserialize_views_the_frame() {
  auto const frame = rbe::try_deserialize<sized>(sized_bytes);

  RBE_REQUIRE(frame.has_value());
  RBE_CHECK(frame->length() == 5);
  RBE_CHECK(frame->header().field<"type">() == 5);
  RBE_CHECK(frame->payload().size() == 2);
}

constexpr auto try_deserialize_rejects_a_short_buffer() {
  // the header alone does not fit
  RBE_CHECK_FALSE(rbe::try_deserialize<flat>(std::span<std::byte const> {flat_bytes}.first(3)).has_value());
  // the header fits but declares more than the buffer holds
  RBE_CHECK_FALSE(rbe::try_deserialize<sized>(std::span<std::byte const> {sized_bytes}.first(4)).has_value());
}

constexpr auto both_agree_on_a_valid_buffer() {
  auto const checked   = rbe::try_deserialize<sized>(sized_bytes).value();
  auto const unchecked = rbe::deserialize<sized>(sized_bytes);

  RBE_CHECK(checked.length() == unchecked.length());
  RBE_CHECK(checked.as_span().data() == unchecked.as_span().data());
}

// clang-format off
TEST_SUITE("framing_deserialize") {
  RBE_TEST_CASE("framing_deserialize - deserialize views a buffer as the frame", deserialize_views_the_frame);
  RBE_TEST_CASE("framing_deserialize - try_deserialize views a buffer as the frame", try_deserialize_views_the_frame);
  RBE_TEST_CASE("framing_deserialize - try_deserialize rejects a buffer too short for the frame", try_deserialize_rejects_a_short_buffer);
  RBE_TEST_CASE("framing_deserialize - deserialize and try_deserialize agree on a valid buffer", both_agree_on_a_valid_buffer);
}
// clang-format on

} // namespace
