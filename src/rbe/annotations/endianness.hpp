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
#include <rbe/annotations/detail/dimension.hpp>
#include <rbe/core/endian.hpp>

// --- STD ---
#include <cstdint>

namespace rbe {

/// At most one endianness-dimension annotation may appear within a single annotation range. The
/// implicit default (no explicit annotation anywhere in scope) is the platform's native byte order,
/// resolved at compile time -- there is no explicit `native` spelling; see docs/explanation/
/// annotation-system-v2.md for why.
struct endianness_dim {
  static constexpr auto kind          = detail::dimension_kind::exclusive;
  static constexpr auto default_value = endian::order::native;
};

/**
 * @brief Explicit bits semantics annotation
 */
struct bits {
  std::uint8_t msb, lsb;
  constexpr explicit bits(std::uint8_t const msb, std::uint8_t const lsb) : msb(msb), lsb(lsb) {}
};

/**
 * @brief Endianness annotations
 *
 * `little`/`big` are literally `endian::order::little`/`endian::order::big` -- the standard enum
 * reused directly as the annotation value.
 */
inline constexpr auto little = endian::order::little; /// < little-endian byte order semantics
inline constexpr auto big    = endian::order::big;    /// < big-endian byte order semantics

} // namespace rbe

template<>
struct rbe::detail::annotation_traits<rbe::endian::order> {
  using dimension = rbe::endianness_dim;
};

template<>
struct rbe::detail::annotation_traits<rbe::bits> {
  using dimension = rbe::endianness_dim;
};
