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
inline constexpr struct {} little {};
inline constexpr struct {} big {};
inline constexpr struct {} pack {};
inline constexpr auto pack_le = derive<pack, little>;
inline constexpr auto pack_be = derive<pack, big>;


// --- Message metadata annotations ---
inline constexpr struct {} id {};
inline constexpr struct {} length {};


// --- Debugging annotations ---

inline constexpr struct { } fmt {};
inline constexpr auto debug = derive<fmt>;

// clng-format on


} // namespace rbe
