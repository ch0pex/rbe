/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file introspection.hpp
 * @date 24/06/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <meta>
#include <ranges>

// --- System ---

namespace rbe::detail {

inline constexpr auto default_context = std::meta::access_context::unchecked();

consteval auto nsdm(std::meta::info info, std::meta::access_context ctx = default_context) {
  return nonstatic_data_members_of(info, ctx);
}

consteval auto nsdm( //
  std::meta::info info,  //
  std::string_view const identifier,  //
  std::meta::access_context ctx = default_context  //
) {
  for (auto [idx, field]: nsdm(info, ctx) | std::views::enumerate) {
    if (has_identifier(field) and identifier_of(field) == identifier)
      return field;
  }

  // Reflecting overload sets is not supported yet, as a work arround to throw
  static constexpr auto nsdm_by_id = []() { };
  throw std::meta::exception("invalid member identifier, no such nonstatic data member", ^^nsdm_by_id);
}

consteval auto nsdm(
    std::meta::info const info, //
    std::size_t const index, //
    std::meta::access_context ctx = default_context //
) {
  auto members = nsdm(info, ctx);
  if (index < members.size()) {
    return members[index];
  }

  static constexpr auto nsdm_by_index = []() { };
  throw std::meta::exception("invalid member index", ^^nsdm_by_index);
}

consteval std::size_t nsdm_index( //
  std::meta::info const info,  //
  std::string_view const identifier, //
  std::meta::access_context ctx = default_context //
) {
  for (auto [idx, field]: nsdm(info, ctx) | std::views::enumerate) {
    if (has_identifier(field) and identifier_of(field) == identifier)
      return static_cast<std::size_t>(idx);
  }
  throw std::meta::exception("invalid member identifier, no such nonstatic data member", ^^nsdm_index);
}

} // namespace rbe::detail
