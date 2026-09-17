/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file alignment.hpp
 * @date 20/08/2026
 * @brief Memory alignment annotations
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/annotation.hpp>

// --- STD ---
#include <cstdint>

namespace rbe {

namespace detail {

/**
 * The alignment strategies a type or member can be annotated with. `native` (regular padded layout)
 * is the default and is only ever reached implicitly, as the value nothing along the way overrode;
 * `align` is how it is said out loud.
 */
enum class alignment_mode : std::uint8_t { native, pack, align };

/**
 * At most one alignment-dimension annotation may appear within a single annotation range. The
 * implicit default (no explicit annotation anywhere in scope) is `native` -- regular padded layout.
 */
struct alignment_dim {
  static constexpr auto kind          = dimension_kind::exclusive;
  static constexpr auto default_value = alignment_mode::native;
};

/// Layout semantics: `rbe::alignment(alignment_mode::pack)`, or the `pack`/`align` aliases.
struct alignment_tag {
  using dimension  = alignment_dim;
  using value_type = alignment_mode;
};

} // namespace detail

/// The layout a field can be given on the wire: `pack` (no padding) or `align` (regular C++ layout).
using alignment_mode = detail::alignment_mode;

/**
 * @brief How the annotated struct or member is laid out on the wire.
 *
 * Written on a struct it applies to every member: a member with no alignment of its own inherits
 * it, and a member that states one replaces it for that field. What a member may not do is
 * contradict its own type -- annotate a member whose type already carries an alignment and the two
 * share a single annotation range, so they do not override, they conflict, and compilation fails.
 * An explicitly annotated struct keeps its layout wherever it is used.
 *
 * `pack`/`align` below are the two values named, and are what you normally write -- this spelling
 * is for passing an `alignment_mode` decided elsewhere.
 */
inline constexpr detail::annotation_kind<detail::alignment_tag> alignment {};
inline constexpr auto pack  = alignment(alignment_mode::pack);
inline constexpr auto align = alignment(alignment_mode::align);

} // namespace rbe
