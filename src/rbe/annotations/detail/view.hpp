/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file view.hpp
 * @version 2.0
 * @date 15/08/2026
 * @brief Range adaptor that flattens annotations and annotation lists into a single view
 */
#pragma once

// --- Includes ---
#include <rbe/annotations/detail/annotation_info.hpp>

// --- STD ---
#include <ranges>
#include <vector>

namespace rbe::detail::views {

/**
 * @brief The annotations one raw attribute denotes: a `derive<...>` list expands into its
 * constituents (recursively), a plain RBE annotation is itself, anything else vanishes.
 */
consteval auto expand_annotation(std::meta::info const raw) -> std::vector<annotation_info> {
  if (is_annotation_list(raw)) {
    std::vector<annotation_info> expanded;
    for (auto const arg: template_arguments_of(normalize_type(raw))) {
      expanded.append_range(expand_annotation(arg));
    }
    return expanded;
  }
  return is_rbe_annotation(raw) ? std::vector {annotation_info {raw}} : std::vector<annotation_info> {};
}

struct annotations_view_fn : std::ranges::range_adaptor_closure<annotations_view_fn> {
  template<std::ranges::range R>
    requires std::ranges::viewable_range<R>
  consteval auto operator()(R const& r) const {
    return r // all annotations
           | std::views::transform(expand_annotation) // drop non-rbe ones, expand the lists
           | std::views::join; // join back to one single range of annotations
  }
};

inline constexpr annotations_view_fn annotations;

} // namespace rbe::detail::views
