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
#include <rbe/detail/introspection.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <meta>

// --- System ---

// clang-format off

namespace rbe {

// --- Annotation list ---

template<auto... Args>
inline constexpr detail::annotations_t<Args...>  derive {};


// --- Memory layout annotations ---
inline constexpr struct {} little {};  /// < little-endian byte order semantics
inline constexpr struct {} big {};     /// < big-endian byte order semantics
inline constexpr struct {} pack {};    /// < pragma pack ABI semantics
inline constexpr struct {} no_pack {}; /// < C++ ABI semantics

inline constexpr auto pack_le = derive<pack, little>; /// < pack with little-endian semantics
inline constexpr auto pack_be = derive<pack, big>;    /// < pack with big-endian semantics


// --- Message metadata annotations ---
inline constexpr struct {} id {};     /// < message id
inline constexpr struct {} length {}; /// < message length


// --- Debugging annotations ---

inline constexpr struct { } fmt {};         /// < format message for debugging purposes
inline constexpr auto debug = derive<fmt>;  /// < debug annotation

// clng-format on


} // namespace rbe
