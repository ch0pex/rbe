/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame_delimiting_concepts.hpp
 * @date 11/09/2026
 * @brief Compile-time classification of how a frame's length is resolved
 *
 * These concepts only look at the shape of a frame (rbe::is_frame), so they apply to an rbe:: vocabulary
 * frame and to its dsrl:: / srl:: lowerings alike: lowering never changes how a frame is delimited.
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/annotation_concepts.hpp>
#include <rbe/annotations/length.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/framing/detail/payload_extent.hpp>
#include <rbe/framing/frame_concepts.hpp>

// --- STD ---
#include <concepts>

namespace rbe {

/**
 * @brief A frame whose length can be resolved without looking at the size of the buffer
 *
 * The length comes from a header field (payload_length, frame_length), from static sizes, or from a nested
 * self-delimiting frame (see rbe::detail::payload_extent). Such a frame may be read from a larger buffer,
 * trailing bytes being padding, which is what allows a sequence of frames to share a single buffer.
 */
template<typename T>
concept self_delimiting_frame = is_frame<T> and detail::is_self_delimiting<T>();

/**
 * @brief A frame whose payload extends to the end of the buffer it is read from
 *
 * The buffer size is the frame length, so the buffer must cover exactly the frame: trailing bytes are
 * taken as payload, never as padding. Such a frame can only be the last one in a buffer.
 *
 * @note these frames are not iterable: a sequence of them cannot be split without an external length
 */
template<typename T>
concept buffer_delimited_frame = is_frame<T> and not detail::is_self_delimiting<T>();

/**
 * @brief A frame whose length is explicitly resolved by reading a wire field.
 *
 * This concept requires the frame to be self-delimiting and checks if its
 * header contains an explicit annotation for either the total frame length
 * (`rbe::frame_length`) or the payload length (`rbe::payload_length`).
 *
 * @tparam T The frame type to be evaluated.
 */
template<typename T>
concept explicitly_delimited_frame = //
    self_delimiting_frame<T> //
    and (contains_annotation<frame_header_t<T>, rbe::frame_length> or
         contains_annotation<frame_header_t<T>, rbe::payload_length>);

/**
 * @brief A frame whose length is resolved by the implicit size of its underlying types.
 *
 * This concept applies to self-delimiting frames that lack explicit length
 * annotations in their header.
 *
 * @note Determining the length of this kind of frame might be slower when the
 * payload is arbitrary (e.g., `std::any`), as it requires dynamic type
 * dispatching to calculate the total size. The `dispatch_delimited_frame` concept
 * is provided for cases where the payload is of type `rbe::any`.
 *
 * @tparam T The frame type to be evaluated.
 */
template<typename T>
concept implicitly_delimited_frame = //
    self_delimiting_frame<T> //
    and not(contains_annotation<frame_header_t<T>, rbe::frame_length> or
            contains_annotation<frame_header_t<T>, rbe::payload_length>);

/**
 * @brief A frame whose length is resolved dynamically at runtime, typically because its payload is of type `rbe::any`.
 */
template<typename T>
concept dispatch_delimited_frame = self_delimiting_frame<T> and detail::is_dispatch_delimited<T>();

} // namespace rbe
