/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file utils.hpp
 * @version 2.0
 * @date 15/08/2026
 * @brief Helpers for gathering and querying the RBE annotations attached to a type or member
 *
 * Three ranges, each one a `std::vector<annotation_info>` -- the annotations written on an entity,
 * the annotation range the requirements define for it, and that same range recursively. Everything
 * else is an ordinary range algorithm over one of them.
 */
#pragma once

// --- Includes ---
#include <rbe/annotations/detail/view.hpp>

// --- STD ---
#include <algorithm>
#include <optional>
#include <ranges>
#include <type_traits>
#include <vector>

namespace rbe::detail {

/**
 * The RBE annotations written directly on `entity` (a type or a non-static data member) -- derive<...>
 * lists already expanded, non-RBE attributes already filtered out.
 *
 * Not named `annotations_of`: ADL on `std::meta::info` would make the call ambiguous with
 * `std::meta::annotations_of`.
 */
consteval auto own_annotations(std::meta::info const entity) -> std::vector<annotation_info> {
  if (not is_nonstatic_data_member(entity) and not std::meta::is_type(entity)) {
    throw std::meta::exception("info is not a non-static data member or a type", ^^own_annotations);
  }
  return std::meta::annotations_of(entity) | views::annotations | std::ranges::to<std::vector>();
}

/**
 * @brief The REQ-058..061 "annotation range" for `entity`: for a member, its own annotations unioned
 * with its type's own annotations; for a type, just its own annotations.
 *
 * @param info entity to gather rbe annotations from
 * @return a vector with all the annotations of the entity
 */
consteval auto annotation_range(std::meta::info const info) -> std::vector<annotation_info> {
  auto result = own_annotations(info);
  if (is_nonstatic_data_member(info)) {
    result.append_range(own_annotations(type_of(info)));
  }
  return result;
}

/**
 * @brief annotation_range(entity), recursively unioned with every nested non-static data member's.
 *
 * Recursion is driven by the entity's *type*, so it descends through members of class type at any
 * depth -- a member is not a leaf, its type's members are visited too. Non-class types stop it.
 *
 * @param info reflection of the type (or member) to inspect recursively
 * @return a vector with all the annotations found within the type
 */
consteval auto deep_annotations(std::meta::info const info) -> std::vector<annotation_info> {
  std::vector<annotation_info> result = annotation_range(info);

  auto const type = normalize_type(info);
  if (not is_class_type(type)) {
    return result;
  }

  std::ranges::for_each(nsdm(type), [&](std::meta::info const member) {
    result.append_range(deep_annotations(member));
  });

  return result;
}

/**
 * @param annotations The range of annotations to search within.
 * @param needle The annotation, or `derive<...>` list of annotations, to find.
 *
 * @return true if every annotation the needle denotes is found in the range.
 */
consteval auto has_annotations(std::ranges::range auto const& annotations, auto const needle) -> bool
  requires annotation<std::remove_cvref_t<decltype(needle)>> or annotation_list<std::remove_cvref_t<decltype(needle)>>
{
  using needle_type = std::remove_cvref_t<decltype(needle)>;

  if constexpr (annotation_list<needle_type>) {
    return std::ranges::all_of(views::expand_annotation(^^needle_type), [&](annotation_info const one) {
      return std::ranges::contains(annotations, one);
    });
  }
  else {
    return std::ranges::any_of(annotations, [&](annotation_info const one) { return one.is(needle); });
  }
}

/**
 * Single entry point: `needle` may be a plain annotation OR a `derive<...>` list -- both are compared
 * against the same range, so there is exactly one code path.
 */
consteval auto has_annotations(std::meta::info const info, auto const needle) -> bool
  requires annotation<std::remove_cvref_t<decltype(needle)>> or annotation_list<std::remove_cvref_t<decltype(needle)>>
{
  return has_annotations(annotation_range(info), needle);
}

consteval auto has_annotations_deep(std::meta::info const info, auto const needle) -> bool
  requires annotation<std::remove_cvref_t<decltype(needle)>> or annotation_list<std::remove_cvref_t<decltype(needle)>>
{
  return has_annotations(deep_annotations(info), needle);
}

/**
 * @brief The annotation built from `tag` that applies to `entity`, if any.
 *
 * The reverse direction of `has_annotations`: instead of asking whether a known annotation is there,
 * it hands back the one that is, so its value can be read (`rbe::id(value)` -> the id a message type
 * is dispatched under).
 */
consteval auto find_annotation(std::meta::info const entity, std::meta::info const tag)
    -> std::optional<annotation_info> {
  auto const range = annotation_range(entity);
  auto const it    = std::ranges::find_if(range, [tag](annotation_info const one) { return one.tag() == tag; });
  return it == std::ranges::end(range) ? std::nullopt : std::optional {*it};
}

/// `find_annotation`, searching the whole (deep) type instead of just the entity's own range.
consteval auto find_annotation_deep(std::meta::info const entity, std::meta::info const tag)
    -> std::optional<annotation_info> {
  auto const range = deep_annotations(entity);
  auto const it    = std::ranges::find_if(range, [tag](annotation_info const one) { return one.tag() == tag; });
  return it == std::ranges::end(range) ? std::nullopt : std::optional {*it};
}

/**
 * Searches entity's REQ-058..061 annotation range (its own annotations, unioned with its type's own
 * annotations for a member) for the first annotation carrying a `T`.
 */
template<typename T>
consteval auto resolve_in_scope(std::meta::info const entity) -> std::optional<T> {
  for (auto const one: annotation_range(entity)) {
    if (auto const value = one.value<T>()) {
      return value;
    }
  }
  return std::nullopt;
}

} // namespace rbe::detail
