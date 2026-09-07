/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file id.hpp
 * @date 07/09/2026
 * @brief Message id annotation
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

namespace detail {

template<std::equality_comparable T>
struct id_value {
  T value;
  consteval explicit id_value(T const value) : value(value) { }
};

} // namespace detail


/// The id annotation is unique across the whole (deep) type, but is not mutually exclusive with any
/// other dimension -- a message may carry an id and any of the length annotations.
struct id_dim {
  static constexpr auto kind = detail::dimension_kind::unique;
};

/**
 * @brief Message id annotation
 */
inline constexpr struct {
  consteval auto operator()(std::equality_comparable auto id) { return detail::id_value(id); }
} id {}; /// < message id

} // namespace rbe

template<>
struct rbe::detail::annotation_traits<std::remove_cvref_t<decltype(rbe::id)>> {
  using dimension = rbe::id_dim;

  static consteval auto check(std::meta::info const /**/, std::meta::info const entity) -> bool { // clang-format off
     return invoke_concept(^^std::equality_comparable, {normalize_type(entity)});
   } // clang-format on
};

template<typename T>
struct rbe::detail::annotation_traits<rbe::detail::id_value<T>> {
  using dimension = rbe::id_dim;
};
