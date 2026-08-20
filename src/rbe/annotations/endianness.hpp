/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file endianness.hpp
 * @date 20/08/2026
 * @brief Endianness annotations
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/base.hpp>

// --- STD ---
#include <cstdint>

namespace rbe {

/**
 * @brief Explicit bits semantics annotation
 */
struct bits : detail::base_annotation {
  std::uint8_t msb, lsb;
  constexpr explicit bits(std::uint8_t const msb, std::uint8_t const lsb) : msb(msb), lsb(lsb) {}
};

/**
 * @brief Endianness annotations
 */
inline constexpr struct : detail::base_annotation {} native;    /// < native-endian byte order semantics
inline constexpr struct : detail::base_annotation {} little {}; /// < little-endian byte order semantics
inline constexpr struct : detail::base_annotation {} big {};    /// < big-endian byte order semantics

} // namespace rbe
