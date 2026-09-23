/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame_concepts.hpp
 * @date 11/09/2026
 * @brief Level-agnostic core of the framing concepts
 *
 * A frame exists at three levels: the rbe:: vocabulary type the user writes (rbe::frame, see
 * frame.hpp), and its rbe::dsrl:: / rbe::srl:: lowerings. Lowering only rewrites the payload
 * *representation*, never the shape of a frame nor the categories a payload may fall into, so the shape
 * (rbe::is_frame) and the payload of a lowered frame (rbe::frame_payload) are written once here and reused by
 * every level. What each level adds on top -- the traits it must expose, the API it must offer -- lives with
 * that level: rbe::frame_serder (frame_serder_concepts.hpp), rbe::dsrl::is_frame, rbe::srl::is_frame.
 *
 * Everything that only depends on the shape -- the delimiting classification of
 * frame_delimiting_concepts.hpp and detail/payload_extent.hpp -- is therefore written against
 * rbe::is_frame and applies to a frame and to its lowerings alike.
 */

#pragma once

// --- Includes ---
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/framing/detail/base_tags.hpp>

// --- STD ---
#include <concepts>
#include <cstddef>
#include <span>
#include "rbe/annotations/empty.hpp"

namespace rbe {

// TODO: structural concepts, see rbe::dsrl::any / rbe::dsrl::many
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
concept frame_header = wirable<T>;

/**
 * @brief The shape shared by every frame, at every level
 *
 * rbe::frame, rbe::dsrl::frame and rbe::srl::frame all expose the same header_type / payload_type pair.
 * This is the weakest thing worth calling a frame, and the only thing the delimiting classification needs:
 * it says nothing about how the frame is (de)serialized, which is what the per-level concepts add.
 */
template<typename T>
concept is_frame = requires {
  typename T::header_type;
  typename T::payload_type;
  requires frame_header<typename T::header_type>;
};

template<typename T>
concept frame_wirable = wirable<T> or explicitly_empty<T>;

template<typename T>
concept frame_wirable_class = wirable_class<T> or explicitly_empty<T>;

/**
 * @brief The payload of a lowered frame, shared by rbe::dsrl and rbe::srl
 *
 * The payload categories are the same on both sides of the wire, and so is the concept: a deserializer views
 * the payload bytes read-only and a serializer writes into them, but a type constructible from
 * std::span<std::byte const> is also constructible from std::span<std::byte>, which converts to it, so
 * testing the writable span covers both lowerings. The vocabulary counterpart is rbe::frame_serder_payload.
 */
template<typename T>
concept frame_payload = //
    frame_wirable<T> // wirable or explicitly_empty
    or is_frame<T> // a nested frame
    or is_any<T> // a set of alternatives resolved by an id
    or is_many<T> // a sequence of frames
    or std::constructible_from<T, std::span<std::byte>>; // an opaque view over the payload bytes

template<is_frame T>
using frame_header_t = T::header_type;

template<is_frame T>
using frame_payload_t = T::payload_type;

} // namespace rbe
