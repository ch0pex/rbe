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
#include <rbe/detail/annotation_base.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <meta>

// --- System ---

// clang-format off

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
inline constexpr struct : detail::base_annotation {} native;     /// < native-endian byt order semantics
inline constexpr struct : detail::base_annotation {} little {};  /// < little-endian byte order semantics
inline constexpr struct : detail::base_annotation {} big {};     /// < big-endian byte order semantics

/**
 * @brief Memory alignment annotations
 */
inline constexpr struct : detail::base_annotation {} pack {};    /// < pragma pack ABI semantics
inline constexpr struct : detail::base_annotation {} align {};   /// < C++ ABI alignment semantics



/**
 * @brief Message metadata annotations
 */
inline constexpr struct : detail::base_annotation {} id {};     /// < message id
inline constexpr struct : detail::base_annotation {} length {}; /// < message length


/**
 * @brief Debugging annotations
 */
inline constexpr struct : detail::base_annotation { } fmt {};  /// < format message for debugging purposes


/**
 * @brief Groups rbe annotations into one variable
 *
 * Users can use rbe::derive in annotations directly:
 * [[=rbe::derive<rbe::pack, rbe::native>]]
 *
 * Or they can use it to create reusable group of annotations:
 * inline constexpr auto native_abi = rbe::derive<rbe::align, rbe::native>;
 *
 * And then use it like:
 * [[=native_abi]]
 *
 * @tparam Args Annotations to be grouped
 */
template<auto... Args>
inline constexpr detail::annotations_t<Args...>  derive {};

// --- Builtin Annotation lists ---
inline constexpr auto pack_le = derive<pack, little>; /// < pack with little-endian semantics
inline constexpr auto pack_be = derive<pack, big>;    /// < pack with big-endian semantics
inline constexpr auto debug = derive<fmt>;  /// < debug annotation

// clang-format on

} // namespace rbe
