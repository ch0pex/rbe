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
#include <rbe/framing/dsrl/base_frame.hpp>
#include <rbe/framing/dsrl/frame.hpp>
#include <rbe/framing/dsrl/many.hpp>
#include <rbe/framing/dsrl/payload.hpp>

// --- STD ---
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
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
// header_proxy spans: the length the wire declares, the size of the header type
// onto, and the trailing bytes the type does not account for
// ============================================================

constexpr auto header_proxy_extended_spans() {
  auto const extended_buffer = buffer({0x06, 0xAB});
  auto const hdr = rbe::dsrl::frame<HeaderLengthHeader, std::uint32_t>::make(extended_buffer).value().header();

  RBE_CHECK(hdr.length() == 6); // length: what the wire declares the header occupies
  RBE_CHECK(hdr.size() == 2); // size: wire_size_of<HeaderLengthHeader>, known to the code
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
  auto const hdr          = rbe::dsrl::frame<HeaderLengthHeader, std::uint32_t>::make(exact_buffer).value().header();

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

constexpr auto frame_payload_bigger_than_wireable() {
  std::array<std::byte, 1500> frame_buffer {
    std::byte {0xFF}, // type
    std::byte {0x10}, std::byte {0x00}, std::byte {0x00}, std::byte {0x00} // frame length = 16
  };
  // 3 bytes + 8 bytes of payload = 11 bytes, but the frame length is 16, so the payload is bigger than
  // the wiarble size. This is a valid scenario rbe must handle, since it's quite common fro a frame
  // to have been extended with new fields even though the code doesn't konw how to decode them.
  using frame_type = rbe::dsrl::frame<FrameLengthHeader, Message>;
  auto const frame = frame_type::make(frame_buffer).value();

  RBE_CHECK(frame.length() == 16);

  RBE_CHECK(frame.header().length() == 3);
  RBE_CHECK(frame.payload().size() == 8);

  RBE_CHECK(frame.payload().as_span().size() == 8);
  RBE_CHECK(frame.as_span().size() == 16);
}


// ============================================================
// buffer() and the derived spans: the untrimmed buffer a frame was handed, as opposed to as_span()
// ============================================================

constexpr auto frame_buffer_is_untrimmed() {
  auto const larger = buffer({0x07, 0x0A, 0x00}); // buffer_size bytes, the frame is 10
  auto const f      = rbe::dsrl::frame<FrameLengthHeader, blob>::make(larger).value();

  RBE_CHECK(f.buffer().size() == buffer_size);
  RBE_CHECK(f.buffer().data() == larger.data());
  RBE_CHECK(f.as_span().size() == 10);
  RBE_CHECK(f.header_span().size() == 3);
  RBE_CHECK(f.header_span().data() == f.data());
  RBE_CHECK(f.payload_span().size() == 7);
  RBE_CHECK(f.payload_span().data() == f.data() + 3);
}

// ============================================================
// the building blocks on their own: a frame is header<H>::make + payload_extent + make_payload
// ============================================================

constexpr auto building_blocks_compose_into_a_frame() {
  auto const wire = buffer({0x00, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC}); // id 0 -> msg_1, 4 bytes
  using any_t     = dsrl::any_test;

  auto const hdr = rbe::dsrl::header<MessageIdHeader>::make(wire).value(); // step 1
  RBE_CHECK(hdr.length() == 4);

  auto const rest  = std::span<std::byte const> {wire}.subspan(hdr.length());
  auto const bytes = rbe::dsrl::try_payload_extent<any_t>(hdr, rest).value(); // step 2: no length field, so the rest
  RBE_CHECK(bytes.size() == rest.size());
  RBE_CHECK(rbe::dsrl::payload_extent<any_t>(hdr, rest).size() == rest.size());

  auto const payload = rbe::dsrl::make_payload<any_t>(hdr, bytes).value(); // step 3: the any narrows to msg_1
  RBE_CHECK(payload.is<msg_1>());
  RBE_CHECK(payload.size() == 4);

  // the same three steps are what frame::make does
  auto const f = rbe::dsrl::frame<MessageIdHeader, any_t>::make(wire).value();
  RBE_CHECK(f.length() == 8);
  RBE_CHECK(f.payload().as_span().data() == payload.as_span().data());

  // a wirable payload comes back as a proxy, an opaque one as the span it was handed
  static_assert(std::same_as<rbe::dsrl::payload_view_t<Message>, rbe::dsrl::proxy<Message>>);
  static_assert(std::same_as<rbe::dsrl::payload_view_t<blob>, blob>);
  RBE_CHECK(rbe::dsrl::make_payload<blob>(hdr, rest).value().size() == rest.size());
  RBE_CHECK(not rbe::dsrl::make_payload<Message>(hdr, rest.first(3)).has_value()); // 8 bytes do not fit in 3
}

// ============================================================
// a custom frame on base_frame: swaps step 2 (the extent) and reuses steps 1 and 3 untouched
// ============================================================

/// MessageIdHeader followed by a candidate, each record terminated by one or more '\n' bytes. The line
/// extent comes from the terminator alone, never from the id, which is what lets a many step over a
/// record whose id this build does not know instead of stopping there.
class line_view : public rbe::dsrl::base_frame<MessageIdHeader, dsrl::any_test> {
  using base = rbe::dsrl::base_frame<MessageIdHeader, dsrl::any_test>;

public:
  static constexpr auto make(buffer_type const buf) -> std::optional<line_view> {
    auto const hdr = header_return_type::make(buf); // step 1
    if (not hdr.has_value()) {
      return std::nullopt;
    }
    auto const rest   = buf.subspan(hdr->length());
    auto const extent = line_extent(rest); // step 2: the only custom part
    if (not extent.has_value()) {
      return std::nullopt; // incomplete line: wait for more bytes
    }
    auto const line = rest.first(*extent);
    return rbe::dsrl::make_payload<payload_type>(*hdr, line) // step 3
        .transform([&](payload_return_type const p) { return line_view {*hdr, p, hdr->length() + line.size()}; });
  }

  /// precondition: buf holds a whole line
  constexpr explicit line_view(buffer_type const buf) : line_view(*make(buf)) { }

  [[nodiscard]] constexpr auto length() const -> size_type { return line_length_; }

  [[nodiscard]] constexpr auto is_extended() const -> bool { return this->header().length() + this->payload().size() != line_length_; }


private:
  /// up to the first '\n', then every '\n' that follows it; nullopt if there is none yet
  static constexpr auto line_extent(buffer_type const rest) -> std::optional<size_type> {
    auto const nl = std::ranges::find(rest, std::byte {'\n'});
    if (nl == rest.end()) {
      return std::nullopt;
    }
    auto n = static_cast<size_type>(nl - rest.begin());
    while (n < rest.size() and rest[n] == std::byte {'\n'}) {
      ++n;
    }
    return n;
  }

  constexpr line_view(header_return_type const hdr, payload_return_type const p, size_type const n) :
    base(hdr, p), line_length_(n) { }

  size_type line_length_;
};

// the two primitives are all it took: it is a frame, and a self-delimiting one (any_id, inherited shape)
static_assert(rbe::dsrl::is_frame<line_view>);
static_assert(rbe::self_delimiting_frame<line_view>);
static_assert(rbe::dispatch_delimited_frame<line_view>);

constexpr auto custom_frame_follows_its_own_length() {
  // id 0 (msg_1, 4 bytes) '\n' | id 9 (unknown) 2 bytes '\n' '\n' | id 2 (msg_3, 1 byte) '\n'
  constexpr auto wire = std::array<std::byte, 23> {
    std::byte {0x00}, std::byte {0x00}, std::byte {0x00}, std::byte {0x00}, std::byte {0x11}, std::byte {0x11},
    std::byte {0x11}, std::byte {0x11}, std::byte {'\n'}, //
    std::byte {0x09}, std::byte {0x00}, std::byte {0x00}, std::byte {0x00}, std::byte {'A'},  std::byte {'B'},
    std::byte {'\n'}, std::byte {'\n'}, //
    std::byte {0x02}, std::byte {0x00}, std::byte {0x00}, std::byte {0x00}, std::byte {'x'},  std::byte {'\n'},
  };

  auto const first = line_view::make(wire).value();
  RBE_CHECK(first.length() == 9); // header + msg_1 + '\n', not header + msg_1
  RBE_CHECK(first.as_span().size() == 9); // the derived spans follow the custom length...
  RBE_CHECK(first.payload_span().size() == 5);
  RBE_CHECK(first.payload().as_span().size() == 4); // ...but the any, like a proxy, is only the candidate it knows
  RBE_CHECK(first.payload().is<msg_1>());
  RBE_CHECK(first.buffer().size() == wire.size());

  // an incomplete line is reported, not constructed
  RBE_CHECK(not line_view::make(std::span {wire}.first(8)).has_value());
}

constexpr auto many_iterates_a_custom_frame_by_line() {
  constexpr auto wire = std::array<std::byte, 23> {
    std::byte {0x00}, std::byte {0x00}, std::byte {0x00}, std::byte {0x00}, std::byte {0x11}, std::byte {0x11},
    std::byte {0x11}, std::byte {0x11}, std::byte {'\n'}, //
    std::byte {0x09}, std::byte {0x00}, std::byte {0x00}, std::byte {0x00}, std::byte {'A'},  std::byte {'B'},
    std::byte {'\n'}, std::byte {'\n'}, //
    std::byte {0x02}, std::byte {0x00}, std::byte {0x00}, std::byte {0x00}, std::byte {'x'},  std::byte {'\n'},
  };
  constexpr auto lengths  = std::array<std::size_t, 3> {9, 8, 6};
  constexpr auto known_id = std::array<bool, 3> {true, false, true};

  auto lines       = rbe::dsrl::many<line_view> {wire};
  std::size_t seen = 0;
  for (auto const line: lines) {
    RBE_CHECK(seen < lengths.size());
    RBE_CHECK(line.length() == lengths[seen]);
    RBE_CHECK(line.payload().known_id() == known_id[seen]);
    ++seen;
  }
  // the unknown id in the middle did not stop the iteration, and the buffer was consumed exactly
  RBE_CHECK(seen == 3);
  RBE_CHECK(lines.remainder().empty());
}

// clang-format off
TEST_SUITE("dsrl_frame - length and buffer accessors") {
  RBE_TEST_CASE("dsrl_frame - length and buffer: construction narrows the span to the frame", frame_narrows_at_construction);
  RBE_TEST_CASE("dsrl_frame - length and buffer: static sizes", static_sizes);
  RBE_TEST_CASE("dsrl_frame - length and buffer: no annotations blob", no_annotations_blob);
  RBE_TEST_CASE("dsrl_frame - length and buffer: payload_length = frame_length - header_length", frame_hdr_length);
  RBE_TEST_CASE("dsrl_frame - length and buffer: frame_length = header_length + payload_length", frame_length_hdr_plus_payload);
  RBE_TEST_CASE("dsrl_frame - length and buffer: payload starts after the annotated header length, not the static header size", frame_payload_starts_after_header_length);
  RBE_TEST_CASE("dsrl_frame - length and buffer: an extended header exposes its length, size and extension spans", header_proxy_extended_spans);
  RBE_TEST_CASE("dsrl_frame - length and buffer: an unextended header has no extension bytes", header_proxy_unextended_spans);
  RBE_TEST_CASE("dsrl_frame - length and buffer: the outer payload narrows to the inner frame length", frame_nested_frame_length);
  RBE_TEST_CASE("dsrl_frame - length and buffer: frame with empty payload", frame_with_empty_payload);
  RBE_TEST_CASE("dsrl_frame - length and buffer: payload bigger than wirable size", frame_payload_bigger_than_wireable);
  RBE_TEST_CASE("dsrl_frame - length and buffer: buffer() is the untrimmed buffer, the spans derive from length()", frame_buffer_is_untrimmed);
  RBE_TEST_CASE("dsrl_frame - building blocks: header::make + payload_extent + make_payload compose into a frame", building_blocks_compose_into_a_frame);
  RBE_TEST_CASE("dsrl_frame - custom frame: a base_frame with its own length() drives as_span, payload_span and the payload", custom_frame_follows_its_own_length);
  RBE_TEST_CASE("dsrl_frame - custom frame: many iterates by the custom length and steps over an unknown id", many_iterates_a_custom_frame_by_line);
}
// clang-format on


} // namespace
