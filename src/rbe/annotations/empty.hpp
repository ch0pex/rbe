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
  // not `unique`: a type holding two members of one empty type is not annotated twice
  static constexpr auto kind = dimension_kind::exclusive | dimension_kind::type_only;
};

struct empty_tag {
  using dimension = empty_dim;
};

} // namespace detail

/**
 * @brief Marks a type as empty, which means it has no member variables and no base classes.
 *
 * An empty struct is not wirable on its own, it has nothing to write; the annotation makes it a valid
 * zero-byte payload and `any` candidate. It is written on the type, never on a member, and combines
 * freely with annotations of other dimensions (`id`, `pack`, ...).
 *
 */
inline constexpr detail::annotation_kind<detail::empty_tag> empty {};

/**
 */
template<typename T>
concept explicitly_empty = contains_annotation<T, empty> and std::is_empty_v<T>;

} // namespace rbe
