/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file endianness.hpp
 * @date 20/08/2026
 * @brief Endianness annotations
 *
 * The endianness dimension is declared here; `rbe::bits` joins it from `bits.hpp`.
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/annotation.hpp>
#include <rbe/core/endian.hpp>

// --- STD ---
#include <cstdint>

namespace rbe {

namespace detail {

struct endianness_dim {
  static constexpr auto kind = dimension_kind::exclusive;

  /// NOTE: little endian is defaulted here becouse it is the most common on the wire
  /// we need determinisitc wire representation accross different machines to avoid
  /// ABI incomppatibilities, with order::native two machines with different endianness would
  /// serialize the same struct differently making it imposible to communcate between them.
  static constexpr auto default_value = endian::order::little;
};

struct order_tag {
  using dimension  = endianness_dim;
  using value_type = endian::order;
};

} // namespace detail

/**
 * @brief The byte order the annotated struct or member is serialized in.
 *
 * Written on a struct it applies to every member: a member with no byte order of its own inherits
 * it, and a member that states one replaces it for that field. What a member may not do is
 * contradict its own type -- annotate a member whose type already carries a byte order and the two
 *
 * share a single annotation range, so they do not override, they conflict, and compilation fails.
 * An explicitly annotated struct keeps its byte order wherever it is used.
 *
 * With no endianness annotation anywhere in scope, fields are little-endian -- a fixed byte order,
 * not the host's, so the same struct reaches the wire the same way from any machine.
 */
inline constexpr detail::annotation_kind<detail::order_tag> order {};
inline constexpr auto little = order(endian::order::little);
inline constexpr auto big    = order(endian::order::big);

} // namespace rbe
