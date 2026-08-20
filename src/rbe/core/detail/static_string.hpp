/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file ctp.hpp
 * @date 16/07/2026
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
#include <vector>

// --- System ---

namespace rbe {


namespace detail {

template<typename T, std::meta::info S>
inline constexpr auto string_object = [] { //
  return std::string_view(extract<char const*>(S), extent(type_of(S)) - 1);
}();

template<typename T, std::meta::info D, std::meta::info S>
inline constexpr auto string_view_object = [] { //
  return std::string_view(extract<char const*>(D), extract<std::size_t>(S));
}();

consteval auto define_static_string(std::string const& s) -> std::string_view const& {
  std::vector<std::meta::info> parts;

  parts.push_back(type_of(^^s));
  parts.push_back(reflect_constant(std::meta::reflect_constant_string(s)));

  auto r = object_of(substitute(^^string_object, parts));

  return extract<std::string_view const&>(r);
}

consteval auto define_static_string(std::string_view const s) -> std::string_view const& {
  std::vector<std::meta::info> parts;
  auto const str = std::string {s};

  parts.push_back(type_of(^^s));
  parts.push_back(reflect_constant(std::meta::reflect_constant_string(str)));

  auto const r = object_of(substitute(^^string_object, parts));

  return extract<std::string_view const&>(r);
}

} // namespace detail

/**
 * @brief string class that can be passed as constant template parameter (nttp)
 *
 * @note This class is inspired by the ctp library from this post:
 * https://brevzin.github.io/c++/2025/08/02/ctp-reflection
 *
 */
struct static_string {
  using target_type = std::string_view;

  target_type const& value;

  consteval static_string(std::string const& s) : value(detail::define_static_string(s)) { }

  consteval static_string(std::string_view const s) : value(detail::define_static_string(s)) { }

  consteval static_string(char const* s) : value(detail::define_static_string(std::string_view {s})) { }

  consteval operator target_type const&() const { return value; }

  consteval auto get() const -> target_type const& { return value; }

  consteval auto operator*() const -> target_type const& { return value; }

  consteval auto operator->() const -> target_type const* { return std::addressof(value); }
};

} // namespace rbe
