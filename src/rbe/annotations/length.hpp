/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file length.hpp
 * @date 07/09/2026
 * @brief Message length annotations
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/annotation_info.hpp>
#include <rbe/core/detail/introspection.hpp>

// --- STD ---
#include <cstddef>
#include <cstdint>

namespace rbe {

namespace detail {

/// Which length a field encodes.
enum class length_kind : std::uint8_t {
  frame, ///< total frame length: header + payload
  payload, ///< payload length: frame minus header
  header, ///< header length
};

/**
 * A field encodes exactly one length, so the three annotations may not share an annotation range;
 * across the whole (deep) type each is independently unique -- a message may carry any combination
 * of frame/payload/header lengths, each at most once.
 */
struct length_dim {
  static constexpr auto kind = dimension_kind::exclusive | dimension_kind::unique;
};

/**
 * The three lengths are three values of one annotation, not three annotations: they share a dimension,
 * a check and a spelling, and they are told apart by their value alone.
 */
struct length_tag {
  using dimension  = length_dim;
  using value_type = length_kind;

  /// The annotated field must be able to hold a length.
  static consteval auto check(annotation_info const /**/, std::meta::info const entity) -> bool {
    return is_convertible_type(normalize_type(entity), ^^std::size_t);
  }
};

} // namespace detail

/// Which of the three lengths on the wire a field encodes.
using length_kind = detail::length_kind;

/**
 * @brief Marks the field that encodes a length on the wire.
 *
 * A type may carry any combination of the three lengths, each on its own field and each at most
 * once; the annotated field must be convertible to `std::size_t`.
 *
 * Length ssemantics:
 * - frame_length: the total frame length, header + payload
 * - payload_length: the payload length, frame minus header
 * - header_length: the header length
 */
inline constexpr detail::annotation_kind<detail::length_tag> length {};
inline constexpr auto frame_length   = length(length_kind::frame);
inline constexpr auto payload_length = length(length_kind::payload);
inline constexpr auto header_length  = length(length_kind::header);

} // namespace rbe
