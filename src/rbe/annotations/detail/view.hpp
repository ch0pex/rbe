/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file view.hpp
 * @version 1.0
 * @date 15/08/2026
 * @brief Range adaptor that flattens annotations and annotation lists into a single view
 */
#pragma once

// --- Includes ---
#include <rbe/annotations/detail/base.hpp>
#include <rbe/core/detail/introspection.hpp>


namespace rbe::detail::views {

consteval auto types_of_rbe_annotation(std::meta::info const info) {
  if (is_annotation_list(info)) {
    auto const args = template_arguments_of(type_of(info));
    return args | std::views::transform(normalize_type) | std::ranges::to<std::vector>();
  }
  return std::vector {normalize_type(info)};
}

struct rbe_annotations_view_fn : std::ranges::range_adaptor_closure<rbe_annotations_view_fn> {
  template<std::ranges::range R>
    requires std::ranges::viewable_range<R>
  consteval auto operator()(R const& r) const {
    return r // all annotations
           | std::views::filter(is_rbe_annotation) // filter out non-rbe annotations
           | std::views::transform(types_of_rbe_annotation) // normalize lists and annotations to ranges of annotations
           | std::views::join; // join back to one single range of annotations
  }

  consteval auto operator()(auto value) const
    requires annotation<decltype(value)> or annotation_list<decltype(value)>
  {
    return types_of_rbe_annotation(^^value);
  }
};

inline constexpr rbe_annotations_view_fn rbe_annotations;

} // namespace rbe::detail::views
