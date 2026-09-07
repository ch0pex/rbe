/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file id.hpp
 * @date 07/09/2026
 * @brief Message id annotations
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

/**
 * The id a message type is dispatched under -- the annotation value produced by `rbe::id(value)`.
 * Never spelled directly: `struct [[=rbe::id(25)]] Heartbeat {};`
 */
template<std::equality_comparable T>
struct id_value {
  T value;
  consteval explicit id_value(T const value) : value(value) { }
};

} // namespace detail

/**
 * `id` and `id(value)` are the two halves of message identity -- where the id lives on the wire, and
 * which id a type is dispatched under -- so they share a dimension: they may not appear in the same
 * annotation range (a field is one or the other, never both), and neither may repeat across the
 * whole (deep) type. They are not exclusive with any other dimension: a message may carry an id and
 * any of the length annotations.
 */
struct id_dim {
  static constexpr auto kind = detail::dimension_kind::exclusive | detail::dimension_kind::unique;
};

/**
 * @brief Message id annotations
 *
 * `[[=rbe::id]]` on a member marks the field the id is read from on the wire; `[[=rbe::id(value)]]`
 * on a struct declares the id that message type is dispatched under. The call is `consteval`, so the
 * value is baked into the annotation's type.
 */
inline constexpr struct {
  consteval auto operator()(std::equality_comparable auto const value) const { // clang-format off
    return detail::id_value {value};
  } // clang-format on
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

  static consteval auto check(std::meta::info const /**/, std::meta::info const entity) -> bool { // clang-format off
    return is_class_type(normalize_type(entity));
  } // clang-format on
};
