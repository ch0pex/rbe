/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file metadata.hpp
 * @date 20/08/2026
 * @brief Message metadata annotations
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/dimension.hpp>
#include <rbe/core/detail/introspection.hpp>
#include <rbe/core/detail/invoke_concept.hpp>

// --- STD ---
#include <concepts>
#include <type_traits>

namespace rbe {

/// id and the three length annotations are each independently unique across the whole (deep) type,
/// but are NOT mutually exclusive with each other -- a message may carry an id and any combination
/// of frame/payload/header lengths, each at most once.
/// NOTE: id annotation requires operator== on the entity annotated; the length annotations require
/// an entity convertible to std::size_t
struct metadata_dim {
  static constexpr auto kind = detail::dimension_kind::unique;
};

/**
 * @brief Message metadata annotations
 */
// clang-format off
inline constexpr struct {} id {};     /// < message id

inline constexpr struct {} frame_length {};   /// < total frame length: header + payload
inline constexpr struct {} payload_length {}; /// < payload length: frame minus header
inline constexpr struct {} header_length {};  /// < header length

// clang-format on

} // namespace rbe

template<>
struct rbe::detail::annotation_traits<std::remove_cvref_t<decltype(rbe::id)>> {
  using dimension = rbe::metadata_dim;

  static consteval auto check(std::meta::info const /**/, std::meta::info const entity) -> bool { // clang-format off
     return invoke_concept(^^std::equality_comparable, {normalize_type(entity)});
   } // clang-format on
};

template<>
struct rbe::detail::annotation_traits<std::remove_cvref_t<decltype(rbe::frame_length)>> {
  using dimension = rbe::metadata_dim;

  static consteval auto check(std::meta::info const /**/, std::meta::info const entity) -> bool { // clang-format off
    return is_convertible_type(normalize_type(entity), ^^std::size_t);
  } // clang-format on
};

template<>
struct rbe::detail::annotation_traits<std::remove_cvref_t<decltype(rbe::payload_length)>> {
  using dimension = rbe::metadata_dim;

  static consteval auto check(std::meta::info const /**/, std::meta::info const entity) -> bool { // clang-format off
    return is_convertible_type(normalize_type(entity), ^^std::size_t);
  } // clang-format on
};

template<>
struct rbe::detail::annotation_traits<std::remove_cvref_t<decltype(rbe::header_length)>> {
  using dimension = rbe::metadata_dim;

  static consteval auto check(std::meta::info const /**/, std::meta::info const entity) -> bool { // clang-format off
    return is_convertible_type(normalize_type(entity), ^^std::size_t);
  } // clang-format on
};
