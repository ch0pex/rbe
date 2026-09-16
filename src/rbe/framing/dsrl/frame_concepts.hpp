/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame_concepts.hpp
 * @date 11/09/2026
 * @brief Deserialization level of the framing concepts
 *
 * Only what the dsrl lowering adds on top of the shared core (rbe/framing/frame_concepts.hpp) lives here:
 * the API a frame deserializer must offer. The shape (rbe::is_frame), the header (rbe::frame_header) and the
 * payload (rbe::frame_payload) are used as-is from the enclosing namespace.
 */

#pragma once

// --- Includes ---
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/dsrl/return_type.hpp>
#include <rbe/dsrl/tags.hpp>
#include <rbe/framing/frame_concepts.hpp>

// --- STD ---
#include <concepts>
#include <cstddef>

namespace rbe::dsrl {

/**
 * @brief A frame deserializer: a non-owning view over a frame's bytes that decodes it lazily
 *
 * On top of the shape, this is the API rbe::dsrl::frame offers and the one generic code (flatten, many)
 * relies on.
 */
template<typename T>
concept is_frame = rbe::is_frame<T> and requires(T const ct) {
  typename T::buffer_type;
  typename T::size_type;

  { ct.header() } -> std::same_as<return_type<lazy_t, typename T::header_type>>;
  { ct.header(lazy) } -> std::same_as<return_type<lazy_t, typename T::header_type>>;
  { ct.header(eager) } -> std::same_as<return_type<eager_t, typename T::header_type>>;
  { ct.header(in_place) } -> std::same_as<return_type<in_place_t, typename T::header_type>>;
  // TOOO: payload getter

  { ct.header_span() } -> std::same_as<typename T::buffer_type>;
  { ct.payload_span() } -> std::same_as<typename T::buffer_type>;
  { ct.length() } -> std::same_as<typename T::size_type>;
  { T::length_of(ct.as_span()) } -> std::same_as<typename T::size_type>;
  { ct.header_length() } -> std::same_as<typename T::size_type>;
  { ct.payload_length() } -> std::same_as<typename T::size_type>;
  { ct.as_span() } -> std::same_as<typename T::buffer_type>;
  { ct.data() } -> std::same_as<std::byte const*>;

  requires std::constructible_from<T, typename T::buffer_type>;
  requires frame_payload<typename T::payload_type>;
};

} // namespace rbe::dsrl
