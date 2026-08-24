/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file dimension.hpp
 * @brief Annotation identity, dimension membership and value-extraction machinery
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/traits.hpp>
#include <rbe/core/detail/introspection.hpp>

// --- STD ---
#include <cstdint>
#include <meta>
#include <optional>

namespace rbe::detail {

/**
 * @brief The kind of correctness rule a dimension enforces across an annotated type.
 */
enum class dimension_kind : std::uint8_t {
  exclusive, ///< at most one annotation of the dimension may appear within a single annotation range
  unique, ///< each annotation of the dimension may independently appear at most once across the whole (deep) type
};

/**
 * @brief The dimension tag type `type` belongs to, or a null reflection if it belongs to none.
 */
consteval auto dimension_of(std::meta::info const type) -> std::meta::info { // clang-format off
  auto const traits = traits_of(type);
  if (not traits) {
    return std::meta::info {};
  }
  for (auto const member: members_of(*traits, default_context)) {
    if (is_type_alias(member) and has_identifier(member) and identifier_of(member) == "dimension") {
      return dealias(member);
    }
  }
  return std::meta::info {};
}

/// Reads the `static constexpr dimension_kind kind` member off a dimension tag type.
consteval auto kind_of(std::meta::info const dim) -> dimension_kind {
  for (auto const member: static_data_members_of(dim, default_context)) {
    if (has_identifier(member) and identifier_of(member) == "kind") {
      return extract<dimension_kind>(member);
    }
  }
  throw std::meta::exception("dimension tag type is missing a `static constexpr dimension_kind kind` member", dim);
}

/// Reads the `static constexpr auto default_value` member off a dimension tag type, as a `T`.
template<typename T>
consteval auto default_value_of(std::meta::info const dim) -> T {
  for (auto const member: static_data_members_of(dim, default_context)) {
    if (has_identifier(member) and identifier_of(member) == "default_value") {
      return extract<T>(member);
    }
  }
  throw std::meta::exception("dimension has no default_value for the requested type", dim);
}

/// Filter predicate factory: keep only annotations belonging to dimension `dim`.
consteval auto by_dimension(std::meta::info const dim) {
  return [dim](std::meta::info const a) { return dimension_of(normalize_type(a)) == dim; };
}

/**
 * @brief Extracts the concrete semantic value `T` carried by an annotation instance, if any.
 *
 * Works with zero boilerplate whenever the annotation's own type IS `T` (e.g. reusing `endian::order`
 * enumerators directly as endianness annotations): the annotation's reflected value already *is* the
 * semantic value, so extraction is the identity function. Annotations that carry a different kind of
 * payload (e.g. `rbe::bits`, whose type is not `endian::order`) simply don't match and yield
 * `std::nullopt` -- they still participate in the dimension structurally (exclusivity/uniqueness)
 * without being forced to produce a `T`.
 */
template<typename T>
consteval auto value_of(std::meta::info const annotation_value) -> std::optional<T> {
  if (remove_cvref(type_of(annotation_value)) != ^^T) {
    return std::nullopt;
  }
  return extract<T>(annotation_value);
}

} // namespace rbe::detail
