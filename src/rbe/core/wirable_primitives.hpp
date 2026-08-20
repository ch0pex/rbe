/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file wirable_primitives.hpp
 * @date 02/08/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/core/custom.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <concepts>
#include <meta>
#include <span>

// --- System ---

namespace rbe {

// NOTE: is_array_type is not included here bc remove_all_extents is used when called this function
// I not consider array a trivially_wirable_primitive yet bc we use this concept to dispatch to endian::load/store
// functions. Maybe I should have onne more level of primitive for arrays rather than considering them as
// wirable/trivially_wirable
consteval auto is_trivially_wirable_primitive(std::meta::info const info) -> bool {
  return is_integral_type(info) or is_enum_type(info);
}

consteval auto is_custom_wirable(std::meta::info const info) -> bool { // clang-format off
  return is_complete_type(substitute(^^custom, {info})); // clang-format on
}

consteval auto is_wirable_primitive(std::meta::info const info) -> bool {
  return is_trivially_wirable_primitive(info) or is_custom_wirable(info);
}

template<typename T>
concept trivially_wirable_primitive = is_trivially_wirable_primitive(^^T);

template<typename T>
concept custom_wirable = not trivially_wirable_primitive<T> and is_custom_wirable(^^T);

template<typename T>
concept wirable_primitive = trivially_wirable_primitive<T> or custom_wirable<T>;

} // namespace rbe
