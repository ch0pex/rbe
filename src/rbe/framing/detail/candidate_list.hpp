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
#include <array>
#include <meta>
#include <stdexcept>
#include <string>

// --- System ---

namespace rbe::detail {

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
}

template<typename... T>
concept compatible_candidates = no_throw(diagnose_compatible_candidates, std::array {^^T...});

/**
 * @brief A list of wirable types, each declaring the id it answers to.
 *
 * The primary template is the error path: it carries no members, and its `consteval` block re-runs
 * the diagnosis so instantiating a bad list reports *why*
 */
template<typename... T>
struct candidate_list {
  consteval { diagnose_compatible_candidates(std::array {^^T...}); }
};

template<identifiable... T>
  requires(compatible_candidates<T...>)
struct candidate_list<T...> {
  using variant_type       = std::variant<T...>;
  using proxy_variant_type = std::variant<dsrl::proxy<T>...>;
  using id_type            = rbe::id_type_of<T...[0]>;

  // The type and canonical id of every candidate
  static constexpr auto types = std::array {^^T...};
  static constexpr auto ids   = std::array {rbe::id_of<T>()...};

  // NOTE: for now Rbe only supports fixed-size wirables,
  // so we can store their sizes in a constexpr array
  // However keep in mind that in the future this will change
  static constexpr auto wire_size = std::array {rbe::wire_size_of<T>()...};
  static constexpr auto count     = sizeof...(T);

  // NOTE: in the future we can optimize this bc we could sort those
  // ids that are comparable and use a binary search instead of a linear
  static constexpr auto index_of(id_type const id) -> std::size_t {
    auto const it = std::ranges::find(ids, id);
    return it != std::ranges::end(ids) //
               ? std::ranges::distance(std::ranges::begin(ids), it) //
               : std::numeric_limits<std::size_t>::max(); //
  }
};

template<typename T, typename List>
concept belongs_to = [] {
  auto id = rbe::id_of<T>();
  return std::ranges::find(List::ids, id) != std::ranges::end(List::ids);
}();

} // namespace rbe::detail
