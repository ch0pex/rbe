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
#include <rbe/annotations/detail/dimension.hpp>

// --- STD ---
#include <type_traits>

namespace rbe {

/**
 * @brief Debugging annotations
 */
inline constexpr struct {} fmt {}; /// < format message for debugging purposes

} // namespace rbe

/// `fmt` opts into RBE annotation identity but belongs to no dimension: no correctness rule is ever
/// enforced for it, and well_annotated's generic dimension loop never even sees it.
template<>
struct rbe::detail::annotation_traits<std::remove_cvref_t<decltype(rbe::fmt)>> { };
