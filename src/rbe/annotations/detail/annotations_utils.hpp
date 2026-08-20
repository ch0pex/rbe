/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file annotations_utils.hpp
 * @version 1.0
 * @date 15/08/2026
 * @brief Short description
 *
 * Longer description
 */
#pragma once

#include <rbe/annotations/detail/annotations_view.hpp>

namespace rbe::detail {

/// NOTE: most annotations are variables with anonymous types, however those annotations that require arguments are
/// types so we need to normalize them into a list of types to define dimensions
consteval auto types_list(auto... args) {
  return std::array<std::meta::info, sizeof...(args)> {normalize_type(args)...};
}

/**
 * @brief Gathers all the annotations of a given type or non static data member
 *
 * When it's a non static data member it gathers and concat both: the annotations of
 * the member and the type of the member
 *
 * @param info entity to gather rbe annotations from
 * @return a vector with all the annotations of the entity
 */
consteval auto annotation_types_of(std::meta::info const info) -> std::vector<std::meta::info> {
  std::vector<std::meta::info> raw_annotations;
  // TODO: Annotations should be gathered from type aliases ??
  if (is_nonstatic_data_member(info)) {
    auto member_annotations = std::meta::annotations_of(info);
    auto type_annotations   = std::meta::annotations_of(type_of(info));
    raw_annotations = std::views::concat(member_annotations, type_annotations) | std::ranges::to<std::vector>(); //
  }
  else if (std::meta::is_type(info)) {
    raw_annotations = std::meta::annotations_of(info);
  }
  else {
    throw std::meta::exception("info is not a non-static data member or a type", ^^annotation_types_of);
  }

  return raw_annotations | views::rbe_annotations | std::ranges::to<std::vector>();
}

/**
 * @brief Gathers all the annotations of a given type and its members recursively
 *
 * @param info reflection of the type to inspect recursively
 * @return a vector with all the annotations found within the type
 */
consteval auto deep_annotation_types_of(std::meta::info const info) -> std::vector<std::meta::info> {
  std::vector<std::meta::info> result = annotation_types_of(info);
  if ((is_type(info) and not is_class_type(info)) or is_nonstatic_data_member(info)) {
    return result;
  }

  std::ranges::for_each(nsdm(info), [&](std::meta::info const member) {
    result.append_range(deep_annotation_types_of(member));
  });

  return result;
}

consteval auto has_annotation(std::meta::info const info, annotation auto value) -> bool {
  auto rbe_annotations = annotation_types_of(info);
  return std::ranges::contains(rbe_annotations, normalize_type(^^value));
}

consteval auto has_annotation(std::meta::info const info, annotation_list auto annotations) -> bool {
  // all annotations in the list must be appear in the annotations of the type
  auto rbe_annotations = annotation_types_of(info);
  return std::ranges::all_of(annotations | views::rbe_annotations, [&](std::meta::info const annotation) {
    return std::ranges::contains(rbe_annotations, annotation);
  });
}


} // namespace rbe::detail
