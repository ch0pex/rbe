/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file wirable_concepts.hpp
 * @date 30/06/2026
 * @brief Wirable concept definition
 */

#pragma once

// --- Includes ---
#include <rbe/core/detail/introspection.hpp>
#include <rbe/core/detail/static_array.hpp>
#include <rbe/core/wirable_primitives.hpp>

// --- STD ---
#include <meta>
#include <type_traits>


// --- System ---

namespace rbe {

namespace detail {

consteval auto is_wirable_class_type(std::meta::info type) -> bool {
  type = normalize_type(type);
  if (is_wirable_primitive(remove_all_extents(type))) { // leaf case
    return true;
  }

  if (is_class_type(type) and not is_empty_type(type)) {
    return std::ranges::all_of(nsdm(type), is_wirable_class_type, std::meta::type_of);
  }
  return false;
}

} // namespace detail

template<typename T>
concept introspectable = std::meta::is_enumerable_type(^^T);

/**
 * @brief Concept to determine if a given type is suitable to be transmitted
 *        through the wire using RBE.
 *
 * A type is wirable if it satisfies any of the following:
 *   - It is an arithmetic type (`std::is_arithmetic_v`)
 *   - It is an enumeration type (`std::is_enum_v`)
 *   - It has a custom serder (user-provided or RBE-provided specialization)
 *   - It is a class such that:
 *       - It has at least one member variable
         - Member variables exist in only one class within the inheritance
 *         hierarchy (inheritance is allowed, but fields cannot be split
 *         across multiple levels)
 *       - All member variables are wirable
 *
 * Not supported yet:
 *   - Nested wirable types in non-static member variables
 *   - Base classes
 *   - Array types
 *
 */
template<typename T>
concept wirable = wirable_primitive<T> or detail::is_wirable_class_type(^^T);

template<typename T>
concept wirable_class = std::is_class_v<T> and wirable<T>;

template<typename T>
concept wirable_range = wirable<T> and std::ranges::range<T>;

} // namespace rbe
