/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file introspection.hpp
 * @date 24/06/2026
 * @brief Reflection helpers wrapping std::meta for non-static data member access
 */

#pragma once

// --- Includes ---

// --- STD ---
#include <algorithm>
#include <meta>
#include <ranges>

// --- System ---

namespace rbe::detail {

inline constexpr auto default_context = std::meta::access_context::unchecked();

consteval auto normalize_type(std::meta::info const info) -> std::meta::info {
  return not is_type(info) ? remove_cvref(type_of(info)) : remove_cvref(info);
};

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
  static constexpr auto nsdm_by_id = [] { };
  throw std::meta::exception("invalid member identifier, no such nonstatic data member", ^^nsdm_by_id);
}

consteval auto nsdm(
    std::meta::info const info, //
    std::size_t const index, //
    std::meta::access_context ctx = default_context //
) {
  if (auto const members = nsdm(info, ctx); index < members.size()) {
    return members[index];
  }

  static constexpr auto nsdm_by_index = [] { };
  throw std::meta::exception("invalid member index", ^^nsdm_by_index);
}

consteval auto nsdm_index( //
  std::meta::info const info,  //
  std::string_view const identifier, //
  std::meta::access_context ctx = default_context //
) -> std::size_t {
  for (auto [idx, field]: nsdm(info, ctx) | std::views::enumerate) {
    if (has_identifier(field) and identifier_of(field) == identifier)
      return static_cast<std::size_t>(idx);
  }
  static constexpr auto nsdm_index = [] { };
  throw std::meta::exception("invalid member, no such nonstatic data member", ^^nsdm_index);
}

consteval auto nsdm_index( //
  std::meta::info const info,  //
  std::meta::info const member,
  std::meta::access_context ctx = default_context //
) -> std::size_t {
  return nsdm_index(info, identifier_of(member), ctx);
}

consteval std::size_t nsdm_count(std::meta::info const info, std::meta::access_context ctx = default_context) {
  return nsdm(info, ctx).size();
}

consteval bool specialization_of(std::meta::info const info, std::meta::info const template_info) {
  if (not has_template_arguments(info)) {
    return false;
  }

  return template_of(info) == template_info;
}

consteval auto bases_of(std::meta::info info, std::meta::access_context ctx = default_context) {
  return std::meta::bases_of(info, ctx);
}

consteval auto static_member_functions_of(std::meta::info const info, std::meta::access_context ctx = default_context) {
  static constexpr auto is_static_member_function = [](std::meta::info const member) -> bool {
    return is_static_member(member) and is_function(member) and not is_special_member_function(member);
  };
  return members_of(info, ctx) | std::views::filter(is_static_member_function) | std::ranges::to<std::vector>();
}

consteval auto member_aliases_of(std::meta::info const info, std::meta::access_context ctx = default_context)
    -> std::vector<std::meta::info> {
  return members_of(info, ctx) //
         | std::views::filter(std::meta::is_type_alias) //
         | std::ranges::to<std::vector>();
}

consteval auto member_alias_of( //
    std::meta::info const info,  //
    std::string_view const identifier,  //
    std::meta::access_context ctx = default_context //
) -> std::meta::info {
  auto const aliases = member_aliases_of(info, ctx);
  auto it            = std::ranges::find_if(aliases, [&](std::meta::info const alias) {
    return has_identifier(alias) and identifier_of(alias) == identifier;
  });

  if (it != aliases.end()) {
    return *it;
  }

  throw std::meta::exception("invalid member alias, no such member alias", ^^member_alias_of);
}


} // namespace rbe::detail
