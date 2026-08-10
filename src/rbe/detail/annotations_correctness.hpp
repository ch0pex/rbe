/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file annotations_correctness.hpp
 * @version 1.0
 * @date 15/08/2026
 * @brief Short description
 *
 * Longer description
 */
#pragma once

#include <rbe/core/annotations.hpp>
#include <rbe/detail/annotations_utils.hpp>
#include <rbe/detail/annotations_view.hpp>

namespace rbe::detail {


inline constexpr auto all_annotations = types_list(^^pack, ^^align, ^^little, ^^big, ^^native, ^^bits, ^^id, ^^length);

/// --- Annotations' dimensions ---
/// Two annotations from the same dimension cannot coexist within the same annotation range.
inline constexpr auto alignment  = types_list(^^pack, ^^align); /// < list of layout annotations
inline constexpr auto endianness = types_list(^^little, ^^big, ^^native, ^^bits); /// < list of endianness annotations

// Special annotations that cannot appear twice in a well_annotated structure
// NOTE: id annotation requires operator== on the entity annotated
inline constexpr auto global_unique = types_list(^^id, ^^length); /// < list of unique annotations

consteval auto no_duplicates(std::ranges::range auto const& elements) -> bool {
  auto it        = std::ranges::begin(elements);
  auto const end = std::ranges::end(elements);

  for (; it != end; ++it) {
    if (std::ranges::find(std::next(it), end, *it) != end) {
      return false;
    }
  }
  return true;
}

consteval auto verify_dimension_correctness(std::meta::info const type, std::ranges::range auto const& anns) -> bool {
  return true;
}

template<std::ranges::range R>
consteval auto verify_global_unique_dimension(std::meta::info const type, R const& annotations) -> bool {
  auto type_annotations = deep_annotation_types_of(type);
  auto only_once        = [&type_annotations](std::meta::info const annotation) {
    return std::ranges::count(type_annotations, annotation) <= 1;
  };
  return std::ranges::all_of(annotations, only_once);
}

consteval auto is_well_annotated(std::meta::info const type) -> bool {
  // No global unique duplicated in deep_annotations
  // No duplicated within type/member annotations
  return verify_global_unique_dimension(type, global_unique);
}

} // namespace rbe::detail
