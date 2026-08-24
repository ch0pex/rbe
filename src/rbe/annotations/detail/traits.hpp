/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file traits.hpp
 * @date 24/08/2026
 * @brief Short description
 */

#pragma once

// --- Includes ---

// --- STD ---
#include <meta>

namespace rbe::detail {

/**
 * @brief Customization point establishing RBE annotation identity and, optionally, dimension membership.
 *
 * Every RBE annotation type -- an empty tag struct, a payload-carrying struct, or a plain enumeration
 * reused directly (e.g. `endian::order`) -- must specialize this template exactly once, colocated with
 * the annotation's own definition. Completeness of the specialization is what makes `is_rbe_annotation`
 * recognize the type; this mirrors the `rbe::custom<T>` customization point used for user-provided
 * serders (see core/custom.hpp) and replaces base-class inheritance, which enum types cannot
 * participate in.
 *
 * Annotations that don't belong to any orthogonal dimension (free markers like `rbe::fmt`) specialize
 * this template with an empty body -- no `dimension` member required. Annotations that DO belong to a
 * dimension additionally provide `using dimension = SomeDimensionTag;`, naming an (empty) tag type
 * identifying the dimension. Two annotations belong to the same dimension iff they name the same tag
 * type. Adding a new annotation to an existing dimension is therefore just: define the annotation,
 * specialize this trait once, done -- no separate registry to edit.
 */
template<class T>
struct annotation_traits;

consteval auto traits_of(std::meta::info const type) -> std::optional<std::meta::info> { // clang-format off
  auto const traits = substitute( ^^annotation_traits, { type }); // clang-format on
  if (not is_complete_type(traits)) {
    return std::nullopt;
  }
  return traits;
}

/// True iff `type` opted into RBE annotation identity via `annotation_traits<T>`.
consteval auto is_marked_annotation(std::meta::info const type) -> bool { return traits_of(type).has_value(); }


} // namespace rbe::detail
