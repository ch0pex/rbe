/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file utils.hpp
 * @version 1.0
 * @date 15/08/2026
 * @brief Helpers for gathering and querying the RBE annotations attached to a type or member
 */
#pragma once

#include <rbe/annotations/detail/dimension.hpp>
#include <rbe/annotations/detail/view.hpp>

namespace rbe::detail {

/// NOTE: most annotations are variables with anonymous types, however those annotations that require arguments are
/// types so we need to normalize them into a list of types to define dimensions
consteval auto types_list(auto... args) {
  return std::array<std::meta::info, sizeof...(args)> {normalize_type(args)...};
}

/// The RBE annotations written directly on `entity` (a type or a non-static data member) -- derive<...>
/// lists already expanded, non-RBE attributes already filtered out.
consteval auto rbe_annotations(std::meta::info const entity) -> std::vector<std::meta::info> {
  std::vector<std::meta::info> raw_annotations;
  if (is_nonstatic_data_member(entity) or std::meta::is_type(entity)) {
    raw_annotations = std::meta::annotations_of(entity);
  }
  else {
    throw std::meta::exception("info is not a non-static data member or a type", ^^rbe_annotations);
  }
  return raw_annotations | views::rbe_annotations | std::ranges::to<std::vector>();
}

/**
 * @brief The REQ-058..061 "annotation range" for `entity`: for a member, its own annotations unioned
 * with its type's own annotations; for a type, just its own annotations.
 *
 * @param info entity to gather rbe annotations from
 * @return a vector with all the annotations of the entity
 */
consteval auto annotation_range(std::meta::info const info) -> std::vector<std::meta::info> {
  auto result = rbe_annotations(info);
  if (is_nonstatic_data_member(info)) {
    result.append_range(rbe_annotations(type_of(info)));
  }
  return result;
}

/**
 * @brief annotation_range(entity), recursively unioned with every nested non-static data member's.
 *
 * @param info reflection of the type to inspect recursively
 * @return a vector with all the annotations found within the type
 */
consteval auto deep_annotations(std::meta::info const info) -> std::vector<std::meta::info> {
  std::vector<std::meta::info> result = annotation_range(info);
  if ((is_type(info) and not is_class_type(info)) or is_nonstatic_data_member(info)) {
    return result;
  }

  std::ranges::for_each(nsdm(info), [&](std::meta::info const member) {
    result.append_range(deep_annotations(member));
  });

  return result;
}

/// Single entry point: `value` may be a plain annotation OR a `derive<...>` list -- both normalize
/// through the same `views::rbe_annotations` range, so there is exactly one code path.
consteval auto has_annotation(std::meta::info const info, auto value) -> bool
  requires annotation<decltype(value)> or annotation_list<decltype(value)>
{
  auto const haystack = annotation_range(info);
  return std::ranges::all_of(views::rbe_annotations(value), [&](std::meta::info const needle) {
    return std::ranges::contains(haystack, needle);
  });
}

/// Flattens a single raw attribute into the RBE annotation VALUES it denotes (a `derive<...>` list
/// expands into its filtered constituents, a plain annotation is itself, anything else vanishes).
/// Unlike `views::rbe_annotations`/`annotation_range`, this does NOT normalize to types -- it must
/// preserve the actual values so `value_of` can `extract<T>` them.
consteval auto rbe_annotation_values(std::meta::info const raw) -> std::vector<std::meta::info> {
  if (is_annotation_list(raw)) {
    return template_arguments_of(type_of(raw)) | std::views::filter(is_rbe_annotation) | std::ranges::to<std::vector>();
  }
  return is_rbe_annotation(raw) ? std::vector {raw} : std::vector<std::meta::info> {};
}

/// Same annotation range as `annotation_range` (member's own, unioned with its type's own), but
/// preserving values instead of normalizing to types -- the range `resolve_in_scope` searches.
consteval auto annotation_values(std::meta::info const entity) -> std::vector<std::meta::info> {
  std::vector<std::meta::info> result;
  for (auto const a: std::meta::annotations_of(entity)) {
    result.append_range(rbe_annotation_values(a));
  }
  if (is_nonstatic_data_member(entity)) {
    for (auto const a: std::meta::annotations_of(type_of(entity))) {
      result.append_range(rbe_annotation_values(a));
    }
  }
  return result;
}

/// Searches entity's REQ-058..061 annotation range (its own annotations, unioned with its type's own
/// annotations for a member) for the first annotation that yields a `T`.
template<typename T>
consteval auto resolve_in_scope(std::meta::info const entity) -> std::optional<T> {
  for (auto const a: annotation_values(entity)) {
    if (auto v = value_of<T>(a)) {
      return v;
    }
  }
  return std::nullopt;
}

/// Replaces the endianness-specific `endiannes_from_annotation`/`has_endianness_annotation`/
/// `get_member_endianness` chain with one function, generically, for ANY value-bearing dimension.
template<typename T>
consteval auto resolve(std::meta::info const parent, std::meta::info const member, std::meta::info const dim) -> T {
  if (auto v = resolve_in_scope<T>(member)) {
    return *v;
  }
  if (auto v = resolve_in_scope<T>(parent)) {
    return *v;
  }
  return default_value_of<T>(dim);
}

} // namespace rbe::detail
