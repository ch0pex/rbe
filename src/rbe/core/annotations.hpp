/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file annotations.hpp
 * @date 24/06/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/core/endian.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---


namespace rbe {

// memory layout annotations
inline constexpr endian::order little = endian::order::little;
inline constexpr endian::order big    = endian::order::big;
inline constexpr bool packing         = true;

// message metadata annotations
inline constexpr bool id     = true;
inline constexpr bool length = true;

} // namespace rbe
