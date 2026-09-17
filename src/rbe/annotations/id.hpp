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
#include <rbe/annotations/detail/annotation_info.hpp>
#include <rbe/core/detail/introspection.hpp>
#include <rbe/core/detail/invoke_concept.hpp>

// --- STD ---
#include <concepts>

namespace rbe {

namespace detail {

struct id_dim {
  static constexpr auto kind = dimension_kind::exclusive | dimension_kind::unique;
};

struct id_tag { // clang-format off
  using dimension  = id_dim;
  using value_type = deduced;

  static constexpr auto marker   = true; ///< the bare `rbe::id` is an annotation of its own
  static constexpr auto identity = identity_kind::kind;

  static consteval auto check(annotation_info const ann, std::meta::info const entity) -> bool {
    if (ann.has_value()) { // rbe::id(value): declares the id of a message type
      return is_class_type(normalize_type(entity))
             and invoke_concept(^^std::equality_comparable, {ann.value_type()});
    }
    // rbe::id: marks the field the id is read from
    return invoke_concept(^^std::equality_comparable, {normalize_type(entity)});
  }
}; // clang-format on

} // namespace detail

/**
 * @brief Where a type's id lives on the wire, and which id a message type answers to.
 *
 * Ids can be used to recognize a message type on the wire, and to select the right type
 * to deserialize into.
 *
 * To express ids there are two forms of the annotation:
 *   - `rbe::id(value)` declares the id of a type.
 *   - `rbe::id` marks the field that carries the id.
 *
 * @note The field annotated and the value annotated as id must be comparable.
 */
inline constexpr detail::annotation_kind<detail::id_tag> id {};

} // namespace rbe
