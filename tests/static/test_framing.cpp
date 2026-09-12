/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_framing.cpp
 * @date 12/09/2026
 * @brief Static assertions for the frame type mapping (concepts, dsrl/srl lowering and traits)
 */

// --- Includes ---
#include "common_structs.hpp"

#include <rbe/framing.hpp>

// --- STD ---
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace {

// clang-format off

// ============================================================
// Shorthands
// ============================================================
using candidates  = rbe::any<MessageWithHeader, MessageWithHeaderPack>;
using msg_frame   = rbe::frame<CommonHeader, MessageWithHeader>;   // one wirable payload
using any_frame   = rbe::frame<CommonHeader, candidates>;          // candidate set payload
using blob_frame  = rbe::frame<CommonHeader, rbe::blob>;           // opaque bytes payload
using nested      = rbe::frame<NestedPackParent, msg_frame>;       // nested frame payload
using packet      = rbe::frame<NestedPackParent, rbe::many<any_frame>>; // two-level protocol


// ============================================================
// frame header
// ============================================================
static_assert(rbe::frame_header<CommonHeader>);
static_assert(rbe::frame_header<NestedPackParent>);
// a header must be wirable: empty classes and non-wirable members are rejected
static_assert(not rbe::frame_header<Empty>);
static_assert(not rbe::frame_header<AggregateWithPtr>);


// ============================================================
// frame payload
// ============================================================
static_assert(rbe::frame_payload<MessageWithHeader>); // wirable
static_assert(rbe::frame_payload<rbe::any<MessageWithHeader, MessageWithHeaderPack>>); // any
static_assert(rbe::frame_payload<rbe::many<rbe::frame<CommonHeader, MessageWithHeader>>>); // many
static_assert(rbe::frame_payload<rbe::blob>); // blob
static_assert(rbe::frame_payload<rbe::frame<CommonHeader, rbe::many<rbe::frame<CommonHeader, MessageWithHeader>>>>);
static_assert(rbe::frame_payload<rbe::frame<CommonHeader, rbe::many<rbe::frame<CommonHeader, rbe::any<MessageWithHeader, MessageWithHeaderPack>>>>>);
// neither wirable nor frame_serder
static_assert(not rbe::frame_payload<Empty>);
static_assert(not rbe::frame_payload<AggregateWithPtr>);


// ============================================================
// frame_serder
// ============================================================

// every framing vocabulary type lowers to a deserialization type
static_assert(rbe::frame_serder<candidates>);
static_assert(rbe::frame_serder<rbe::many<msg_frame>>);
static_assert(rbe::frame_serder<rbe::blob>);
static_assert(rbe::frame_serder<msg_frame>);

// a plain wirable message is not a frame_serder, it is lowered as-is
static_assert(not rbe::frame_serder<MessageWithHeader>);
static_assert(not rbe::frame_serder<std::uint32_t>);


// ============================================================
// is_frame
// ============================================================

static_assert(rbe::is_frame<rbe::frame<CommonHeader, MessageWithHeader>>);
static_assert(rbe::is_frame<any_frame>);
static_assert(rbe::is_frame<blob_frame>);
static_assert(rbe::is_frame<nested>);
static_assert(rbe::is_frame<packet>);

// the framing vocabulary types are payloads, not frames
static_assert(not rbe::is_frame<candidates>);
static_assert(not rbe::is_frame<rbe::many<msg_frame>>);
static_assert(not rbe::is_frame<rbe::blob>);
static_assert(not rbe::is_frame<MessageWithHeader>);
static_assert(not rbe::is_frame<std::uint32_t>);

// a lowered frame is a dsrl::frame, and only satisfies the dsrl concept
static_assert(rbe::dsrl::is_frame<msg_frame::dsrl_type>);
static_assert(not rbe::is_frame<msg_frame::dsrl_type>);


// ============================================================
// frame member types
// ============================================================
static_assert(std::same_as<msg_frame::header_type, CommonHeader>);
static_assert(std::same_as<msg_frame::payload_type, MessageWithHeader>);
static_assert(std::same_as<packet::header_type, NestedPackParent>);
static_assert(std::same_as<packet::payload_type, rbe::many<any_frame>>);

// the payload of a nested frame is the frame itself, not its lowering
static_assert(std::same_as<nested::payload_type, msg_frame>);


// ============================================================
// to_dsrl_type
// ============================================================

// a wirable payload is its own dsrl type -- deserialization is driven by the strategy, not the type
static_assert(std::same_as<rbe::detail::to_dsrl_t<MessageWithHeader>, MessageWithHeader>);
static_assert(std::same_as<rbe::detail::to_dsrl_t<std::uint32_t>, std::uint32_t>);

// vocabulary types lower to their rbe::dsrl counterpart
static_assert(std::same_as<rbe::detail::to_dsrl_t<candidates>, rbe::dsrl::any<MessageWithHeader, MessageWithHeaderPack>>); 
static_assert(std::same_as<rbe::detail::to_dsrl_t<rbe::many<msg_frame>>, rbe::dsrl::many<rbe::dsrl::frame<CommonHeader, MessageWithHeader>>>);
static_assert(std::same_as<rbe::detail::to_dsrl_t<rbe::blob>, std::span<std::byte const>>);
static_assert(std::same_as<rbe::detail::to_dsrl_t<msg_frame>, rbe::dsrl::frame<CommonHeader, MessageWithHeader>>);
// lowering is idempotent: an already lowered type has no dsrl_type of its own
static_assert(std::same_as<rbe::detail::to_dsrl_t<rbe::detail::to_dsrl_t<msg_frame>>, rbe::detail::to_dsrl_t<msg_frame>>);
static_assert(std::same_as<rbe::detail::to_dsrl_t<rbe::dsrl::any<MessageWithHeader>>, rbe::dsrl::any<MessageWithHeader>>);

// the header is never lowered, only the payload is
static_assert(std::same_as<msg_frame::dsrl_type, rbe::dsrl::frame<CommonHeader, MessageWithHeader>>);
static_assert(std::same_as<any_frame::dsrl_type, rbe::dsrl::frame<CommonHeader, rbe::dsrl::any<MessageWithHeader, MessageWithHeaderPack>>>); 
static_assert(std::same_as<blob_frame::dsrl_type, rbe::dsrl::frame<CommonHeader, std::span<std::byte const>>>);

 // lowering recurses through nesting: frame -> dsrl::frame, many<frame> -> dsrl::many<dsrl::frame>
static_assert(std::same_as<nested::dsrl_type, rbe::dsrl::frame<NestedPackParent, rbe::dsrl::frame<CommonHeader, MessageWithHeader>>>); 
static_assert(std::same_as<packet::dsrl_type, rbe::dsrl::frame<NestedPackParent, rbe::dsrl::many<rbe::dsrl::frame<CommonHeader, rbe::dsrl::any<MessageWithHeader, MessageWithHeaderPack>>>>>); // the lowered frame keeps the header, and carries the lowered payload
static_assert(std::same_as<packet::dsrl_type::header_type, NestedPackParent>);
static_assert(std::same_as<packet::dsrl_type::payload_type, rbe::detail::to_dsrl_t<packet::payload_type>>);
static_assert(std::same_as<packet::dsrl_type::buffer_type, std::span<std::byte const>>);


// ============================================================
// frame_traits
// ============================================================
static_assert(std::same_as<rbe::frame_header_t<msg_frame>, msg_frame::header_type>);
static_assert(std::same_as<rbe::frame_payload_t<msg_frame>, msg_frame::payload_type>);
static_assert(std::same_as<rbe::frame_dsrl_t<msg_frame>, msg_frame::dsrl_type>);
static_assert(std::same_as<rbe::frame_header_t<packet>, NestedPackParent>);
static_assert(std::same_as<rbe::frame_payload_t<packet>, rbe::many<any_frame>>);
static_assert(std::same_as<rbe::frame_dsrl_t<packet>, packet::dsrl_type>);
static_assert(std::same_as<rbe::frame_payload_t<rbe::frame_payload_t<nested>>, MessageWithHeader>);
static_assert(std::same_as<rbe::frame_header_t<rbe::frame_payload_t<nested>>, CommonHeader>);

// clang-format on

} // namespace
