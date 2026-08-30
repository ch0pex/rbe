/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file correctness.hpp
 * @version 1.0
 * @date 15/08/2026
 * @brief Annotation dimension definitions and correctness checks used by well_annotated
 */
#pragma once

#include <rbe/annotations/detail/dimension.hpp>
#include <rbe/annotations/detail/utils.hpp>
#include <rbe/core/detail/introspection.hpp>

// --- STD ---
#include <ranges>

namespace rbe::detail::annotations {

/**
 * @brief Verifies that at most one annotation belonging to `dim` appears in a single annotation range.
 *
 * @param anns annotations range to verify
 * @param dim dimension
 * @return false if 2 or more annotations from the same dimension where found
 */
consteval auto satisfies_dimension(std::ranges::range auto const& anns, std::meta::info const dim) -> bool {
  return std::ranges::count_if(anns, by_dimension(dim)) <= 1;
}

consteval auto verify_dimension_correctness(std::meta::info info, std::meta::info const dim) -> bool {
  if (not satisfies_dimension(annotation_range(info), dim)) {
    return false;
  }
  auto member_check = [dim](std::meta::info const member) { return verify_dimension_correctness(member, dim); };
  return is_class_type(normalize_type(info)) ? std::ranges::all_of(nsdm(normalize_type(info)), member_check) : true;
}

// --- Global unique annotations ---

consteval auto verify_global_unique_dimension(std::meta::info const type, std::meta::info const dim) -> bool {
  auto const in_dim = deep_annotations(type) | std::views::filter(by_dimension(dim)) | std::ranges::to<std::vector>();
  return std::ranges::all_of(in_dim, [&in_dim](std::meta::info const a) { return std::ranges::count(in_dim, a) <= 1; });
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
  if (has_duplicates(annotation_range(info))) {
    return false;
  }

  return is_class_type(info) ? std::ranges::all_of(nsdm(info), verify_no_local_duplications) : true;
}

// --- Generic dispatch: discover which dimensions are actually used, verify each per its kind ---

consteval auto dimensions_used_in(std::meta::info const type) -> std::vector<std::meta::info> {
  std::vector<std::meta::info> dims;
  for (auto const ann: deep_annotations(type)) {
    if (auto const dim = dimension_of(normalize_type(ann));
        dim != std::meta::info {} and not std::ranges::contains(dims, dim)) {
      dims.push_back(dim);
    }
  }
  return dims;
}

consteval auto verify_dimension(std::meta::info const type, std::meta::info const dim) -> bool {
  switch (kind_of(dim)) { // clang-format off
    case dimension_kind::exclusive: return verify_dimension_correctness(type, dim);
    case dimension_kind::unique:    return verify_global_unique_dimension(type, dim);
  } // clang-format on
  throw std::meta::exception("unhandled dimension_kind", dim);
}

// --- Local constraints : verify that every annotation found in `type` satisfies its own correctness rule

consteval auto verify_check(std::meta::info const value, std::meta::info const entity) -> bool {
  auto const traits = traits_of(normalize_type(value));
  if (not traits) {
    throw std::meta::exception("annotation is not marked with annotation_traits<T>", ^^verify_check);
  }

  auto static_member_fns = static_member_functions_of(*traits);
  auto const check_fn    = std::ranges::find(static_member_fns, std::string {"check"}, std::meta::identifier_of);
  if (check_fn == std::ranges::end(static_member_fns)) {
    return true;
  }

  using check_fn_t = bool (*)(std::meta::info const, std::meta::info const);
  return std::meta::extract<check_fn_t>(*check_fn)(value, entity);
}

consteval auto verify_local_constraints(std::meta::info info) -> bool {
  auto check = std::ranges::all_of(annotation_range(info), [info](std::meta::info value) { //
    return verify_check(value, info);
  });

  info = normalize_type(info);
  return check and (is_class_type(info) ? std::ranges::all_of(nsdm(info), verify_local_constraints) : true);
}

/**
 * @brief Verifies that `type` is annotated correctly: no local duplicates, and every dimension found
 * among its (possibly nested) annotations satisfies its own correctness rule.
 *
 * Adding a brand new dimension anywhere in the codebase requires zero edits here -- it is discovered
 * dynamically from the annotations actually attached to `type`.
 */
consteval auto well_annotated(std::meta::info const type) -> bool {
  auto const dimension_check = [type](std::meta::info const dim) { return verify_dimension(type, dim); };

  return verify_no_local_duplications(type) //
         and verify_local_constraints(type) //
         and std::ranges::all_of(dimensions_used_in(type), dimension_check);
}

} // namespace rbe::detail::annotations
