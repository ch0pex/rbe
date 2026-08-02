/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file trivially_wirable.hpp
 * @date 31/07/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/core/memory_layout.hpp>
#include "rbe/concepts/wirable_primitives.hpp"
#include "rbe/concepts/wirable.hpp"

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---

namespace rbe {

namespace detail {

template<typename T>
consteval auto is_trivially_wirable_type() -> bool {
  static constexpr auto info = ^^T;
  if (not is_class_type(info)) {
    return is_integral_type(info) or is_enum_type(info);
  }
  return is_trivially_copyable_type(info) //
         and is_standard_layout_type(info) //
         and get_struct_layout<T>() == get_wire_layout<T>(); //
}

} // namespace detail

/**
 * @brief Concept to check if a type is trivially wirable.
 *
 * A type is considered trivially wirable if its struct layout matches its wire layout.
 * Meaning that the in-memory representation of the type can be directly used for serialization without any additional
 * processing. Therefore, serialization and deserialization can be performed by simply memcpy'ing the data to and from a
 * buffer.
 *
 */
template<typename T>
concept trivially_wirable = wirable<T> and not custom_wirable<T> and detail::is_trivially_wirable_type<T>();

template<typename T>
concept trivially_wirable_class = std::is_class_v<T> and trivially_wirable<T>;


} // namespace rbe
