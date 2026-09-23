/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file common_frame.hpp
 * @date 23/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include "common_structs.hpp"

#include <rbe/framing.hpp>


// ============================================================
// Test headers
// ============================================================

// clang-format off
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

struct [[=rbe::little, = rbe::pack]] MessageIdHeader {
  [[=rbe::id]] std::int32_t id;
};

// clang-format on

// ============================================================
// Shorthands
// ============================================================
using candidates = rbe::any<MessageWithHeader, MessageWithHeaderPack>;
using msg_frame  = rbe::frame<CommonHeader, MessageWithHeader>; // one wirable payload
using any_frame  = rbe::frame<CommonHeader, candidates>; // candidate set payload
using blob_frame = rbe::frame<CommonHeader, rbe::blob>; // opaque bytes payload
using nested     = rbe::frame<NestedPackParent, msg_frame>; // nested frame payload
using packet     = rbe::frame<NestedPackParent, rbe::many<any_frame>>; // two-level protocol

struct[[= rbe::id(0)]] msg_1 {
  int value1;
  constexpr auto operator==(msg_1 const&) const -> bool = default;
};

struct[[= rbe::id(1)]] msg_2 {
  std::array<int, 25> numbers;
  constexpr auto operator==(msg_2 const&) const -> bool = default;
};

struct[[= rbe::id(2)]] msg_3 {
  char type;
  constexpr auto operator==(msg_3 const&) const -> bool = default;
};

struct[[ = rbe::id(42), = rbe::empty ]] heartbeat {
  constexpr auto operator==(heartbeat const&) const -> bool = default;
};

template<typename T>
concept msgs_1_and_2 = std::same_as<T, msg_1> or std::same_as<T, msg_2>;

namespace dsrl {

using any_test                = rbe::dsrl::any<msg_1, msg_2, msg_3>;
using any_test_with_heartbeat = rbe::dsrl::any<msg_1, msg_2, msg_3, heartbeat>;

} // namespace dsrl
