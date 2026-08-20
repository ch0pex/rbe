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
#include <rbe/annotations/detail/base.hpp>

namespace rbe {

/**
 * @brief Memory alignment annotations
 */
inline constexpr struct : detail::base_annotation {} pack {};  /// < pragma pack ABI semantics
inline constexpr struct : detail::base_annotation {} align {}; /// < C++ ABI alignment semantics

} // namespace rbe
