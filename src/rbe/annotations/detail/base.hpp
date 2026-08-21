/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file base.hpp
 * @date 06/08/2026
 * @brief Base type and predicates for identifying RBE annotations and annotation lists
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/dimension.hpp>
#include <rbe/core/detail/introspection.hpp>

// --- STD ---
#include <meta>
#include <ranges>

namespace rbe::detail {

template<auto... Args>
struct annotations_t { };

consteval auto is_annotation_list(std::meta::info info) -> bool {
  info = normalize_type(info);
  return has_template_arguments(info) and template_of(info) == ^^annotations_t;
}

/// An entity is a first-class RBE annotation iff it opts in via `annotation_traits<T>` (dimension.hpp)
/// -- either directly, or by being a `derive<...>` list thereof.
consteval auto is_rbe_annotation(std::meta::info info) -> bool {
  auto const type = normalize_type(info);
  return is_annotation_list(type) or is_marked_annotation(type);
}

template<typename T>
concept annotation = is_rbe_annotation(^^T) and not is_annotation_list(^^T);

template<typename T>
concept annotation_list = is_rbe_annotation(^^T) and is_annotation_list(^^T);

} // namespace rbe::detail
