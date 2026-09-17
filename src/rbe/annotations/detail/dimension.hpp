/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file dimension.hpp
 * @brief Annotation dimensions: the orthogonal groups annotations belong to, and the correctness
 *        rules each group enforces
 */

#pragma once

// --- Includes ---
#include <rbe/core/detail/introspection.hpp>

// --- STD ---
#include <cstdint>
#include <meta>
#include <utility>

namespace rbe::detail {

/**
 * @brief The set of correctness rules a dimension enforces across an annotated type.
 *
 * The rules are independent and combinable with `|`: a dimension whose annotations are both mutually
 * exclusive locally and non-repeatable globally (e.g. the id dimension, where `id` and `id(value)`
 * may not share an annotation range and neither may repeat across the message) declares
 * `exclusive | unique`.
 */
enum class dimension_kind : std::uint8_t {
  exclusive = 1 << 0, ///< at most one annotation of the dimension may appear within a single annotation range
  unique    = 1 << 1, ///< each annotation of the dimension may independently appear at most once across the whole (deep) type
};

consteval auto operator|(dimension_kind const lhs, dimension_kind const rhs) -> dimension_kind {
  return static_cast<dimension_kind>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

/// Whether `kind` includes the given `rule`.
consteval auto enforces(dimension_kind const kind, dimension_kind const rule) -> bool {
  return (std::to_underlying(kind) & std::to_underlying(rule)) != 0;
}

/**
 * @brief What makes two annotations produced by the same factory "the same annotation" when counting
 * repetitions for `dimension_kind::unique`.
 *
 * `value` (the default) is the intuitive reading and the one annotations want whenever the factory's
 * values are alternatives rather than payloads: `rbe::frame_length` and `rbe::payload_length` are two
 * values of one annotation type, and each is independently unique. `kind` is for annotations whose
 * value is a payload rather than an alternative -- `rbe::id(1)` and `rbe::id(2)` are the same
 * annotation said twice, and a message carries one id whatever its value.
 */
enum class identity_kind : std::uint8_t { value, kind };

/// Reads the `static constexpr dimension_kind kind` member off a dimension tag type.
consteval auto kind_of(std::meta::info const dim) -> dimension_kind {
  if (auto const member = static_data_member(dim, "kind")) {
    return extract<dimension_kind>(*member);
  }
  throw std::meta::exception("dimension tag type is missing a `static constexpr dimension_kind kind` member", dim);
}

} // namespace rbe::detail
