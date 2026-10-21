/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file format.hpp
 * @date 20/08/2026
 * @brief Debugging annotations
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/annotation.hpp>

namespace rbe {

namespace detail {

struct fmt_dim {
  // not `unique`: the formatter is per type, and a formatted type routinely holds members of other
  // formatted types, each carrying its own `fmt`
  static constexpr auto kind = dimension_kind::exclusive | dimension_kind::type_only;
};

struct fmt_tag {
  using dimension = fmt_dim;
};

} // namespace detail

/**
 * @brief Opts the annotated type into RBE's universal formatter.
 *
 * The type becomes printable through `std::format` and `operator<<` (`rbe/core/fmt.hpp`), which
 * walks every member recursively, base classes and bit-fields included. It has a dimension of its own,
 * so it combines freely with any wire annotation.
 *
 * Why `type_only`: the formatter is `std::formatter<T>`, a specialization selected by the type alone.
 * When `std::format("{}", obj.member)` is compiled the member is gone, only its type is looked up, so
 * an annotation written on the member has nothing to reach and would promise a behavior that cannot
 * happen. Hence it is written on the type, once, never on a member, and a nested member type that
 * should print carries its own `fmt`.
 */
inline constexpr detail::annotation_kind<detail::fmt_tag> fmt {};

} // namespace rbe
