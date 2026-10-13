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

#include <rbe/core/memory_layout.hpp>
#include <rbe/frames/frame_delimiting_concepts.hpp>
#include <rbe/frames/frame_serder_concepts.hpp>

// --- STD ---
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace {

// clang-format off

// ============================================================
// frame header
// ============================================================
// there is a single header concept: lowering never changes the header type
static_assert(rbe::frames::header<CommonHeader>);
static_assert(rbe::frames::header<NestedPackParent>);
// a header must be wirable: empty classes and non-wirable members are rejected
static_assert(not rbe::frames::header<Empty>);
static_assert(not rbe::frames::header<AggregateWithPtr>);


// ============================================================
// is_frame -- the shape shared by every level
// ============================================================
static_assert(rbe::frames::is_frame<msg_frame>); // vocabulary frame
static_assert(rbe::frames::is_frame<msg_frame::dsrl_type>); // its deserializer
static_assert(rbe::frames::is_frame<nested>);
static_assert(rbe::frames::is_frame<packet>);

// the framing vocabulary types are payloads, not frames
static_assert(not rbe::frames::is_frame<candidates>);
static_assert(not rbe::frames::is_frame<rbe::frames::many<msg_frame>>);
static_assert(not rbe::frames::is_frame<rbe::frames::blob>);
static_assert(not rbe::frames::is_frame<MessageWithHeader>);
static_assert(not rbe::frames::is_frame<std::uint32_t>);


// ============================================================
// frame serder payload -- the vocabulary level
// ============================================================
static_assert(rbe::frames::serder_payload<MessageWithHeader>); // wirable
static_assert(rbe::frames::serder_payload<rbe::frames::any<MessageWithHeader, MessageWithHeaderPack>>); // any
static_assert(rbe::frames::serder_payload<rbe::frames::many<rbe::frames::frame<CommonHeader, MessageWithHeader>>>); // many
static_assert(rbe::frames::serder_payload<rbe::frames::blob>); // blob
static_assert(rbe::frames::serder_payload<rbe::frames::frame<CommonHeader, rbe::frames::many<rbe::frames::frame<CommonHeader, MessageWithHeader>>>>);
static_assert(rbe::frames::serder_payload<rbe::frames::frame<CommonHeader, rbe::frames::many<rbe::frames::frame<CommonHeader, rbe::frames::any<MessageWithHeader, MessageWithHeaderPack>>>>>);
// neither wirable nor serder_traits
static_assert(not rbe::frames::serder_payload<Empty>);
static_assert(not rbe::frames::serder_payload<AggregateWithPtr>);


// ============================================================
// serder_traits
// ============================================================

// every framing vocabulary type lowers to a deserialization type
static_assert(rbe::frames::serder_traits<candidates>);
static_assert(rbe::frames::serder_traits<rbe::frames::many<msg_frame>>);
static_assert(rbe::frames::serder_traits<rbe::frames::blob>);
static_assert(rbe::frames::serder_traits<msg_frame>);

// a plain wirable message is not a serder_traits, it is lowered as-is
static_assert(not rbe::frames::serder_traits<MessageWithHeader>);
static_assert(not rbe::frames::serder_traits<std::uint32_t>);


// ============================================================
// frame_serder -- the vocabulary level
// ============================================================

static_assert(rbe::frames::frame_serder<rbe::frames::frame<CommonHeader, MessageWithHeader>>);
static_assert(rbe::frames::frame_serder<any_frame>);
static_assert(rbe::frames::frame_serder<blob_frame>);
static_assert(rbe::frames::frame_serder<nested>);
static_assert(rbe::frames::frame_serder<packet>);

// the framing vocabulary types are payloads, not frames
static_assert(not rbe::frames::frame_serder<candidates>);
static_assert(not rbe::frames::frame_serder<rbe::frames::many<msg_frame>>);
static_assert(not rbe::frames::frame_serder<rbe::frames::blob>);
static_assert(not rbe::frames::frame_serder<MessageWithHeader>);
static_assert(not rbe::frames::frame_serder<std::uint32_t>);

// a lowered frame is a dsrl::frame: it has the shape, but not the vocabulary traits
static_assert(rbe::frames::dsrl::is_frame<msg_frame::dsrl_type>);
static_assert(rbe::frames::dsrl::is_frame<packet::dsrl_type>);
static_assert(rbe::frames::is_frame<msg_frame::dsrl_type>);
static_assert(not rbe::frames::frame_serder<msg_frame::dsrl_type>);

// the payload concept is shared by dsrl and srl, so it takes both spans
static_assert(rbe::frames::payload<MessageWithHeader>);
static_assert(rbe::frames::payload<std::span<std::byte const>>); // rbe::frames::blob lowered for dsrl
static_assert(rbe::frames::payload<std::span<std::byte>>); // ... and for srl
static_assert(rbe::frames::payload<msg_frame::dsrl_type>); // a nested lowered frame


// ============================================================
// the deduced delimiting tag
// ============================================================

using rbe::frames::delimiting_concept_t;
using rbe::frames::frame_delimiting_tag;

// what the shape of a frame says, before any frame declares anything
static_assert(std::same_as<frame_delimiting_tag<WithPayloadLength, rbe::frames::blob>, rbe::frames::length_delimited_tag>);
static_assert(std::same_as<frame_delimiting_tag<WithFrameLength, rbe::frames::blob>, rbe::frames::length_delimited_tag>);
static_assert(std::same_as<frame_delimiting_tag<CommonHeader, msg_frame>, rbe::frames::static_delimited_tag>);
static_assert(std::same_as<frame_delimiting_tag<CommonHeader, MessageWithHeader>, rbe::frames::static_delimited_tag>);
static_assert(std::same_as<frame_delimiting_tag<CommonHeader, rbe::frames::blob>, rbe::frames::buffer_delimited_tag>);
static_assert(std::same_as<frame_delimiting_tag<CommonHeader, rbe::frames::many<msg_frame>>, rbe::frames::buffer_delimited_tag>);
static_assert(std::same_as<frame_delimiting_tag<CommonHeader, candidates>, rbe::frames::implicitly_delimited_tag>);
// a length field takes precedence over the static size of the payload
static_assert(std::same_as<frame_delimiting_tag<WithPayloadLength, MessageWithHeader>, rbe::frames::length_delimited_tag>);
// an extended header is O(1) and not a constant: the interior node, neither of the two implicit leaves
static_assert(std::same_as<frame_delimiting_tag<WithHeaderLength, MessageWithHeader>, rbe::frames::implicitly_delimited_tag>);
// a count settles the extent when the element has a constant size: count * stride, no element read
static_assert(std::same_as<frame_delimiting_tag<HeaderWithCount, rbe::frames::many<msg_frame>>, rbe::frames::static_stride_delimited_tag>);
// and when it has not, the count still delimits the run -- by measuring every element, so the frame is
// count-delimited without being constant: what it must not be is bounded by a buffer it does not own
static_assert(std::same_as<frame_delimiting_tag<HeaderWithCount, rbe::frames::many<any_frame>>, rbe::frames::count_delimited_tag>);

// lowering never changes the category, so dsrl::frame::length() agrees with the rbe:: classification
template<typename Header, typename Payload>
inline constexpr bool same_tag_when_lowered = std::same_as<
    frame_delimiting_tag<Header, Payload>, frame_delimiting_tag<Header, rbe::frames::detail::to_dsrl_t<Payload>>>;

static_assert(same_tag_when_lowered<WithFrameLength, rbe::frames::blob>);
static_assert(same_tag_when_lowered<CommonHeader, msg_frame>);
static_assert(same_tag_when_lowered<CommonHeader, MessageWithHeader>);
static_assert(same_tag_when_lowered<CommonHeader, rbe::frames::blob>);
static_assert(same_tag_when_lowered<CommonHeader, candidates>);
static_assert(same_tag_when_lowered<NestedPackParent, rbe::frames::many<any_frame>>);


// ============================================================
// self-delimiting vs buffer-delimited frames
// ============================================================
static_assert(rbe::frames::self_delimited<msg_frame>);
static_assert(rbe::frames::self_delimited<rbe::frames::frame<WithFrameLength, rbe::frames::blob>>);
static_assert(rbe::frames::self_delimited<rbe::frames::frame<WithPayloadLength, rbe::frames::blob>>);
static_assert(rbe::frames::self_delimited<rbe::frames::frame<WithHeaderLength, MessageWithHeader>>);
static_assert(rbe::frames::buffer_delimited<blob_frame>);
static_assert(rbe::frames::buffer_delimited<packet>); // many runs to the end of the buffer
// header_length only delimits the header, the payload still runs to the end of the buffer
static_assert(rbe::frames::buffer_delimited<rbe::frames::frame<WithHeaderLength, rbe::frames::blob>>);

// a nested frame delimits the outer payload only if it delimits itself...
static_assert(rbe::frames::self_delimited<nested>);
static_assert(rbe::frames::buffer_delimited<rbe::frames::frame<CommonHeader, blob_frame>>);
// ...unless the outer header already carries a length
static_assert(rbe::frames::self_delimited<rbe::frames::frame<WithFrameLength, blob_frame>>);

// both concepts are level-agnostic: a lowered frame is delimited exactly like the frame it comes from
static_assert(rbe::frames::self_delimited<msg_frame::dsrl_type>);
static_assert(rbe::frames::self_delimited<nested::dsrl_type>);
static_assert(rbe::frames::buffer_delimited<blob_frame::dsrl_type>);
static_assert(rbe::frames::buffer_delimited<packet::dsrl_type>);
static_assert(not rbe::frames::buffer_delimited<msg_frame::dsrl_type>);
static_assert(not rbe::frames::buffer_delimited<nested::dsrl_type>);
static_assert(not rbe::frames::self_delimited<blob_frame::dsrl_type>);
static_assert(not rbe::frames::self_delimited<packet::dsrl_type>);

// ... but neither accepts something that is not a frame
static_assert(not rbe::frames::self_delimited<MessageWithHeader>);
static_assert(not rbe::frames::buffer_delimited<MessageWithHeader>);
static_assert(not rbe::frames::self_delimited<rbe::frames::blob>);
static_assert(not rbe::frames::buffer_delimited<rbe::frames::blob>);

// ============================================================
// rbe::frames::explicitly_delimited vs rbe::frames::implicitly_delimited
// ============================================================
static_assert(rbe::frames::self_delimited<msg_frame>);
static_assert(rbe::frames::self_delimited<rbe::frames::frame<WithHeaderLength, MessageWithHeader>>);
static_assert(rbe::frames::self_delimited<rbe::frames::frame<WithFrameLength, rbe::frames::blob>>);
static_assert(rbe::frames::self_delimited<rbe::frames::frame<WithPayloadLength, rbe::frames::blob>>);

static_assert(not rbe::frames::explicitly_delimited<msg_frame>);
static_assert(not rbe::frames::explicitly_delimited<rbe::frames::frame<WithHeaderLength, MessageWithHeader>>);
static_assert(rbe::frames::explicitly_delimited<rbe::frames::frame<WithFrameLength, rbe::frames::blob>>);
static_assert(rbe::frames::explicitly_delimited<rbe::frames::frame<WithPayloadLength, rbe::frames::blob>>);

static_assert(rbe::frames::implicitly_delimited<msg_frame>);
static_assert(rbe::frames::implicitly_delimited<rbe::frames::frame<WithHeaderLength, MessageWithHeader>>);
static_assert(not rbe::frames::implicitly_delimited<rbe::frames::frame<WithFrameLength, rbe::frames::blob>>);
static_assert(not rbe::frames::implicitly_delimited<rbe::frames::frame<WithPayloadLength, rbe::frames::blob>>);

static_assert(not rbe::frames::implicitly_delimited<rbe::frames::frame<CommonHeader, blob_frame>>);
static_assert(not rbe::frames::explicitly_delimited<rbe::frames::frame<CommonHeader, blob_frame>>);

// ============================================================
// an any payload -- the id settles the length, with no leaf of its own
// ============================================================
static_assert(rbe::frames::implicitly_delimited<rbe::frames::frame<CommonHeader, candidates>>);
static_assert(not rbe::frames::static_delimited<rbe::frames::frame<CommonHeader, candidates>>);
static_assert(rbe::frames::constant_length<rbe::frames::frame<CommonHeader, candidates>>); // one id lookup
// header_length only delimits the header, so the payload still resolves through the id
static_assert(rbe::frames::implicitly_delimited<rbe::frames::frame<WithHeaderLength, candidates>>);
// a length field is read straight off the wire: explicit, whatever the payload is
static_assert(rbe::frames::length_delimited<rbe::frames::frame<WithFrameLength, rbe::frames::blob>>);
static_assert(rbe::frames::length_delimited<rbe::frames::frame<WithPayloadLength, candidates>>);
static_assert(not rbe::frames::implicitly_delimited<rbe::frames::frame<WithFrameLength, candidates>>);
// ... and the category of a nested frame follows the frame it nests
static_assert(rbe::frames::implicitly_delimited<rbe::frames::frame<CommonHeader, rbe::frames::frame<CommonHeader, candidates>>>);
static_assert(rbe::frames::length_delimited<rbe::frames::frame<WithFrameLength, rbe::frames::frame<CommonHeader, candidates>>>);

// ============================================================
// the delimiting categories -- the mechanism leaves and the cost axis
// ============================================================

using extended_header_frame = rbe::frames::frame<WithHeaderLength, MessageWithHeader>;
// an empty payload only exists at the lowered level: rbe::frames::serder_payload does not admit
// explicitly_empty types, while rbe::frames::payload does
using empty_payload_frame = rbe::frames::dsrl::frame<PlainHeader, ExplictlyEmpty>;
using count_frame         = rbe::frames::frame<HeaderWithCount, rbe::frames::many<msg_frame>>;
using dynamic_count_frame = rbe::frames::frame<HeaderWithCount, rbe::frames::many<any_frame>>;

// the mechanism axis agrees with the rbe:: concepts it replaces
static_assert(rbe::frames::self_delimited<msg_frame>);
static_assert(rbe::frames::buffer_delimited<blob_frame>);
static_assert(rbe::frames::length_delimited<rbe::frames::frame<WithFrameLength, rbe::frames::blob>>);
static_assert(rbe::frames::implicitly_delimited<msg_frame>);

// --- static_delimited: the whole frame is a compile-time constant ---
static_assert(rbe::frames::static_delimited<msg_frame>);
static_assert(rbe::frames::static_length<msg_frame> == rbe::wire_size_of<CommonHeader>() + rbe::wire_size_of<MessageWithHeader>());

// a nested chain is a constant only if every level of it is, which is what the recursion is for
static_assert(rbe::frames::static_delimited<nested>);
static_assert(rbe::frames::static_length<nested> == rbe::wire_size_of<NestedPackParent>() + rbe::frames::static_length<msg_frame>);
static_assert(rbe::frames::static_delimited<rbe::frames::frame<CommonHeader, nested>>);
static_assert(not rbe::frames::static_delimited<rbe::frames::frame<CommonHeader, blob_frame>>);
static_assert(not rbe::frames::static_delimited<rbe::frames::frame<CommonHeader, extended_header_frame>>);

// an empty payload is a frame of its header alone, not a frame that runs to the end of the buffer
static_assert(rbe::frames::static_delimited<empty_payload_frame>);
static_assert(rbe::frames::static_length<empty_payload_frame> == rbe::wire_size_of<PlainHeader>());

// a length field is the authority even when the types would answer, so these are not implicit at all
static_assert(not rbe::frames::static_delimited<rbe::frames::frame<WithFrameLength, MessageWithHeader>>);
static_assert(not rbe::frames::static_delimited<rbe::frames::frame<WithPayloadLength, MessageWithHeader>>);

// an extended header declares its own length on the wire: implicitly delimited, O(1), not a constant
static_assert(rbe::frames::implicitly_delimited<extended_header_frame>);
static_assert(not rbe::frames::static_delimited<extended_header_frame>);
static_assert(rbe::frames::constant_length<extended_header_frame>);

static_assert(not rbe::frames::static_delimited<blob_frame>);
static_assert(not rbe::frames::static_delimited<any_frame>);

// level-agnostic: lowering never changes the extent, so it never changes the constant either
static_assert(rbe::frames::static_delimited<msg_frame::dsrl_type>);
static_assert(rbe::frames::static_delimited<nested::dsrl_type>);
static_assert(rbe::frames::static_length<nested::dsrl_type> == rbe::frames::static_length<nested>);

// --- an id resolves in O(1) at runtime, so the frame is implicit without being a constant ---
static_assert(rbe::frames::implicitly_delimited<any_frame>);
static_assert(rbe::frames::implicitly_delimited<rbe::frames::frame<CommonHeader, any_frame>>);
static_assert(not rbe::frames::static_delimited<any_frame>);
static_assert(rbe::frames::constant_length<any_frame>);
// a frame that is implicit and static is the leaf; one that is implicit and not static is the node
static_assert(rbe::frames::implicitly_delimited<msg_frame> and rbe::frames::static_delimited<msg_frame>);
static_assert(rbe::frames::implicitly_delimited<extended_header_frame>);
static_assert(not rbe::frames::static_delimited<extended_header_frame>);

// --- the stride of a many, which is a property of its element and not of the run ---
static_assert(std::same_as<rbe::frames::many_element_t<rbe::frames::many<msg_frame>>, msg_frame>);
static_assert(rbe::frames::static_stride<rbe::frames::many<msg_frame>>);
static_assert(rbe::frames::static_stride<rbe::frames::many<nested>>);
static_assert(rbe::frames::static_stride<rbe::frames::dsrl::many<msg_frame::dsrl_type>>);
static_assert(rbe::frames::dynamic_stride<rbe::frames::many<rbe::frames::frame<WithFrameLength, rbe::frames::blob>>>); // a field per element
static_assert(rbe::frames::dynamic_stride<rbe::frames::many<any_frame>>); // an id lookup per element

// --- the cost axis holds for every frame the library can form today ---
static_assert(rbe::frames::constant_length<msg_frame>);
static_assert(rbe::frames::constant_length<blob_frame>);
static_assert(rbe::frames::constant_length<any_frame>);
static_assert(rbe::frames::constant_length<nested>);
static_assert(rbe::frames::constant_length<packet>);
static_assert(not rbe::frames::constant_length<MessageWithHeader>); // not a frame

// --- the count mechanism, for the one stride it settles without walking anything ---
static_assert(rbe::frames::static_stride_delimited<count_frame>);
static_assert(rbe::frames::count_delimited<count_frame>); // the leaf refines it
static_assert(rbe::frames::explicitly_delimited<count_frame>); // the count is a field on the wire
static_assert(rbe::frames::constant_length<count_frame>);
static_assert(rbe::frames::static_stride<rbe::frames::many<msg_frame>>);

// a dynamic stride delimits by walking: self-delimiting, and the first category that is not O(1), so such
// a frame carries no length() at all and answers through rbe::frames::distance
static_assert(rbe::frames::count_delimited<dynamic_count_frame>);
static_assert(not rbe::frames::static_stride_delimited<dynamic_count_frame>);
static_assert(not rbe::frames::constant_length<dynamic_count_frame>);
static_assert(not rbe::frames::constant_length<dynamic_count_frame::dsrl_type>);
static_assert(not rbe::frames::detail::member_length<dynamic_count_frame::dsrl_type>); // no length() to call at all
static_assert(requires(dynamic_count_frame::dsrl_type const f) { rbe::frames::distance(f); });
static_assert(rbe::frames::dynamic_stride<rbe::frames::many<any_frame>>);

// --- the tags themselves: refinement is derivation, so a concept is one derived_from ---
static_assert(std::derived_from<rbe::frames::static_delimited_tag, rbe::frames::implicitly_delimited_tag>);
static_assert(std::derived_from<rbe::frames::length_delimited_tag, rbe::frames::explicitly_delimited_tag>);
static_assert(std::derived_from<rbe::frames::static_stride_delimited_tag, rbe::frames::count_delimited_tag>);
static_assert(std::derived_from<rbe::frames::implicitly_delimited_tag, rbe::frames::self_delimited_tag>);
static_assert(std::derived_from<rbe::frames::explicitly_delimited_tag, rbe::frames::self_delimited_tag>);
// the buffer bounding a frame is not the frame finding its own end
static_assert(not std::derived_from<rbe::frames::buffer_delimited_tag, rbe::frames::self_delimited_tag>);

// every frame carries its category as a member, at both levels
static_assert(std::same_as<msg_frame::delimiting_concept, rbe::frames::static_delimited_tag>);
static_assert(std::same_as<msg_frame::dsrl_type::delimiting_concept, rbe::frames::static_delimited_tag>);
static_assert(std::same_as<any_frame::delimiting_concept, rbe::frames::implicitly_delimited_tag>);
static_assert(std::same_as<blob_frame::delimiting_concept, rbe::frames::buffer_delimited_tag>);
static_assert(std::same_as<nested::delimiting_concept, rbe::frames::static_delimited_tag>);
static_assert(std::same_as<count_frame::delimiting_concept, rbe::frames::static_stride_delimited_tag>);
static_assert(std::same_as<extended_header_frame::delimiting_concept, rbe::frames::implicitly_delimited_tag>);
static_assert(std::same_as<rbe::frames::frame<WithFrameLength, rbe::frames::blob>::delimiting_concept, rbe::frames::length_delimited_tag>);

// and the member is what the trait reports, deduction and declaration going through the same door
static_assert(std::same_as<rbe::frames::delimiting_concept_t<msg_frame>, msg_frame::delimiting_concept>);
static_assert(std::same_as<rbe::frames::delimiting_concept_t<msg_frame>, rbe::frames::frame_delimiting_tag<CommonHeader, MessageWithHeader>>);

// --- a frame that resolves its own length: the declaration wins over the deduction ---
// a '\n' terminated line. Its shape says buffer-delimited -- a blob runs to the end of the buffer -- and
// the scan says otherwise: self-delimiting, by no mechanism the hierarchy names, which is the bare root.
struct terminated_line : rbe::frames::dsrl::base_frame<PlainHeader, rbe::frames::blob::dsrl_type> {
  using delimiting_concept = rbe::frames::self_delimited_tag;
};

static_assert(std::same_as<rbe::frames::delimiting_concept_t<terminated_line>, rbe::frames::self_delimited_tag>);
static_assert(rbe::frames::self_delimited<terminated_line>);
static_assert(not rbe::frames::buffer_delimited<terminated_line>); // what the shape alone would have said
static_assert(not rbe::frames::static_delimited<terminated_line>);
// the tag reports the mechanism and only that: what the resolution costs is answered by the member, and
// this fixture declares a category without defining length() at all, so it answers nothing
static_assert(not rbe::frames::constant_length<terminated_line>);

// a declaration made inside a chain of frames reaches the outermost frame, which is what a walk over the
// shapes alone cannot do: by shape, a frame over a blob-payload frame is buffer-delimited
using wrapped_line = rbe::frames::dsrl::frame<CommonHeader, terminated_line>;
static_assert(std::same_as<frame_delimiting_tag<CommonHeader, rbe::frames::dsrl::frame<PlainHeader, rbe::frames::blob::dsrl_type>>, rbe::frames::buffer_delimited_tag>);
static_assert(rbe::frames::self_delimited<wrapped_line>);
static_assert(rbe::frames::implicitly_delimited<wrapped_line>);
// the cost is the chain's and not the outer frame's, which is why the condition recurses: the inner frame
// cannot answer in O(1), so neither can this one, and it has no length() either
static_assert(not rbe::frames::constant_length<wrapped_line>);
static_assert(not rbe::frames::detail::member_length<wrapped_line>);


// ============================================================
// frame member types
// ============================================================
static_assert(std::same_as<msg_frame::header_type, CommonHeader>);
static_assert(std::same_as<msg_frame::payload_type, MessageWithHeader>);
static_assert(std::same_as<packet::header_type, NestedPackParent>);
static_assert(std::same_as<packet::payload_type, rbe::frames::many<any_frame>>);

// the payload of a nested frame is the frame itself, not its lowering
static_assert(std::same_as<nested::payload_type, msg_frame>);


// ============================================================
// to_dsrl_type
// ============================================================

// a wirable payload is its own dsrl type -- deserialization is driven by the strategy, not the type
static_assert(std::same_as<rbe::frames::detail::to_dsrl_t<MessageWithHeader>, MessageWithHeader>);
static_assert(std::same_as<rbe::frames::detail::to_dsrl_t<std::uint32_t>, std::uint32_t>);

// vocabulary types lower to their rbe::frames::dsrl counterpart
static_assert(std::same_as<rbe::frames::detail::to_dsrl_t<candidates>, rbe::frames::dsrl::any<MessageWithHeader, MessageWithHeaderPack>>);
static_assert(std::same_as<rbe::frames::detail::to_dsrl_t<rbe::frames::many<msg_frame>>, rbe::frames::dsrl::many<rbe::frames::dsrl::frame<CommonHeader, MessageWithHeader>>>);
static_assert(std::same_as<rbe::frames::detail::to_dsrl_t<rbe::frames::blob>, std::span<std::byte const>>);
static_assert(std::same_as<rbe::frames::detail::to_dsrl_t<msg_frame>, rbe::frames::dsrl::frame<CommonHeader, MessageWithHeader>>);
// lowering is idempotent: an already lowered type has no dsrl_type of its own
static_assert(std::same_as<rbe::frames::detail::to_dsrl_t<rbe::frames::detail::to_dsrl_t<msg_frame>>, rbe::frames::detail::to_dsrl_t<msg_frame>>);
static_assert(std::same_as<rbe::frames::detail::to_dsrl_t<rbe::frames::dsrl::any<MessageWithHeader, MessageWithHeaderPack>>, rbe::frames::dsrl::any<MessageWithHeader, MessageWithHeaderPack>>);

// the header is never lowered, only the payload is
static_assert(std::same_as<msg_frame::dsrl_type, rbe::frames::dsrl::frame<CommonHeader, MessageWithHeader>>);
static_assert(std::same_as<any_frame::dsrl_type, rbe::frames::dsrl::frame<CommonHeader, rbe::frames::dsrl::any<MessageWithHeader, MessageWithHeaderPack>>>);
static_assert(std::same_as<blob_frame::dsrl_type, rbe::frames::dsrl::frame<CommonHeader, std::span<std::byte const>>>);

// lowering recurses through nesting: frame -> dsrl::frame, many<frame> -> dsrl::many<dsrl::frame>
static_assert(std::same_as<nested::dsrl_type, rbe::frames::dsrl::frame<NestedPackParent, rbe::frames::dsrl::frame<CommonHeader, MessageWithHeader>>>);
static_assert(std::same_as<packet::dsrl_type, rbe::frames::dsrl::frame<NestedPackParent, rbe::frames::dsrl::many<rbe::frames::dsrl::frame<CommonHeader, rbe::frames::dsrl::any<MessageWithHeader, MessageWithHeaderPack>>>>>);
// the lowered frame keeps the header, and carries the lowered payload
static_assert(std::same_as<packet::dsrl_type::header_type, NestedPackParent>);
static_assert(std::same_as<packet::dsrl_type::payload_type, rbe::frames::detail::to_dsrl_t<packet::payload_type>>);
static_assert(std::same_as<packet::dsrl_type::buffer_type, std::span<std::byte const>>);


// ============================================================
// frame_serder_traits
// ============================================================
static_assert(std::same_as<rbe::frames::header_t<msg_frame>, msg_frame::header_type>);
static_assert(std::same_as<rbe::frames::payload_t<msg_frame>, msg_frame::payload_type>);
static_assert(std::same_as<rbe::frames::dsrl_t<msg_frame>, msg_frame::dsrl_type>);
static_assert(std::same_as<rbe::frames::header_t<packet>, NestedPackParent>);
static_assert(std::same_as<rbe::frames::payload_t<packet>, rbe::frames::many<any_frame>>);
static_assert(std::same_as<rbe::frames::dsrl_t<packet>, packet::dsrl_type>);
static_assert(std::same_as<rbe::frames::payload_t<rbe::frames::payload_t<nested>>, MessageWithHeader>);
static_assert(std::same_as<rbe::frames::header_t<rbe::frames::payload_t<nested>>, CommonHeader>);

// clang-format on

} // namespace
