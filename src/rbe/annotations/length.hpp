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
#include <rbe/annotations/detail/dimension.hpp>
#include <rbe/core/detail/introspection.hpp>

// --- STD ---
#include <cstddef>
#include <type_traits>

namespace rbe {

/// The three length annotations are each independently unique across the whole (deep) type, but are
/// NOT mutually exclusive with each other -- a message may carry any combination of frame/payload/
/// header lengths, each at most once.
/// NOTE: length annotations require an entity convertible to std::size_t
struct length_dim {
  static constexpr auto kind = detail::dimension_kind::unique;
};

/**
 * @brief Message length annotations
 */
// clang-format off
inline constexpr struct {} frame_length {};   /// < total frame length: header + payload
inline constexpr struct {} payload_length {}; /// < payload length: frame minus header
inline constexpr struct {} header_length {};  /// < header length
// clang-format on

} // namespace rbe

template<>
struct rbe::detail::annotation_traits<std::remove_cvref_t<decltype(rbe::frame_length)>> {
  using dimension = rbe::length_dim;

  static consteval auto check(std::meta::info const /**/, std::meta::info const entity) -> bool { // clang-format off
    return is_convertible_type(normalize_type(entity), ^^std::size_t);
  } // clang-format on
};

template<>
struct rbe::detail::annotation_traits<std::remove_cvref_t<decltype(rbe::payload_length)>> {
  using dimension = rbe::length_dim;

  static consteval auto check(std::meta::info const /**/, std::meta::info const entity) -> bool { // clang-format off
    return is_convertible_type(normalize_type(entity), ^^std::size_t);
  } // clang-format on
};

template<>
struct rbe::detail::annotation_traits<std::remove_cvref_t<decltype(rbe::header_length)>> {
  using dimension = rbe::length_dim;

  static consteval auto check(std::meta::info const /**/, std::meta::info const entity) -> bool { // clang-format off
    return is_convertible_type(normalize_type(entity), ^^std::size_t);
  } // clang-format on
};
