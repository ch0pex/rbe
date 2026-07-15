/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file concepts.hpp
 * @date 30/06/2026
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
#include <type_traits>

// --- System ---

namespace rbe {

namespace detail {

consteval auto wirable(std::meta::info const info) -> bool {
  auto members = nsdm(info);
  return is_class_type(info) //
         and is_aggregate_type(info) //
         and members.size() > 0 // Not supported empty classes
         and bases_of(info).size() == 0 and // Not supported yet base clases
         std::all_of(members.begin(), members.end(), [](std::meta::info const member) {
           auto const type = remove_all_extents(type_of(member));
           return (is_scalar_type(type) and not is_pointer_type(type));
         });
}

} // namespace detail


template<typename T>
concept introspectable = std::meta::is_enumerable_type(^^T);

/**
 * @brief Concept to determine if a given type is suitable to be transmited through
 * the wire using RBE.
 *
 * The type must be:
 *   - An aggregate
 *   - Specifically a class
 *   - With at least one member variable
 *   - With no base classes
 *   - And all it's members must be:
 *     - A scalar type
 *     - Not a pointer or reference
 *
 * To be supported in future versions:
 *   - Nested wirable types in nonstatic member variables
 *   - Base classes
 *   - Variable length fields
 */
template<typename T>
concept wirable = std::is_trivially_copyable_v<T> and introspectable<T> and detail::wirable(^^T);

} // namespace rbe
