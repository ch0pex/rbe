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
#include "common_frame.hpp"
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
  auto const plain        = rbe::dsrl::frame<PlainHeader, std::uint32_t>::make(plain_buffer).value();

  RBE_CHECK(plain.header().length() == 4);
  RBE_CHECK(plain.payload().size() == 4);
  RBE_CHECK(plain.length() == 8);
  RBE_CHECK(plain.header().as_span().size() == 4);
  RBE_CHECK(plain.payload().as_span().size() == 4);
  RBE_CHECK(plain.as_span().size() == 8);
}


// ============================================================
// frame_length: payload_length = buffer.size() - wire_size_of<hdr>
// ============================================================

constexpr auto no_annotations_blob() {
  auto const plain_buffer = buffer({0x01, 0x00, 0x02, 0x00});
  auto const greedy       = rbe::dsrl::frame<PlainHeader, blob> {plain_buffer};
  RBE_CHECK(greedy.payload().size() == buffer_size - greedy.header().length());
  RBE_CHECK(greedy.length() == buffer_size);
  RBE_CHECK(greedy.as_span().size() == buffer_size);
}


// ============================================================
// frame_length: payload_length = frame_length - header_length
// ============================================================

constexpr auto frame_hdr_length() {
  auto const frame_length_buffer = buffer({0x07, 0x0A, 0x00});
  auto const with_frame_length   = rbe::dsrl::frame<FrameLengthHeader, blob>::make(frame_length_buffer).value();

  RBE_CHECK(with_frame_length.header().length() == 3);
  RBE_CHECK(with_frame_length.length() == 10);
  RBE_CHECK(with_frame_length.payload().size() == 7);
  RBE_CHECK(with_frame_length.payload().data() == with_frame_length.data() + 3);
  RBE_CHECK(with_frame_length.as_span().size() == 10);
}

constexpr auto frame_length_hdr_plus_payload() {
  auto const payload_length_buffer = buffer({0x05, 0x00});
  auto const with_payload_length   = rbe::dsrl::frame<PayloadLengthHeader, blob>::make(payload_length_buffer).value();

  RBE_CHECK(with_payload_length.header().length() == 2);
  RBE_CHECK(with_payload_length.payload().size() == 5);
  RBE_CHECK(with_payload_length.length() == 7);
}

// ============================================================
// header_length: the payload starts after the annotated header length, not the static header size
// ============================================================

constexpr auto frame_payload_starts_after_header_length() {
  auto const header_length_buffer = buffer({0x06, 0x00});
  auto const with_header_length =
      rbe::dsrl::frame<HeaderLengthHeader, std::uint32_t>::make(header_length_buffer).value();

  RBE_CHECK(with_header_length.header().length() == 6);
  RBE_CHECK(with_header_length.header().as_span().size() == 6);
  RBE_CHECK(with_header_length.payload().as_span().data() == with_header_length.data() + 6);
  RBE_CHECK(with_header_length.payload().size() == 4);
  RBE_CHECK(with_header_length.length() == 10);
}

// ============================================================
// header_proxy spans: the logical extent the wire declares, the physical extent the header type maps
// onto, and the trailing bytes the type does not account for
// ============================================================

constexpr auto header_proxy_extended_spans() {
  auto const extended_buffer = buffer({0x06, 0xAB});
  auto const hdr = rbe::dsrl::frame<HeaderLengthHeader, std::uint32_t>::make(extended_buffer).value().header();

  RBE_CHECK(hdr.length() == 6); // logical: what the wire declares the header occupies
  RBE_CHECK(hdr.size() == 2);   // physical: wire_size_of<HeaderLengthHeader>
  RBE_CHECK(hdr.is_extended());

  RBE_CHECK(hdr.as_span().size() == 6);
  RBE_CHECK(hdr.known_span().size() == 2);
  RBE_CHECK(hdr.known_span().data() == hdr.as_span().data());

  // the four bytes a newer peer appended, which HeaderLengthHeader has no field for
  RBE_CHECK(hdr.extension_span().size() == 4);
  RBE_CHECK(hdr.extension_span().data() == hdr.data() + hdr.size());

  // the fields the type does know about still decode out of the known part
  RBE_CHECK(hdr.field<"flags">() == 0xAB);
}

constexpr auto header_proxy_unextended_spans() {
  auto const exact_buffer = buffer({0x02, 0xAB});
  auto const hdr = rbe::dsrl::frame<HeaderLengthHeader, std::uint32_t>::make(exact_buffer).value().header();

  RBE_CHECK_FALSE(hdr.is_extended());
  RBE_CHECK(hdr.as_span().size() == hdr.known_span().size());
  RBE_CHECK(hdr.extension_span().empty());
}

// ============================================================
// nested frame: the outer payload narrows to the inner frame length
// ============================================================

