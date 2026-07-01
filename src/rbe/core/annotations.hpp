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

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---


namespace rbe {

// clang-format off


// annotations list
template<auto... Args>
struct annotations_t { };

template<auto... Args>
inline constexpr annotations_t<Args...> annotations { };


// memory layout annotations
inline constexpr struct{} little {};
inline constexpr struct{} big {};
inline constexpr struct{} pack {};
inline constexpr auto pack_le = annotations<pack, little>;
inline constexpr auto pack_be = annotations<pack, big>;


// message metadata annotations
inline constexpr struct{} id {};
inline constexpr struct{} length {};


// debugging annotations 

inline constexpr struct{} fmt {};
inline constexpr auto debug = annotations<fmt>;






} // namespace rbe
