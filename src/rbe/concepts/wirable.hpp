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
#include <rbe/detail/introspection.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <algorithm>
#include <meta>

// --- System ---

namespace rbe {

template<class T>
struct serder;

template<class T>
concept custom_wire = requires(T t) {
  { serder<T>::deserialize(std::span<std::byte const> {}) } -> std::same_as<T>;
  { serder<T>::serialize(std::span<std::byte> {}, t) } -> std::same_as<std::size_t>;
};

namespace detail {

consteval auto has_custom_serder(std::meta::info const type) -> bool {
  if (not is_class_type(type)) {
    return false;
  }
  std::vector params {type};
  return is_complete_type(substitute(^^serder, params));
}

consteval auto is_wirable_type(std::meta::info const info) -> bool {
  return is_arithmetic_type(info) or is_enum_type(info) or has_custom_serder(info) // leaf cases
         or ( //
                is_aggregate_type(info) // Must be an aggregate
                and is_class_type(info) // Not union, not array
                and not is_empty_type(info) // Not supported empty classes
                and std::ranges::all_of(nsdm(info), is_wirable_type, std::meta::type_of)
            );
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
 */
template<typename T>
concept wirable = detail::is_wirable_type(^^T);

} // namespace rbe
