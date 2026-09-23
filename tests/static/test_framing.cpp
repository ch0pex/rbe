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
#include "common_frame.hpp"

#include <rbe/framing/frame_delimiting_concepts.hpp>
#include <rbe/framing/frame_serder_concepts.hpp>

// --- STD ---
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace {

// ============================================================
// frame header
// ============================================================
// there is a single header concept: lowering never changes the header type
static_assert(rbe::frame_header<CommonHeader>);
static_assert(rbe::frame_header<NestedPackParent>);
// a header must be wirable: empty classes and non-wirable members are rejected
static_assert(not rbe::frame_header<Empty>);
static_assert(not rbe::frame_header<AggregateWithPtr>);


// ============================================================
// is_frame -- the shape shared by every level
// ============================================================
static_assert(rbe::is_frame<msg_frame>); // vocabulary frame
static_assert(rbe::is_frame<msg_frame::dsrl_type>); // its deserializer
static_assert(rbe::is_frame<nested>);
static_assert(rbe::is_frame<packet>);

// the framing vocabulary types are payloads, not frames
static_assert(not rbe::is_frame<candidates>);
static_assert(not rbe::is_frame<rbe::many<msg_frame>>);
static_assert(not rbe::is_frame<rbe::blob>);
static_assert(not rbe::is_frame<MessageWithHeader>);
static_assert(not rbe::is_frame<std::uint32_t>);


// ============================================================
// frame serder payload -- the vocabulary level
// ============================================================
static_assert(rbe::frame_serder_payload<MessageWithHeader>); // wirable
static_assert(rbe::frame_serder_payload<rbe::any<MessageWithHeader, MessageWithHeaderPack>>); // any
static_assert(rbe::frame_serder_payload<rbe::many<rbe::frame<CommonHeader, MessageWithHeader>>>); // many
static_assert(rbe::frame_serder_payload<rbe::blob>); // blob
static_assert(
    rbe::frame_serder_payload<rbe::frame<CommonHeader, rbe::many<rbe::frame<CommonHeader, MessageWithHeader>>>>
);
static_assert(
    rbe::frame_serder_payload<rbe::frame<
        CommonHeader, rbe::many<rbe::frame<CommonHeader, rbe::any<MessageWithHeader, MessageWithHeaderPack>>>>>
);
// neither wirable nor serder_traits
static_assert(not rbe::frame_serder_payload<Empty>);
static_assert(not rbe::frame_serder_payload<AggregateWithPtr>);


// ============================================================
// serder_traits
// ============================================================

// every framing vocabulary type lowers to a deserialization type
static_assert(rbe::serder_traits<candidates>);
static_assert(rbe::serder_traits<rbe::many<msg_frame>>);
static_assert(rbe::serder_traits<rbe::blob>);
static_assert(rbe::serder_traits<msg_frame>);

// a plain wirable message is not a serder_traits, it is lowered as-is
static_assert(not rbe::serder_traits<MessageWithHeader>);
static_assert(not rbe::serder_traits<std::uint32_t>);


// ============================================================
// frame_serder -- the vocabulary level
// ============================================================

static_assert(rbe::frame_serder<rbe::frame<CommonHeader, MessageWithHeader>>);
static_assert(rbe::frame_serder<any_frame>);
static_assert(rbe::frame_serder<blob_frame>);
static_assert(rbe::frame_serder<nested>);
static_assert(rbe::frame_serder<packet>);

// the framing vocabulary types are payloads, not frames
static_assert(not rbe::frame_serder<candidates>);
static_assert(not rbe::frame_serder<rbe::many<msg_frame>>);
static_assert(not rbe::frame_serder<rbe::blob>);
static_assert(not rbe::frame_serder<MessageWithHeader>);
static_assert(not rbe::frame_serder<std::uint32_t>);

// a lowered frame is a dsrl::frame: it has the shape, but not the vocabulary traits
static_assert(rbe::dsrl::is_frame<msg_frame::dsrl_type>);
static_assert(rbe::dsrl::is_frame<packet::dsrl_type>);
static_assert(rbe::is_frame<msg_frame::dsrl_type>);
static_assert(not rbe::frame_serder<msg_frame::dsrl_type>);

// the payload concept is shared by dsrl and srl, so it takes both spans
static_assert(rbe::frame_payload<MessageWithHeader>);
static_assert(rbe::frame_payload<std::span<std::byte const>>); // rbe::blob lowered for dsrl
static_assert(rbe::frame_payload<std::span<std::byte>>); // ... and for srl
static_assert(rbe::frame_payload<msg_frame::dsrl_type>); // a nested lowered frame


// ============================================================
// payload extent
// ============================================================

using rbe::detail::payload_extent;
using rbe::detail::payload_extent_of;

static_assert(payload_extent_of<WithPayloadLength, rbe::blob>() == payload_extent::payload_length_field);
static_assert(payload_extent_of<WithFrameLength, rbe::blob>() == payload_extent::frame_length_field);
static_assert(payload_extent_of<CommonHeader, msg_frame>() == payload_extent::nested_frame);
static_assert(payload_extent_of<CommonHeader, MessageWithHeader>() == payload_extent::static_size);
static_assert(payload_extent_of<CommonHeader, rbe::blob>() == payload_extent::buffer_end);
static_assert(payload_extent_of<CommonHeader, rbe::many<msg_frame>>() == payload_extent::buffer_end);
static_assert(payload_extent_of<CommonHeader, candidates>() == payload_extent::any_id);
// a length field takes precedence over the static size of the payload
static_assert(payload_extent_of<WithPayloadLength, MessageWithHeader>() == payload_extent::payload_length_field);

// lowering never changes the extent, so dsrl::frame::payload_length() agrees with the rbe:: classification
template<typename Header, typename Payload>
inline constexpr bool same_extent_when_lowered =
    payload_extent_of<Header, Payload>() == payload_extent_of<Header, rbe::detail::to_dsrl_t<Payload>>();

static_assert(same_extent_when_lowered<WithFrameLength, rbe::blob>);
static_assert(same_extent_when_lowered<CommonHeader, msg_frame>);
static_assert(same_extent_when_lowered<CommonHeader, MessageWithHeader>);
static_assert(same_extent_when_lowered<CommonHeader, rbe::blob>);
static_assert(same_extent_when_lowered<CommonHeader, candidates>);
static_assert(same_extent_when_lowered<NestedPackParent, rbe::many<any_frame>>);


// ============================================================
// self-delimiting vs buffer-delimited frames
// ============================================================
static_assert(rbe::self_delimiting_frame<msg_frame>);
static_assert(rbe::self_delimiting_frame<rbe::frame<WithFrameLength, rbe::blob>>);
static_assert(rbe::self_delimiting_frame<rbe::frame<WithPayloadLength, rbe::blob>>);
static_assert(rbe::self_delimiting_frame<rbe::frame<WithHeaderLength, MessageWithHeader>>);
static_assert(rbe::buffer_delimited_frame<blob_frame>);
static_assert(rbe::buffer_delimited_frame<packet>); // many runs to the end of the buffer
// header_length only delimits the header, the payload still runs to the end of the buffer
static_assert(rbe::buffer_delimited_frame<rbe::frame<WithHeaderLength, rbe::blob>>);

// a nested frame delimits the outer payload only if it delimits itself...
static_assert(rbe::self_delimiting_frame<nested>);
static_assert(rbe::buffer_delimited_frame<rbe::frame<CommonHeader, blob_frame>>);
// ...unless the outer header already carries a length
static_assert(rbe::self_delimiting_frame<rbe::frame<WithFrameLength, blob_frame>>);

// both concepts are level-agnostic: a lowered frame is delimited exactly like the frame it comes from
static_assert(rbe::self_delimiting_frame<msg_frame::dsrl_type>);
static_assert(rbe::self_delimiting_frame<nested::dsrl_type>);
static_assert(rbe::buffer_delimited_frame<blob_frame::dsrl_type>);
static_assert(rbe::buffer_delimited_frame<packet::dsrl_type>);
static_assert(not rbe::buffer_delimited_frame<msg_frame::dsrl_type>);
static_assert(not rbe::buffer_delimited_frame<nested::dsrl_type>);
static_assert(not rbe::self_delimiting_frame<blob_frame::dsrl_type>);
static_assert(not rbe::self_delimiting_frame<packet::dsrl_type>);

// ... but neither accepts something that is not a frame
static_assert(not rbe::self_delimiting_frame<MessageWithHeader>);
static_assert(not rbe::buffer_delimited_frame<MessageWithHeader>);
static_assert(not rbe::self_delimiting_frame<rbe::blob>);
static_assert(not rbe::buffer_delimited_frame<rbe::blob>);

// ============================================================
// explicitly_delimited_frame vs implicitly_delimited_frame
// ============================================================
static_assert(rbe::self_delimiting_frame<msg_frame>);
static_assert(rbe::self_delimiting_frame<rbe::frame<WithHeaderLength, MessageWithHeader>>);
static_assert(rbe::self_delimiting_frame<rbe::frame<WithFrameLength, rbe::blob>>);
static_assert(rbe::self_delimiting_frame<rbe::frame<WithPayloadLength, rbe::blob>>);

static_assert(not rbe::explicitly_delimited_frame<msg_frame>);
static_assert(not rbe::explicitly_delimited_frame<rbe::frame<WithHeaderLength, MessageWithHeader>>);
static_assert(rbe::explicitly_delimited_frame<rbe::frame<WithFrameLength, rbe::blob>>);
static_assert(rbe::explicitly_delimited_frame<rbe::frame<WithPayloadLength, rbe::blob>>);

static_assert(rbe::implicitly_delimited_frame<msg_frame>);
static_assert(rbe::implicitly_delimited_frame<rbe::frame<WithHeaderLength, MessageWithHeader>>);
static_assert(not rbe::implicitly_delimited_frame<rbe::frame<WithFrameLength, rbe::blob>>);
static_assert(not rbe::implicitly_delimited_frame<rbe::frame<WithPayloadLength, rbe::blob>>);

static_assert(not rbe::implicitly_delimited_frame<rbe::frame<CommonHeader, blob_frame>>);
static_assert(not rbe::explicitly_delimited_frame<rbe::frame<CommonHeader, blob_frame>>);

// ============================================================
// dispatch_delimited_frame -- the any_id case
// ============================================================
static_assert(rbe::dispatch_delimited_frame<rbe::frame<CommonHeader, candidates>>);
static_assert(not rbe::dispatch_delimited_frame<rbe::frame<CommonHeader, MessageWithHeader>>);
// header_length only delimits the header, so the payload still resolves through the id
static_assert(rbe::dispatch_delimited_frame<rbe::frame<WithHeaderLength, candidates>>);
// a length field is read straight off the wire: no dispatch, whatever the payload is
static_assert(not rbe::dispatch_delimited_frame<rbe::frame<WithFrameLength, rbe::blob>>);
static_assert(not rbe::dispatch_delimited_frame<rbe::frame<WithPayloadLength, rbe::blob>>);
static_assert(not rbe::dispatch_delimited_frame<rbe::frame<WithFrameLength, candidates>>);
static_assert(not rbe::dispatch_delimited_frame<rbe::frame<WithPayloadLength, candidates>>);
// ... and the cost of a nested frame is the cost of the frame it nests
static_assert(rbe::dispatch_delimited_frame<rbe::frame<CommonHeader, rbe::frame<CommonHeader, candidates>>>);
static_assert(not rbe::dispatch_delimited_frame<rbe::frame<WithFrameLength, rbe::frame<CommonHeader, candidates>>>);

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
static_assert(
    std::same_as<rbe::detail::to_dsrl_t<candidates>, rbe::dsrl::any<MessageWithHeader, MessageWithHeaderPack>>
);
static_assert(std::same_as<
              rbe::detail::to_dsrl_t<rbe::many<msg_frame>>,
              rbe::dsrl::many<rbe::dsrl::frame<CommonHeader, MessageWithHeader>>>);
static_assert(std::same_as<rbe::detail::to_dsrl_t<rbe::blob>, std::span<std::byte const>>);
static_assert(std::same_as<rbe::detail::to_dsrl_t<msg_frame>, rbe::dsrl::frame<CommonHeader, MessageWithHeader>>);
// lowering is idempotent: an already lowered type has no dsrl_type of its own
static_assert(
    std::same_as<rbe::detail::to_dsrl_t<rbe::detail::to_dsrl_t<msg_frame>>, rbe::detail::to_dsrl_t<msg_frame>>
);
static_assert(std::same_as<
              rbe::detail::to_dsrl_t<rbe::dsrl::any<MessageWithHeader, MessageWithHeaderPack>>,
              rbe::dsrl::any<MessageWithHeader, MessageWithHeaderPack>>);

// the header is never lowered, only the payload is
static_assert(std::same_as<msg_frame::dsrl_type, rbe::dsrl::frame<CommonHeader, MessageWithHeader>>);
static_assert(
    std::same_as<
        any_frame::dsrl_type, rbe::dsrl::frame<CommonHeader, rbe::dsrl::any<MessageWithHeader, MessageWithHeaderPack>>>
);
static_assert(std::same_as<blob_frame::dsrl_type, rbe::dsrl::frame<CommonHeader, std::span<std::byte const>>>);

// lowering recurses through nesting: frame -> dsrl::frame, many<frame> -> dsrl::many<dsrl::frame>
static_assert(
    std::same_as<
        nested::dsrl_type, rbe::dsrl::frame<NestedPackParent, rbe::dsrl::frame<CommonHeader, MessageWithHeader>>>
);
static_assert(
    std::same_as<
        packet::dsrl_type,
        rbe::dsrl::frame<
            NestedPackParent,
            rbe::dsrl::many<rbe::dsrl::frame<CommonHeader, rbe::dsrl::any<MessageWithHeader, MessageWithHeaderPack>>>>>
); // the lowered frame keeps the header, and carries the lowered payload
static_assert(std::same_as<packet::dsrl_type::header_type, NestedPackParent>);
static_assert(std::same_as<packet::dsrl_type::payload_type, rbe::detail::to_dsrl_t<packet::payload_type>>);
static_assert(std::same_as<packet::dsrl_type::buffer_type, std::span<std::byte const>>);


// ============================================================
// frame_serder_traits
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
