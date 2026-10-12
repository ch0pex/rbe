/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame_concepts.hpp
 * @date 11/09/2026
 * @brief Level-agnostic core of the framing concepts
 *
 * A frame exists at three levels: the rbe:: vocabulary type the user writes (rbe::frames::frame, see
 * frame.hpp), and its rbe::frames::dsrl:: / rbe::frames::srl:: lowerings. Lowering only rewrites the payload
 * *representation*, never the shape of a frame nor the categories a payload may fall into, so the shape
 * (rbe::frames::is_frame) and the payload of a lowered frame (rbe::frames::payload) are written once here and
 * reused by every level. What each level adds on top -- the traits it must expose, the API it must offer -- lives with
 * that level: rbe::frames::frame_serder (frame_serder_concepts.hpp), rbe::frames::dsrl::is_frame,
 * rbe::frames::srl::is_frame.
 *
 * Everything that only depends on the shape -- the delimiting classification of
 * frame_delimiting_tags.hpp and frame_delimiting_concepts.hpp -- is therefore written against
 * rbe::frames::is_frame and applies to a frame and to its lowerings alike.
 */

#pragma once

// --- Includes ---
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/frames/detail/base_tags.hpp>
#include <rbe/frames/frame_delimiting_tags.hpp>

// --- STD ---
#include <concepts>
#include <cstddef>
#include <span>
#include "rbe/annotations/empty.hpp"

namespace rbe::frames {

// TODO: structural concepts, see rbe::frames::dsrl::any / rbe::frames::dsrl::many
template<typename T>
concept is_any = std::derived_from<T, detail::any_tag>;

template<typename T>
concept is_many = std::derived_from<T, detail::many_tag>;

/**
 * @brief The header of a frame, at any level
 *
 * A header is a fixed-layout message, and lowering never changes its type, so there is a single header
 * concept shared by the vocabulary frame and by both lowerings.
 */
template<typename T>
concept header = wirable<T>;

/**
 * @brief The shape shared by every frame, at every level
 *
 * rbe::frames::frame, rbe::frames::dsrl::frame and rbe::frames::srl::frame all expose the same header_type /
 * payload_type pair and, with it, the delimiting category they belong to. This is the weakest thing worth calling a
 * frame, and the only thing the delimiting classification needs: it says nothing about how the frame is (de)serialized,
 * which is what the per-level concepts add.
 *
 * The category is required here, as a member and not as the rbe::frames::delimiting_concept_t that reads it: the
 * trait is constrained on this concept, so asking it here would make the concept depend on itself. What
 * it costs is that a frame written from scratch, deriving from nothing, is not a frame until it declares
 * one -- rbe::frames::frame and rbe::frames::dsrl::base_frame declare rbe::frames::frame_delimiting_tag for everything
 * else.
 */
template<typename T>
concept is_frame = requires {
  typename T::header_type;
  typename T::payload_type;
  requires header<typename T::header_type>;
  requires declares_delimiting_concept<T>;
};

template<typename T>
concept wirable_payload = wirable<T> or explicitly_empty<T>;

template<typename T>
concept wirable_class_payload = wirable_class<T> or explicitly_empty<T>;

/**
 * @brief The payload of a lowered frame, shared by rbe::frames::dsrl and rbe::frames::srl
 *
 * The payload categories are the same on both sides of the wire, and so is the concept: a deserializer views
 * the payload bytes read-only and a serializer writes into them, but a type constructible from
 * std::span<std::byte const> is also constructible from std::span<std::byte>, which converts to it, so
 * testing the writable span covers both lowerings. The vocabulary counterpart is rbe::frames::serder_payload.
 */
template<typename T>
concept payload = //
    wirable_payload<T> // wirable or explicitly_empty
    or is_frame<T> // a nested frame
    or is_any<T> // a set of alternatives resolved by an id
    or is_many<T> // a sequence of frames
    or std::constructible_from<T, std::span<std::byte>>; // an opaque view over the payload bytes

template<is_frame T>
using header_t = T::header_type;

template<is_frame T>
using payload_t = T::payload_type;

/**
 * @brief The frame a `many` is a run of, at any level
 *
 * The element type is what the stride -- and so the cost of walking the run -- is a property of, so the
 * alias is level-agnostic like the rest of this header: rbe::frames::many, rbe::frames::dsrl::many and a many of the
 * user's own all spell it `frame_type`.
 */
template<is_many T>
using many_element_t = T::frame_type;

} // namespace rbe::frames
