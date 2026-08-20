/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file base.hpp
 * @date 06/08/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/core/detail/introspection.hpp>

// --- STD ---
#include <meta>
#include <ranges>

namespace rbe::detail {

struct base_annotation { };

template<auto... Args>
struct annotations_t : base_annotation { };

consteval auto is_rbe_annotation(std::meta::info info) -> bool {
  info = not is_type(info) ? type_of(info) : info;
  if (not is_class_type(info)) {
    return false;
  }
  return info == ^^base_annotation or std::ranges::any_of(bases_of(info), [](std::meta::info const base_annotation) {
           return is_rbe_annotation(base_annotation);
         });
}

consteval auto is_annotation_list(std::meta::info info) -> bool {
  info = normalize_type(info);
  return has_template_arguments(info) and template_of(info) == ^^annotations_t;
}


template<typename T>
concept annotation = is_rbe_annotation(^^T) and not is_annotation_list(^^T);

template<typename T>
concept annotation_list = is_rbe_annotation(^^T) and is_annotation_list(^^T);

} // namespace rbe::detail
