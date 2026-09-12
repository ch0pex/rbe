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
#include <rbe/dsrl/return_type.hpp>
#include <rbe/dsrl/tags.hpp>

// --- STD ---

namespace rbe::dsrl {


template<typename T>
concept is_any = true;

template<typename T>
concept is_many = true;

// template<typename T>
// concept is_any = requires(T const ct) {
//   typename T::id_type;
//   typename T::types // TODO: naming
//
//       // TODO: mock it with overload set
//       {ct.match()};
//
//   requires std::constructible_from<T, typename T::id_type, std::span<std::byte const>>;
// };


template<typename T>
concept frame_header = wirable<T>;

template<typename T>
concept frame_payload = wirable<T> or is_any<T> or is_many<T> or std::constructible_from<T, std::span<std::byte const>>;

template<typename T>
concept is_frame = requires(T const ct) {
  typename T::header_type;
  typename T::payload_type;
  typename T::buffer_type;
  typename T::size_type;

  { ct.header(lazy) } -> std::same_as<return_type<lazy_t, typename T::header_type>>;
  { ct.header(eager) } -> std::same_as<return_type<eager_t, typename T::header_type>>;
  { ct.header(in_place) } -> std::same_as<return_type<in_place_t, typename T::header_type>>;
  // TOOO: payload getter

  { ct.header_span() } -> std::same_as<typename T::buffer_type>;
  { ct.payload_span() } -> std::same_as<typename T::buffer_type>;
  { ct.length() } -> std::same_as<typename T::size_type>;
  { ct.header_length() } -> std::same_as<typename T::size_type>;
  { ct.payload_length() } -> std::same_as<typename T::size_type>;
  { ct.as_span() } -> std::same_as<typename T::buffer_type>;
  { ct.data() } -> std::same_as<typename T::buffer_type>;


  requires std::constructible_from<T, typename T::buffer_type>;
  requires frame_header<typename T::header_type>;
  requires frame_payload<typename T::payload_type>;
};

} // namespace rbe::dsrl