constexpr auto frame_nested_frame_length() {
  using inner_frame = rbe::dsrl::frame<FrameLengthHeader, blob>;

  auto const nested_buffer = buffer({0x01, 0x00, 0x02, 0x00, 0x07, 0x0A, 0x00});
  auto const nested        = rbe::dsrl::frame<PlainHeader, inner_frame>::make(nested_buffer).value();

  RBE_CHECK(nested.payload().length() == 10);
  RBE_CHECK(nested.length() == 14);
  RBE_CHECK(nested.payload().as_span().size() == 10);
  RBE_CHECK(inner_frame {nested.payload().as_span()}.payload().size() == 7);
}

// ============================================================
// construction: the span is narrowed to exactly the frame
// ============================================================

constexpr auto frame_narrows_at_construction() {
  auto const larger     = buffer({0x07, 0x0A, 0x00}); // buffer_size bytes, the frame is 10
  auto const with_frame = rbe::dsrl::frame<FrameLengthHeader, blob>::make(larger).value();

  RBE_CHECK(with_frame.length() == 10);
  RBE_CHECK(with_frame.as_span().size() == 10);
  RBE_CHECK(with_frame.as_span().data() == larger.data());
  RBE_CHECK(with_frame.payload().size() == 7);
}

constexpr auto frame_with_empty_payload() {
  std::array<std::byte, 2> empty_payload_buffer {std::byte {0x00}, std::byte {0x00}};
  auto const with_empty_payload = rbe::dsrl::frame<PayloadLengthHeader, ExplictlyEmpty> {empty_payload_buffer};

  RBE_CHECK(with_empty_payload.header().length() == 2);
  RBE_CHECK(with_empty_payload.payload().size() == 0);
  RBE_CHECK(with_empty_payload.length() == 2);
  RBE_CHECK(with_empty_payload.payload().as_span().size() == 0);
  RBE_CHECK(with_empty_payload.as_span().size() == 2);

  std::array<std::byte, 4> empty_payload_buffer2 {
    std::byte {0x00}, std::byte {0x00}, std::byte {0xCC}, std::byte {0xCC}
  };
  auto const with_empty_payload2 = rbe::dsrl::frame<PlainHeader, ExplictlyEmpty> {empty_payload_buffer2};

  RBE_CHECK(with_empty_payload2.header().length() == 4);
  RBE_CHECK(with_empty_payload2.payload().size() == 0);
  RBE_CHECK(with_empty_payload2.length() == 4);
  RBE_CHECK(with_empty_payload2.payload().as_span().size() == 0);
  RBE_CHECK(with_empty_payload2.as_span().size() == 4);
}

// constexpr auto frame_message_id_any_doesnt_fit() {
//   auto buff        = buffer({0x01, 0x00, 0x00, 0x00, 0xDD, 0xCC, 0xBB, 0xAA});
//   using frame_type = rbe::dsrl::frame<MessageIdHeader, rbe::dsrl::any<msg_1, msg_2>>;
//   RBE_CHECK(frame_type::make_length(buff) == rbe::wire_size_of<MessageIdHeader>() + rbe::wire_size_of<msg_1>());
//   // msg_1 doesn't fit in the buffer, so the frame cannot be constructed
//   RBE_CHECK_FALSE(frame_type::make(buff).has_value());
// }

// clang-format off
TEST_SUITE("dsrl_frame - length and buffer accessors") {
  RBE_TEST_CASE("dsrl_frame - length and buffer: construction narrows the span to the frame", frame_narrows_at_construction);
  RBE_TEST_CASE("dsrl_frame - length and buffer: static sizes", static_sizes);
  RBE_TEST_CASE("dsrl_frame - length and buffer: no annotations blob", no_annotations_blob);
  RBE_TEST_CASE("dsrl_frame - length and buffer: payload_length = frame_length - header_length", frame_hdr_length);
  RBE_TEST_CASE("dsrl_frame - length and buffer: frame_length = header_length + payload_length", frame_length_hdr_plus_payload);
  RBE_TEST_CASE("dsrl_frame - length and buffer: payload starts after the annotated header length, not the static header size", frame_payload_starts_after_header_length);
  RBE_TEST_CASE("dsrl_frame - length and buffer: an extended header exposes its logical, known and extension spans", header_proxy_extended_spans);
  RBE_TEST_CASE("dsrl_frame - length and buffer: an unextended header has no extension bytes", header_proxy_unextended_spans);
  RBE_TEST_CASE("dsrl_frame - length and buffer: the outer payload narrows to the inner frame length", frame_nested_frame_length);
  RBE_TEST_CASE("dsrl_frame - length and buffer: frame with empty payload", frame_with_empty_payload);
  // RBE_TEST_CASE("dsrl_frame - length and buffer: frame with message_id and any payload doesn't fit", frame_message_id_any_doesnt_fit);
}
// clang-format on


} // namespace
