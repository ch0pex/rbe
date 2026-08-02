/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file wirable.hpp
 * @date 30/06/2026
 * @brief Wirable concept definition
 */

#pragma once

// --- Includes ---
#include <rbe/concepts/wirable_primitives.hpp>
#include <rbe/core/static_array.hpp>
#include <rbe/detail/introspection.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <meta>


// --- System ---

namespace rbe {

namespace detail {

template<class T>
consteval auto is_wirable_class_type() -> bool {
  static constexpr auto info = ^^T;
  if constexpr (wirable_primitive<T>) { // leaf case
    return true;
  }

  // check recursively that all it's members are wirable
  if constexpr (is_aggregate_type(info) and is_class_type(info) and not is_empty_type(info)) {
    template for (constexpr auto member: nsdm(info) | std::ranges::to<static_array>()) {
      if (not is_wirable_class_type<typename[:type_of(member):]>())
        return false;
    }
    return true;
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
 *   - It is an aggregate class such that:
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
concept wirable = wirable_primitive<T> or detail::is_wirable_class_type<T>();

template<typename T>
concept wirable_class = std::is_class_v<T> and wirable<T>;

} // namespace rbe
