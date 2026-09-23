/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file empty.hpp
 * @date 22/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/annotation_concepts.hpp>

// --- STD ---
#include <type_traits>

namespace rbe {

namespace detail {

struct empty_dim {
  static constexpr auto kind = dimension_kind::exclusive | dimension_kind::unique;
};

struct empty_tag {
  using dimension = empty_dim;
};

} // namespace detail

/**
 * @brief Marks a type as empty, which means it has no member variables and no base classes.
 *
 * This annotation is used to indicate that a type is empty, which can be useful for optimization
 * purposes. It is an exclusive annotation, meaning that it cannot be combined with other annotations
 *
 */
inline constexpr detail::annotation_kind<detail::empty_tag> empty {};

/**
 */
template<typename T>
concept explicitly_empty = contains_annotation<T, empty> and std::is_empty_v<T>;

} // namespace rbe
