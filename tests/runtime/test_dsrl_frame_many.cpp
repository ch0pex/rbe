/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_dsrl_frame_many.cpp
 * @date 06/10/2026
 * @brief Tests for dsrl::many: splitting a buffer into consecutive self-delimiting frames
 */

// --- Includes ---
#include "common_frame.hpp"
#include "common_serde.hpp"
#include "test_macros.hpp"

#include <rbe/framing/dsrl/frame.hpp>
#include <rbe/framing/dsrl/many.hpp>
#include <rbe/framing/dsrl/views.hpp>

// --- STD ---
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <span>

namespace {

using blob = std::span<std::byte const>;

/// Lengths come from a frame_length field: 3 byte header (type, length), payload up to the declared length
using length_frame = rbe::dsrl::frame<FrameLengthHeader, blob>;
/// No length on the wire at all: header and payload sizes are known to the code, 4 + 4 bytes
using static_frame = rbe::dsrl::frame<PlainHeader, std::uint32_t>;
/// The length comes from the candidate the id selects: 4 byte id, then msg_1 (4 bytes) or msg_3 (1 byte)
using id_frame = rbe::dsrl::frame<MessageIdHeader, dsrl::any_test>;

static_assert(rbe::self_delimiting_frame<length_frame>);
static_assert(rbe::self_delimiting_frame<static_frame>);
static_assert(rbe::self_delimiting_frame<id_frame>);

using length_many = rbe::dsrl::many<length_frame>;
static_assert(std::input_iterator<length_many::iterator>);
static_assert(std::sentinel_for<std::default_sentinel_t, length_many::iterator>);
static_assert(std::ranges::input_range<length_many>);

// type, frame_length (LE), payload      -> frame length
//   1     5                 AA AA          5
//   2     3                 -              3 (empty payload)
//   3     7                 B1 B2 B3 B4    7
constexpr auto three_frames = bytes(
    0x01, 0x05, 0x00, 0xAA, 0xAA, //
    0x02, 0x03, 0x00, //
    0x03, 0x07, 0x00, 0xB1, 0xB2, 0xB3, 0xB4
);

// ============================================================
// iteration: one frame after the other, each as long as the wire says
// ============================================================

constexpr auto iterates_every_frame() {
  constexpr auto lengths       = std::array<std::size_t, 3> {5, 3, 7};
  constexpr auto payload_sizes = std::array<std::size_t, 3> {2, 0, 4};

  auto frames      = rbe::dsrl::many<length_frame> {three_frames};
  std::size_t seen = 0;
  for (auto const frame: frames) {
    RBE_REQUIRE(seen < lengths.size());
    RBE_CHECK(frame.length() == lengths[seen]);
    RBE_CHECK(frame.payload().size() == payload_sizes[seen]);
    RBE_CHECK(frame.header().field<"type">() == seen + 1);
    ++seen;
  }
  RBE_CHECK(seen == 3);
  RBE_CHECK(frames.done());
  RBE_CHECK(frames.remainder().empty());
  RBE_CHECK_FALSE(frames.has_seen_partial());
}

constexpr auto frames_view_the_original_buffer() {
  auto frames = rbe::dsrl::many<length_frame> {three_frames};

  RBE_CHECK(frames.current().data() == three_frames.data());
  frames.next();
  RBE_CHECK(frames.current().data() == three_frames.data() + 5);
  frames.next();
  RBE_CHECK(frames.current().data() == three_frames.data() + 8);
  RBE_CHECK(frames.current().payload().data() == three_frames.data() + 11);
}

constexpr auto static_frames_split_by_their_size() {
  // three 8-byte frames and 3 bytes of a fourth
  auto wire = std::array<std::byte, 27> {};
  for (std::size_t i = 0; i < 24; ++i) {
    wire[i] = std::byte {0x11};
  }

  auto frames      = rbe::dsrl::many<static_frame> {wire};
  std::size_t seen = 0;
  for (auto const frame: frames) {
    RBE_CHECK(frame.length() == 8);
    ++seen;
  }
  RBE_CHECK(seen == 3);
  RBE_CHECK(frames.has_seen_partial());
  RBE_CHECK(frames.remainder().size() == 3);
}

// ============================================================
// manual stepping: current / next / done / remainder
// ============================================================

constexpr auto manual_stepping() {
  auto frames = rbe::dsrl::many<length_frame> {three_frames};

  RBE_REQUIRE(not frames.done());
  RBE_CHECK(frames.current().length() == 5);

  frames.next();
  RBE_REQUIRE(not frames.done());
  RBE_CHECK(frames.current().length() == 3);
  // what is left after consuming the first frame, current frame included
  RBE_CHECK(frames.remainder().size() == three_frames.size() - 5);
  RBE_CHECK(frames.as_span().size() == three_frames.size() - 5);
  RBE_CHECK(frames.data() == three_frames.data() + 5);

  frames.next();
  RBE_CHECK(frames.current().length() == 7);

  frames.next();
  RBE_CHECK(frames.done());
  RBE_CHECK(frames.remainder().empty());
}

constexpr auto iterator_operations() {
  auto frames = rbe::dsrl::many<length_frame> {three_frames};
  auto it     = frames.begin();

  RBE_CHECK(it != frames.end());
  RBE_CHECK((*it).length() == 5);
  RBE_CHECK(it->length() == 5);

  it++; // post-increment advances too
  RBE_CHECK(it->length() == 3);
  ++it;
  RBE_CHECK((*it).length() == 7);
  ++it;
  RBE_CHECK(it == frames.end());
}

// ============================================================
// nothing, or not enough, to build a frame from
// ============================================================

constexpr auto empty_buffer() {
  auto frames = rbe::dsrl::many<length_frame> {blob {}};

  RBE_CHECK(frames.done());
  RBE_CHECK(frames.begin() == frames.end());
  RBE_CHECK(frames.remainder().empty());
  RBE_CHECK_FALSE(frames.has_seen_partial());
}

constexpr auto exactly_one_frame() {
  auto const wire = bytes(0x01, 0x05, 0x00, 0xAA, 0xAA);
  auto frames     = rbe::dsrl::many<length_frame> {wire};

  RBE_REQUIRE(not frames.done());
  RBE_CHECK(frames.current().length() == wire.size());
  frames.next();
  RBE_CHECK(frames.done());
  RBE_CHECK_FALSE(frames.has_seen_partial());
}

constexpr auto partial_header_after_whole_frames() {
  // the frame_length field is not even complete: 2 of the 3 header bytes
  auto const wire = bytes(0x01, 0x05, 0x00, 0xAA, 0xAA, 0x04, 0x09);

  auto frames      = rbe::dsrl::many<length_frame> {wire};
  std::size_t seen = 0;
  for ([[maybe_unused]] auto const frame: frames) {
    ++seen;
  }
  RBE_CHECK(seen == 1);
  RBE_CHECK(frames.done());
  RBE_CHECK(frames.has_seen_partial());
  RBE_CHECK(frames.remainder().size() == 2);
  RBE_CHECK(frames.remainder().data() == wire.data() + 5);
}

constexpr auto partial_payload_after_whole_frames() {
  // the header declares 9 bytes, only 4 arrived
  auto const wire = bytes(0x01, 0x05, 0x00, 0xAA, 0xAA, 0x04, 0x09, 0x00, 0xCC);

  auto frames = rbe::dsrl::many<length_frame> {wire};
  frames.next();
  RBE_CHECK(frames.done());
  RBE_CHECK(frames.has_seen_partial());
  RBE_CHECK(frames.remainder().size() == 4);
}

constexpr auto partial_first_frame() {
  auto const wire = bytes(0x01, 0x05, 0x00, 0xAA);
  auto frames     = rbe::dsrl::many<length_frame> {wire};

  RBE_CHECK(frames.done());
  RBE_CHECK(frames.has_seen_partial());
  RBE_CHECK(frames.remainder().size() == wire.size());
  RBE_CHECK(frames.remainder().data() == wire.data());
}

constexpr auto make_always_yields_a_many() {
  auto const frames = rbe::dsrl::many<length_frame>::make(blob {});
  // an empty or incomplete buffer is not an error, it is a many that is already done
  RBE_REQUIRE(frames.has_value());
  RBE_CHECK(frames->done());
}

// ============================================================
// frames whose length is implied by the id
// ============================================================

constexpr auto id_frames_split_by_their_candidate() {
  // id 0 + msg_1 (4 bytes) | id 2 + msg_3 (1 byte) | id 0 + 2 of the 4 bytes of msg_1
  auto const wire = bytes(
      0x00, 0x00, 0x00, 0x00, 0x11, 0x11, 0x11, 0x11, //
      0x02, 0x00, 0x00, 0x00, 'x', //
      0x00, 0x00, 0x00, 0x00, 0x22, 0x22
  );

  auto frames      = rbe::dsrl::many<id_frame> {wire};
  std::size_t seen = 0;
  for (auto const frame: frames) {
    if (seen == 0) {
      RBE_CHECK(frame.payload().is<msg_1>());
      RBE_CHECK(frame.length() == 8);
    }
    else {
      RBE_CHECK(frame.payload().is<msg_3>());
      RBE_CHECK(frame.length() == 5);
    }
    ++seen;
  }
  RBE_CHECK(seen == 2);
  RBE_CHECK(frames.has_seen_partial()); // msg_1 does not fit in the 2 bytes left
  RBE_CHECK(frames.remainder().size() == 6);
}

constexpr auto unknown_id_takes_the_rest_of_the_buffer() {
  // the code does not know id 9, so nothing says where its frame ends: it runs to the end of the buffer
  auto const wire = bytes(
      0x00, 0x00, 0x00, 0x00, 0x11, 0x11, 0x11, 0x11, //
      0x09, 0x00, 0x00, 0x00, 'A', 'B', 'C', 'D', 'E', 'F'
  );

  auto frames      = rbe::dsrl::many<id_frame> {wire};
  std::size_t seen = 0;
  for (auto const frame: frames) {
    if (seen == 1) {
      RBE_CHECK_FALSE(frame.payload().known_id());
      RBE_CHECK(frame.length() == 10);
    }
    ++seen;
  }
  RBE_CHECK(seen == 2);
  RBE_CHECK(frames.remainder().empty());
}

// clang-format off
// ============================================================
// views::many: the same many, reached through a range adaptor closure
// ============================================================

constexpr auto pipe_builds_the_many() {
  auto frames = three_frames | rbe::views::many<length_frame>();
  static_assert(std::same_as<decltype(frames), length_many>);

  auto count = std::size_t {0};
  for (auto const frame: frames) {
    static_cast<void>(frame);
    ++count;
  }
  return count == 3 and frames.done();
}
static_assert(pipe_builds_the_many());

constexpr auto call_and_pipe_agree() {
  auto const piped  = three_frames | rbe::views::many<length_frame>();
  auto const called = rbe::views::many<length_frame>()(three_frames);
  return piped.as_span().data() == called.as_span().data() and piped.as_span().size() == called.as_span().size();
}

// ============================================================
// views::with_ids, known_ids, unknown_ids: filters over the frames of a many
// ============================================================

// ids 0 (msg_1), 2 (msg_3), 0 again, 9 (unknown, takes the rest)
constexpr auto mixed_ids = bytes(0x00, 0x00, 0x00, 0x00, 0x11, 0x11, 0x11, 0x11, //
                                 0x02, 0x00, 0x00, 0x00, 'x', //
                                 0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x22, //
                                 0x09, 0x00, 0x00, 0x00, 'A', 'B');

constexpr auto frames_decompose_into_header_and_payload() {
  constexpr auto payload_sizes = std::array<std::size_t, 3> {2, 0, 4};

  auto frames = rbe::dsrl::many<length_frame> {three_frames};
  auto index  = std::size_t {0};
  for (auto const [header, payload]: frames) {
    RBE_CHECK(header.length() == 3);
    RBE_CHECK(payload.size() == payload_sizes[index]);
    ++index;
  }
  RBE_CHECK(index == 3);
}

constexpr auto with_ids_keeps_matching_frames() {
  return std::ranges::distance(mixed_ids | rbe::views::many<id_frame>() | rbe::views::with_ids(0u)) == 2
     and std::ranges::distance(mixed_ids | rbe::views::many<id_frame>() | rbe::views::with_ids(2u)) == 1
     and std::ranges::distance(mixed_ids | rbe::views::many<id_frame>() | rbe::views::with_ids(0u, 9u)) == 3 // unknown ids match too
     and std::ranges::distance(mixed_ids | rbe::views::many<id_frame>() | rbe::views::with_ids(7u)) == 0;
}

constexpr auto known_and_unknown_ids_split_the_frames() {
  RBE_CHECK(std::ranges::distance(mixed_ids | rbe::views::many<id_frame>() | rbe::views::known_ids()) == 3);
  RBE_CHECK(std::ranges::distance(mixed_ids | rbe::views::many<id_frame>() | rbe::views::unknown_ids()) == 1);
}

constexpr auto count_specified_many() {
  auto frames = rbe::dsrl::many<length_frame> {three_frames, 2};
  RBE_CHECK(std::ranges::distance(frames) == 2);
  RBE_CHECK(frames.has_seen_partial());
  RBE_CHECK(frames.remainder().size() == 7);

  auto third_frame = length_frame::make(frames.remainder());
  RBE_CHECK(third_frame.has_value());
  RBE_CHECK(third_frame->length() == 7);
}


TEST_SUITE("dsrl_many") {
  RBE_TEST_CASE("dsrl_many - a frame decomposes into header and payload with a structured binding", frames_decompose_into_header_and_payload);
  RBE_TEST_CASE("dsrl_many - views::with_ids keeps the frames with any of the ids", with_ids_keeps_matching_frames);
  RBE_TEST_CASE("dsrl_many - views::known_ids and unknown_ids split by whether the code knows the id", known_and_unknown_ids_split_the_frames);
  RBE_TEST_CASE("dsrl_many - views::many pipes a buffer into a many", pipe_builds_the_many);
  RBE_TEST_CASE("dsrl_many - views::many piped and called build the same many", call_and_pipe_agree);
  RBE_TEST_CASE("dsrl_many - iterates every frame, each as long as the wire says", iterates_every_frame);
  RBE_TEST_CASE("dsrl_many - frames view the original buffer, nothing is copied", frames_view_the_original_buffer);
  RBE_TEST_CASE("dsrl_many - frames without a length on the wire split by their static size", static_frames_split_by_their_size);
  RBE_TEST_CASE("dsrl_many - manual stepping with current, next, done and remainder", manual_stepping);
  RBE_TEST_CASE("dsrl_many - iterator dereference, arrow, pre and post increment, sentinel", iterator_operations);
  RBE_TEST_CASE("dsrl_many - an empty buffer is already done and not partial", empty_buffer);
  RBE_TEST_CASE("dsrl_many - a buffer holding exactly one frame", exactly_one_frame);
  RBE_TEST_CASE("dsrl_many - an incomplete header after whole frames is left as the remainder", partial_header_after_whole_frames);
  RBE_TEST_CASE("dsrl_many - a truncated payload after whole frames is left as the remainder", partial_payload_after_whole_frames);
  RBE_TEST_CASE("dsrl_many - an incomplete first frame is done and partial", partial_first_frame);
  RBE_TEST_CASE("dsrl_many - make never fails, an incomplete buffer gives a done many", make_always_yields_a_many);
  RBE_TEST_CASE("dsrl_many - frames whose length is implied by the id", id_frames_split_by_their_candidate);
  RBE_TEST_CASE("dsrl_many - an unknown id takes the rest of the buffer", unknown_id_takes_the_rest_of_the_buffer);
  RBE_TEST_CASE("dsrl_many - a count limits the number of frames returned", count_specified_many);
}
// clang-format on

} // namespace
