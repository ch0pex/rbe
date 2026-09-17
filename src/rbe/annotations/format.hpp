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

struct fmt_tag { };

} // namespace detail

/**
 * @brief Opts the annotated type into RBE's universal formatter.
 *
 * The type becomes printable through `std::format` and `operator<<` (`rbe/core/fmt.hpp`), which
 * walks every member recursively, base classes and bit-fields included. It belongs to no dimension,
 * so it combines freely with any other annotation.
 */
inline constexpr detail::annotation_kind<detail::fmt_tag> fmt {};

} // namespace rbe
