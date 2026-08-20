/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file correctness.hpp
 * @version 1.0
 * @date 15/08/2026
 * @brief Short description
 *
 * Longer description
 */
#pragma once

#include <rbe/annotations/alignment.hpp>
#include <rbe/annotations/detail/utils.hpp>
#include <rbe/annotations/detail/view.hpp>
#include <rbe/annotations/endianness.hpp>
#include <rbe/annotations/metadata.hpp>
#include <rbe/core/detail/introspection.hpp>

// --- STD ---
#include <expected>
#include <functional>
#include <numeric>

namespace rbe::detail::annotations {


inline constexpr auto all = types_list(^^pack, ^^align, ^^little, ^^big, ^^native, ^^bits, ^^id, ^^length);

/// --- Annotations' dimensions ---

/// Two annotations from the same dimension cannot coexist within the same annotation range.
inline constexpr auto alignment  = types_list(^^pack, ^^align); /// < list of layout annotations
inline constexpr auto endianness = types_list(^^little, ^^big, ^^native, ^^bits); /// < list of endianness annotations

// Special annotations that cannot appear twice in a well_annotated structure
// NOTE: id annotation requires operator== on the entity annotated
inline constexpr auto global_unique = types_list(^^id, ^^length); /// < list of unique annotations

/**
 * @brief Verifies if a range of annotations satisfies the given dimension
 *
 * @param anns annotations range to verify
 * @param dim dimension
 * @return false if 2 or more annotations from the same dimension where found
 */
consteval auto satisfies_dimension(std::ranges::range auto const& anns, std::ranges::range auto const& dim) -> bool {
  auto match = [&dim](std::meta::info const annotation) {
    return std::ranges::find(dim, annotation) != std::ranges::end(dim);
  };
  std::size_t count = 0;
  for (auto annotation: anns) {
    count += match(annotation) ? 1UZ : 0UZ;
  }
  return count <= 1;
}

consteval auto verify_dimension_correctness(std::meta::info info, std::ranges::range auto const& dim) -> bool {
  if (not satisfies_dimension(annotation_types_of(info), dim)) {
    return false;
  }
  auto member_check = [&dim](std::meta::info const member) { return verify_dimension_correctness(member, dim); };
  return is_class_type(normalize_type(info)) ? std::ranges::all_of(nsdm(normalize_type(info)), member_check) : true;
}

// --- Global unique annotations ---

template<std::ranges::range R>
consteval auto verify_global_unique_dimension(std::meta::info const type, R const& annotations) -> bool {
  auto type_annotations = deep_annotation_types_of(type);
  auto is_unique        = [&type_annotations](std::meta::info const annotation) {
    return std::ranges::count(type_annotations, annotation) <= 1;
  };
  return std::ranges::all_of(annotations, is_unique);
}

// --- No duplicated annotations ---

consteval auto has_duplicates(std::ranges::range auto const& elements) -> bool {
  auto it        = std::ranges::begin(elements);
  auto const end = std::ranges::end(elements);

  for (; it != end; ++it) {
    if (std::ranges::find(std::next(it), end, *it) != end) {
      return true;
    }
  }
  return false;
}

consteval auto verify_no_local_duplications(std::meta::info info) -> bool {
  info = normalize_type(info);
  if (has_duplicates(annotation_types_of(info))) {
    return false;
  }

  return is_class_type(info) ? std::ranges::all_of(nsdm(info), verify_no_local_duplications) : true;
}

} // namespace rbe::detail::annotations
