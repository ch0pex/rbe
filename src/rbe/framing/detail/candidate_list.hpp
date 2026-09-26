/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file candidate_list.hpp
 * @date 05/08/2026
 * @brief Type list and variant/tuple aggregation of the wirable types an id can select
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/id.hpp>
#include <rbe/core/detail/context.hpp>
#include <rbe/core/detail/introspection.hpp>
#include <rbe/core/detail/throw_check.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/dsrl/proxy.hpp>

// --- STD ---
#include <algorithm>
#include <array>
#include <limits>
#include <meta>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <version>

#if __cpp_lib_constexpr_unordered_set >= 202502L
#include <unordered_set>
#endif

// --- System ---

namespace rbe::detail {

/// NOTE: if unique_set constepxr is available it's used, otherwise a vector is
/// used and the search is linear.
/// If the id is not hashable, the vector is used even if unique_set is available.

template<typename Id>
concept hashable = requires(Id const& id) { std::hash<Id> {}(id); };

template<typename Id>
consteval auto insert_unique(std::vector<Id>& seen, Id const& id) -> bool {
  if (std::ranges::contains(seen, id)) {
    return false;
  }
  seen.push_back(id);
  return true;
}

#if __cpp_lib_constexpr_unordered_set >= 202502L
template<typename Id>
consteval auto insert_unique(std::unordered_set<Id>& seen, Id const& id) -> bool {
  return seen.insert(id).second;
}

template<typename Id>
using seen_ids = std::conditional_t<hashable<Id>, std::unordered_set<Id>, std::vector<Id>>;
#else
template<typename Id>
using seen_ids = std::vector<Id>;
#endif

/**
 * @brief Verifies that these types can form a candidate list: all identifiable, all agreeing on the
 * id type, and no id declared twice.
 *
 * @throws std::invalid_argument naming the candidate at fault -- this is what a rejected list reports
 */
template<typename IdType>
consteval auto diagnose_compatible_candidates(std::span<std::meta::info const> types) -> void {
  if (types.size() < 2) {
    throw std::invalid_argument("candidate list must have at least 2 candidates");
  }

  for (auto const candidate: types) {
    if (id_type_of(candidate) == std::meta::info {}) {
      throw std::invalid_argument(
          "all candidates must be identifiable types, but '" + std::string(display_string_of(candidate)) +
          "' declares no id"
      );
    }

    if (id_type_of(candidate) != id_type_of(types[0])) {
      throw std::invalid_argument(
          "all candidates must have the same id type, but '" + std::string(display_string_of(candidate)) +
          "' has a different id type than the first candidate '" + std::string(display_string_of(types[0])) + "'"
      );
    }
  }

  seen_ids<IdType> ids;
  for (auto const candidate: types) {
    if (not insert_unique(ids, id_of<IdType>(candidate))) {
      throw std::invalid_argument(
          "all candidates must have unique ids, but '" + std::string(display_string_of(candidate)) +
          "' has the same id as another candidate"
      );
    }
  }
}

template<typename... T>
concept compatible_candidates = no_throw(diagnose_compatible_candidates<rbe::id_type_of<T...[0]>>, std::array {^^T...});

template<typename T, typename List>
concept belongs_to = [] {
  auto id = rbe::id_of<T>();
  return std::ranges::find(List::ids, id) != std::ranges::end(List::ids);
}();


/**
 * @brief The position a candidate occupies in a candidate_list, or `none` if the list has no such candidate.
 *
 * A type of its own rather than a bare index: ids are very often integers themselves, so on any interface
 * that takes both -- notably any's index constructor, which exists precisely to skip the id lookup -- an
 * index and an id would silently convert into one another.
 */
enum class candidate_index : std::size_t {
  none = std::numeric_limits<std::size_t>::max(), ///< what index_of() reports for an id no candidate declares
};

/**
 * @brief A list of wirable types, each declaring the id it answers to.
 *
 * The primary template is the error path: it carries no members, and its `consteval` block re-runs
 * the diagnosis so instantiating a bad list reports *why*
 */
template<typename... T>
struct candidate_list {
  consteval { diagnose_compatible_candidates<rbe::id_type_of<T...[0]>>(std::array {^^T...}); }
};

template<identifiable... T>
  requires(compatible_candidates<T...>)
struct candidate_list<T...> {
  using id_type = rbe::id_type_of<T...[0]>;

  // The type and canonical id of every candidate
  static constexpr auto types = std::array {^^T...};
  static constexpr auto ids   = std::array {rbe::id_of<T>()...};

  // NOTE: for now rbe only supports fixed-size wirables,
  // so we can store their sizes in a constexpr array
  // However keep in mind that in the future this will change
  static constexpr auto wire_size = std::array {rbe::wire_size_of<T>()...};
  static constexpr auto count     = sizeof...(T);

  // NOTE: For now I believe linear search is fine, this allow us to
  // tell the user to specify first in the list the most common candidates
  // Usually market protocols have a few messages that are much more common
  // than the rest, so this is a reasonable assumption
  static constexpr auto index_of(id_type const id) -> candidate_index {
    auto const it = std::ranges::find(ids, id);
    return it != std::ranges::end(ids) //
               ? static_cast<candidate_index>(std::ranges::distance(std::ranges::begin(ids), it)) //
               : candidate_index::none; //
  }

  template<belongs_to<candidate_list> U>
  static consteval auto index_of() -> candidate_index {
    return index_of(rbe::id_of<U>());
  }

  /// Whether `index` selects a candidate of this list, i.e. it came from the id of one of them.
  static constexpr auto contains(candidate_index const index) -> bool { return std::to_underlying(index) < count; }
};

} // namespace rbe::detail
