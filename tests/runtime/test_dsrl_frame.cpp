/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_frame_length.cpp
 * @date 13/09/2026
 * @brief Static assertions for dsrl::frame length resolution and the spans derived from it
 */

// --- Includes ---
#include "test_macros.hpp"

#include <rbe/annotations/alignment.hpp>
#include <rbe/annotations/endianness.hpp>
#include <rbe/annotations/length.hpp>
#include <rbe/framing/dsrl/frame.hpp>

// --- STD ---
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>

namespace {

// clang-format off

// ============================================================
// Test headers
// ============================================================
struct [[=rbe::little, =rbe::pack]] PlainHeader {
  std::uint16_t type;
  std::uint16_t sequence;
};

struct [[=rbe::little, =rbe::pack]] FrameLengthHeader {
  std::uint8_t type;
  [[=rbe::frame_length]] std::uint16_t length;
};

struct [[=rbe::little, =rbe::pack]] PayloadLengthHeader {
  [[=rbe::payload_length]] std::uint16_t length;
};

struct [[=rbe::little, =rbe::pack]] HeaderLengthHeader {
  [[=rbe::header_length]] std::uint8_t length;
  std::uint8_t flags;
};

// clang-format on

using blob                        = std::span<std::byte const>;
inline constexpr auto buffer_size = std::size_t {32};

constexpr auto buffer(std::initializer_list<std::uint8_t> const prefix) -> std::array<std::byte, buffer_size> {
  auto result = std::array<std::byte, buffer_size> {};
  std::ranges::fill(result, std::byte {0xCC});
  std::ranges::transform(prefix, result.begin(), [](std::uint8_t const b) { return std::byte {b}; });
  return result;
}


// ============================================================
// payload extent: payload_length() resolves from the same classification as rbe::self_delimiting_frame
// ============================================================
using rbe::detail::payload_extent;
using rbe::detail::payload_extent_of;
using rbe::dsrl::frame;

static_assert(payload_extent_of<PayloadLengthHeader, blob>() == payload_extent::payload_length_field);
static_assert(payload_extent_of<FrameLengthHeader, blob>() == payload_extent::frame_length_field);
static_assert(payload_extent_of<PlainHeader, frame<FrameLengthHeader, blob>>() == payload_extent::nested_frame);
static_assert(payload_extent_of<PlainHeader, std::uint32_t>() == payload_extent::static_size);
static_assert(payload_extent_of<PlainHeader, blob>() == payload_extent::buffer_end);
// a length field takes precedence over the static size of the payload
static_assert(payload_extent_of<PayloadLengthHeader, std::uint32_t>() == payload_extent::payload_length_field);

// ============================================================
// frame_length: frame_length = wire_size_of<hdr> + wire_size_of<payload>
// ============================================================

constexpr auto static_sizes() {
  auto const plain_buffer = buffer({0x01, 0x00, 0x02, 0x00});
  auto const plain        = rbe::dsrl::frame<PlainHeader, std::uint32_t> {plain_buffer};

  RBE_CHECK(plain.header_length() == 4);
  RBE_CHECK(plain.payload_length() == 4);
  RBE_CHECK(plain.length() == 8);
  RBE_CHECK(plain.header_span().size() == 4);
  RBE_CHECK(plain.payload_span().size() == 4);
  RBE_CHECK(plain.as_span().size() == 8);
}


// ============================================================
// frame_length: payload_length = buffer.size() - wire_size_of<hdr>
// ============================================================

constexpr auto no_annotations_blob() {
  auto const plain_buffer = buffer({0x01, 0x00, 0x02, 0x00});
  auto const greedy       = rbe::dsrl::frame<PlainHeader, blob> {plain_buffer};
  RBE_CHECK(greedy.payload_length() == buffer_size - greedy.header_length());
  RBE_CHECK(greedy.length() == buffer_size);
  RBE_CHECK(greedy.as_span().size() == buffer_size);
}


// ============================================================
// frame_length: payload_length = frame_length - header_length
// ============================================================

constexpr auto frame_hdr_length() {
  auto const frame_length_buffer = buffer({0x07, 0x0A, 0x00});
  auto const with_frame_length   = rbe::dsrl::frame<FrameLengthHeader, blob> {frame_length_buffer};

  RBE_CHECK(with_frame_length.header_length() == 3);
  RBE_CHECK(with_frame_length.length() == 10);
  RBE_CHECK(with_frame_length.payload_length() == 7);
  RBE_CHECK(with_frame_length.payload_span().size() == 7);
  RBE_CHECK(with_frame_length.payload_span().data() == with_frame_length.data() + 3);
  RBE_CHECK(with_frame_length.as_span().size() == 10);
}

constexpr auto frame_length_hdr_plus_payload() {
  auto const payload_length_buffer = buffer({0x05, 0x00});
  auto const with_payload_length   = rbe::dsrl::frame<PayloadLengthHeader, blob> {payload_length_buffer};

  RBE_CHECK(with_payload_length.header_length() == 2);
  RBE_CHECK(with_payload_length.payload_length() == 5);
  RBE_CHECK(with_payload_length.length() == 7);
}

// ============================================================
// header_length: the payload starts after the annotated header length, not the static header size
// ============================================================

constexpr auto frame_payload_starts_after_header_length() {
  auto const header_length_buffer = buffer({0x06, 0x00});
  auto const with_header_length   = rbe::dsrl::frame<HeaderLengthHeader, std::uint32_t> {header_length_buffer};

  RBE_CHECK(with_header_length.header_length() == 6);
  RBE_CHECK(with_header_length.header_span().size() == 6);
  RBE_CHECK(with_header_length.payload_span().data() == with_header_length.data() + 6);
  RBE_CHECK(with_header_length.payload_length() == 4);
  RBE_CHECK(with_header_length.length() == 10);
}

// ============================================================
// nested frame: the outer payload narrows to the inner frame length
// ============================================================

constexpr auto frame_nested_frame_length() {
  using inner_frame = rbe::dsrl::frame<FrameLengthHeader, blob>;

  auto const nested_buffer = buffer({0x01, 0x00, 0x02, 0x00, 0x07, 0x0A, 0x00});
  auto const nested        = rbe::dsrl::frame<PlainHeader, inner_frame> {nested_buffer};

  RBE_CHECK(nested.payload_length() == 10);
  RBE_CHECK(nested.length() == 14);
  RBE_CHECK(nested.payload_span().size() == 10);
  RBE_CHECK(inner_frame {nested.payload_span()}.payload_span().size() == 7);
}

// ============================================================
// length_of: resolved from a partially received buffer, without constructing the frame
// ============================================================

constexpr auto frame_length_of_partial_buffer() {
  using B = std::byte;

  auto const frame_length_prefix   = std::array {B {0x07}, B {0x0A}, B {0x00}};
  auto const payload_length_prefix = std::array {B {0x05}, B {0x00}};
  auto const plain_prefix          = std::array {B {0x01}, B {0x00}, B {0x02}, B {0x00}};
  auto const nested_prefix         = std::array {B {0x01}, B {0x00}, B {0x02}, B {0x00}, B {0x07}, B {0x0A}, B {0x00}};

  // only the fixed-size header prefix is needed for length fields and static sizes
  RBE_CHECK(rbe::dsrl::frame<FrameLengthHeader, blob>::parse_length(frame_length_prefix) == 10);
  RBE_CHECK(rbe::dsrl::frame<PayloadLengthHeader, blob>::parse_length(payload_length_prefix) == 7);
  RBE_CHECK(rbe::dsrl::frame<PlainHeader, std::uint32_t>::parse_length(plain_prefix) == 8);
  // a nested frame payload also needs the nested header
  RBE_CHECK(rbe::dsrl::frame<PlainHeader, rbe::dsrl::frame<FrameLengthHeader, blob>>::parse_length(nested_prefix) == 14);
  // a buffer-delimited frame has no length of its own: it is the whole buffer
  RBE_CHECK(rbe::dsrl::frame<PlainHeader, blob>::parse_length(buffer({0x01, 0x00, 0x02, 0x00})) == buffer_size);
}

// ============================================================
// construction: the span is narrowed to exactly the frame
// ============================================================

constexpr auto frame_narrows_at_construction() {
  auto const larger     = buffer({0x07, 0x0A, 0x00}); // buffer_size bytes, the frame is 10
  auto const with_frame = rbe::dsrl::frame<FrameLengthHeader, blob> {larger};

  RBE_CHECK(with_frame.length() == 10);
  RBE_CHECK(with_frame.as_span().size() == 10);
  RBE_CHECK(with_frame.as_span().data() == larger.data());
  RBE_CHECK(with_frame.payload_span().size() == 7);
  RBE_CHECK(with_frame.length() == rbe::dsrl::frame<FrameLengthHeader, blob>::parse_length(larger));
}

// clang-format off
TEST_SUITE("dsrl_frame - length and buffer accessors") {
  RBE_TEST_CASE("dsrl_frame - length and buffer: length_of works over a partially received buffer", frame_length_of_partial_buffer);
  RBE_TEST_CASE("dsrl_frame - length and buffer: construction narrows the span to the frame", frame_narrows_at_construction);
  RBE_TEST_CASE("dsrl_frame - length and buffer: static sizes", static_sizes);
  RBE_TEST_CASE("dsrl_frame - length and buffer: no annotations blob", no_annotations_blob);
  RBE_TEST_CASE("dsrl_frame - length and buffer: payload_length = frame_length - header_length", frame_hdr_length);
  RBE_TEST_CASE("dsrl_frame - length and buffer: frame_length = header_length + payload_length", frame_length_hdr_plus_payload);
  RBE_TEST_CASE("dsrl_frame - length and buffer: payload starts after the annotated header length, not the static header size", frame_payload_starts_after_header_length);
  RBE_TEST_CASE("dsrl_frame - length and buffer: the outer payload narrows to the inner frame length", frame_nested_frame_length);
}
// clang-format on


} // namespace
