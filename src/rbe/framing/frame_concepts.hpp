/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame_concepts.hpp
 * @date 11/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/framing/detail/payload_extent.hpp>
#include <rbe/framing/dsrl/frame_concepts.hpp>

// --- STD ---
#include <concepts>
#include "rbe/annotations/length.hpp"
#include "rbe/annotations/well_annotated_concepts.hpp"

namespace rbe {

template<typename T>
concept is_any = true;

template<typename T>
concept is_many = true;

template<typename T>
concept serder_traits = requires(T const ct) {
  typename T::dsrl_type;
  // typename T::srl_type;

  // requires std::constructible_from<typename T::dsrl_type, std::span<std::byte const>>;
  // requires std::constructible_from<typename T::srl_type, std::span<std::byte>>;
};

template<typename T>
concept frame_header = wirable<T>;

/**
 * A frame payload can be the following forms:
 *   - A wirable type (struct, class, array, etc.)
 *   - Something constructible from a span of bytes (e.g. custom parser, std::span, rbe::many)
 *   - rbe::any<Args...>
 */
template<typename T>
concept frame_payload = wirable<T> or serder_traits<T>;

/// Verifies wether a header and a payload ar compatible to conform a frame
// TODO: and rbe::detail::is_compatible<HeaderType, PayloadType>
template<typename HeaderType, typename PayloadType>
concept frame_compatible = frame_header<HeaderType> and frame_payload<PayloadType>;


template<typename T>
concept frame_serder = requires(T const ct) {
  requires frame_header<typename T::header_type>;
  requires frame_payload<typename T::payload_type>;
  requires frame_compatible<typename T::header_type, typename T::payload_type>;
  requires dsrl::is_frame<typename T::dsrl_type>;

  // TODO:
  // requires srl::is_frame<typeanme T::srl_type>;
  // requires value_type_of<typename T::value_type, T>;
};

/**
 * @brief A frame whose length can be resolved without looking at the size of the buffer
 *
 * The length comes from a header field (payload_length, frame_length), from static sizes, or from a nested
 * self-delimiting frame (see rbe::detail::payload_extent). Such a frame may be read from a larger buffer,
 * trailing bytes being padding, which is what allows a sequence of frames to share a single buffer.
 */
template<typename T>
concept self_delimiting_frame = frame_serder<T> and detail::is_self_delimiting<T>();

/**
 * @brief A frame whose payload extends to the end of the buffer it is read from
 *
 * The buffer size is the frame length, so the buffer must cover exactly the frame: trailing bytes are
 * taken as payload, never as padding. Such a frame can only be the last one in a buffer.
 *
 * @note these frames are not iterable: a sequence of them cannot be split without an external length
 */
template<typename T>
concept buffer_delimited_frame = frame_serder<T> and not detail::is_self_delimiting<T>();

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
    and (contains_annotation<typename T::header_type, rbe::frame_length> or
         contains_annotation<typename T::header_type, rbe::payload_length>);

/**
 * @brief A frame whose length is resolved by the implicit size of its underlying types.
 *
 * This concept applies to self-delimiting frames that lack explicit length
 * annotations in their header.
 *
 * @note Determining the length of this kind of frame might be slower when the
 * payload is arbitrary (e.g., `std::any`), as it requires dynamic type
 * dispatching to calculate the total size.
 *
 * @tparam T The frame type to be evaluated.
 */
template<typename T>
concept implicitly_delimited_frame = //
    self_delimiting_frame<T> //
    and not(contains_annotation<typename T::header_type, rbe::frame_length> or
            contains_annotation<typename T::header_type, rbe::payload_length>);

} // namespace rbe
