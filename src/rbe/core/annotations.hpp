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


namespace rbe {

// clang-format off

namespace detail {

template<auto... Args>
struct annotations_t { };

consteval bool is_annotation_list(std::meta::info const info) { 
  return has_template_arguments(type_of(info)) and template_of(type_of(info)) == ^^annotations_t; 
}

consteval auto has_annotation(std::meta::info const info, auto const& value) {
  auto expected = std::meta::reflect_constant(value);
  for (std::meta::info a: annotations_of(info)) {
    if (std::meta::constant_of(a) == expected) {
      return true;
    }

    if (is_annotation_list(a)) {
      for (auto const a2: template_arguments_of(type_of(a))) {
        if (constant_of(a2) == expected) {
          return true;
        }
      }
    }
  }
  return false;
}

} // namespace detail


// annotation list

template<auto... Args>
inline constexpr detail::annotations_t<Args...>  ann {};


// memory layout annotations
inline constexpr struct {} little {};
inline constexpr struct {} big {};
inline constexpr struct {} pack {};
inline constexpr auto pack_le = ann<pack, little>;
inline constexpr auto pack_be = ann<pack, big>;


// message metadata annotations
inline constexpr struct {} id {};
inline constexpr struct {} length {};


// debugging annotations

inline constexpr struct { } fmt {};
inline constexpr auto debug = ann<fmt>;

// clng-format on


} // namespace rbe
