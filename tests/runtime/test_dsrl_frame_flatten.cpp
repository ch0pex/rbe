/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_dsrl_frame_flatten.cpp
 * @date 07/10/2026
 * @brief Tests for dsrl::flatten: the headers of every nested frame followed by the innermost payload
 */

// --- Includes ---
#include "common_frame.hpp"
#include "common_serde.hpp"
#include "test_macros.hpp"

#include <rbe/framing/dsrl/flatten.hpp>
#include <rbe/framing/dsrl/frame.hpp>
#include <rbe/framing/dsrl/many.hpp>

// --- STD ---
#include <cstddef>
#include <cstdint>
#include <span>
#include <tuple>
#include <type_traits>

namespace {

using blob = std::span<std::byte const>;

/// One level: PlainHeader (4 bytes) + a static u32
using flat_frame = rbe::dsrl::frame<PlainHeader, std::uint32_t>;
/// Two levels: PlainHeader wraps a frame whose length field bounds it
using inner_frame  = rbe::dsrl::frame<FrameLengthHeader, blob>;
using nested_frame = rbe::dsrl::frame<PlainHeader, inner_frame>;
/// Three levels
using deep_frame = rbe::dsrl::frame<PlainHeader, nested_frame>;
/// A many is not a frame: flatten stops at it
using packet_frame = rbe::dsrl::frame<PlainHeader, rbe::dsrl::many<inner_frame>>;

// type, sequence (LE) | payload
constexpr auto flat_bytes = bytes(0x01, 0x00, 0x02, 0x00, 0x11, 0x22, 0x33, 0x44);
// outer: type 1, seq 2 | inner: type 7, length 5 (3 header + 2 payload), AA BB
constexpr auto nested_bytes = bytes(0x01, 0x00, 0x02, 0x00, 0x07, 0x05, 0x00, 0xAA, 0xBB);
// outermost: type 9, seq 8 | nested
constexpr auto deep_bytes = bytes(0x09, 0x00, 0x08, 0x00, 0x01, 0x00, 0x02, 0x00, 0x07, 0x05, 0x00, 0xAA, 0xBB);
// outer | two inner frames back to back: type 7 "AA BB", type 8 empty
constexpr auto packet_bytes = bytes(0x01, 0x00, 0x02, 0x00, 0x07, 0x05, 0x00, 0xAA, 0xBB, 0x08, 0x03, 0x00);

// ============================================================
// shape: one element per header plus the payload
// ============================================================

static_assert(std::tuple_size_v<decltype(rbe::dsrl::flatten(std::declval<flat_frame>()))> == 2);
static_assert(std::tuple_size_v<decltype(rbe::dsrl::flatten(std::declval<nested_frame>()))> == 3);
static_assert(std::tuple_size_v<decltype(rbe::dsrl::flatten(std::declval<deep_frame>()))> == 4);
static_assert(std::tuple_size_v<decltype(rbe::dsrl::flatten(std::declval<packet_frame>()))> == 2);

// clang-format off
static_assert(std::is_same_v<std::tuple_element_t<0, decltype(rbe::dsrl::flatten(std::declval<nested_frame>()))>, nested_frame::header_return_type>);
static_assert(std::is_same_v<std::tuple_element_t<1, decltype(rbe::dsrl::flatten(std::declval<nested_frame>()))>, inner_frame::header_return_type>);
static_assert(std::is_same_v<std::tuple_element_t<2, decltype(rbe::dsrl::flatten(std::declval<nested_frame>()))>, inner_frame::payload_return_type>);
// clang-format on

// ============================================================
// values
// ============================================================

constexpr auto flat_frame_gives_header_and_payload() {
  auto const frame             = flat_frame::make(flat_bytes).value();
  auto const [header, payload] = frame.flatten();

  RBE_CHECK(header.field<"type">() == 1);
  RBE_CHECK(header.field<"sequence">() == 2);
  RBE_CHECK(payload.size() == 4);
}

constexpr auto nested_frame_gives_every_header_and_the_payload() {
  auto const frame                   = nested_frame::make(nested_bytes).value();
  auto const [outer, inner, payload] = frame.flatten();

  RBE_CHECK(outer.field<"type">() == 1);
  RBE_CHECK(outer.field<"sequence">() == 2);
  RBE_CHECK(inner.field<"type">() == 7);
  RBE_CHECK(inner.field<"length">() == 5);
  RBE_CHECK(payload.size() == 2);
  RBE_CHECK(payload[0] == std::byte {0xAA});
  RBE_CHECK(payload[1] == std::byte {0xBB});
}

constexpr auto flatten_recurses_through_every_level() {
  auto const frame                              = deep_frame::make(deep_bytes).value();
  auto const [outermost, outer, inner, payload] = frame.flatten();

  RBE_CHECK(outermost.field<"type">() == 9);
  RBE_CHECK(outermost.field<"sequence">() == 8);
  RBE_CHECK(outer.field<"type">() == 1);
  RBE_CHECK(inner.field<"type">() == 7);
  RBE_CHECK(payload.size() == 2);
}

constexpr auto flatten_stops_at_a_many_payload() {
  auto const frame            = packet_frame::make(packet_bytes).value();
  auto [header, inner_frames] = frame.flatten();

  RBE_CHECK(header.field<"type">() == 1);
  auto seen = std::size_t {0};
  for (auto const [inner_header, inner_payload]: inner_frames) {
    RBE_CHECK(inner_header.field<"type">() == 7 + seen);
    RBE_CHECK(inner_payload.size() == (seen == 0 ? 2 : 0));
    ++seen;
  }
  RBE_CHECK(seen == 2);
}

constexpr auto member_and_free_flatten_agree() {
  auto const frame                                  = nested_frame::make(nested_bytes).value();
  auto const [outer, inner, payload]                = frame.flatten();
  auto const [free_outer, free_inner, free_payload] = rbe::dsrl::flatten(frame);

  RBE_CHECK(outer.as_span().data() == free_outer.as_span().data());
  RBE_CHECK(inner.as_span().data() == free_inner.as_span().data());
  RBE_CHECK(payload.data() == free_payload.data());
  RBE_CHECK(payload.size() == free_payload.size());
}

// clang-format off
TEST_SUITE("dsrl_frame_flatten") {
  RBE_TEST_CASE("dsrl_frame_flatten - a flat frame gives its header and payload", flat_frame_gives_header_and_payload);
  RBE_TEST_CASE("dsrl_frame_flatten - a nested frame gives every header and the innermost payload", nested_frame_gives_every_header_and_the_payload);
  RBE_TEST_CASE("dsrl_frame_flatten - flatten recurses through every level", flatten_recurses_through_every_level);
  RBE_TEST_CASE("dsrl_frame_flatten - flatten stops at a many payload", flatten_stops_at_a_many_payload);
  RBE_TEST_CASE("dsrl_frame_flatten - the member and the free function agree", member_and_free_flatten_agree);
}
// clang-format on

} // namespace
