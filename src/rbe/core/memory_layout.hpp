/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file memory_layout.hpp
 * @date 27/06/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/concepts/wirable.hpp>
#include <rbe/core/annotations.hpp>
#include <rbe/core/endian.hpp>
#include <rbe/core/static_array.hpp>
#include <rbe/detail/introspection.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <algorithm>
#include <cstddef>
#include <ranges>
#include <span>

// --- System ---


namespace rbe {

namespace detail {

consteval endian::order get_member_endianness(std::meta::info const member) {
  if (has_annotation(member, little)) {
    return endian::order::little;
  }
  if (has_annotation(member, big)) {
    return endian::order::big;
  }
  return endian::order::native;
}

} // namespace detail

using member_offset = std::meta::member_offset;

struct member_layout {
  member_offset offset {};
  std::size_t size {};
  endian::order endianness {std::endian::little};

  constexpr bool operator==(member_layout const&) const = default;
};

//  TODO: add support for bit fields
//  TODO: add support for nested structs
struct struct_layout {
  std::size_t size {};
  static_array<member_layout> members {};

  constexpr bool operator==(struct_layout const& /**/) const = default;
};

// NOTE: at some point if compile times get really bad maybe we should consider caching the results
// of these functions in static constexpr variables. Referencing to static constexpr however is
// buggy in clang so returning by value is the only safe option for now.
// Related threads:
// - https://github.com/llvm/llvm-project/issues/82994
// - https://github.com/llvm/llvm-project/issues/61425

template<wirable_class T>
consteval auto get_struct_layout() -> struct_layout {
  auto const members = detail::nsdm(^^T);

  std::vector<member_layout> member_layouts;
  member_layouts.reserve(members.size());

  for (auto const& member: members) {
    member_layouts.emplace_back(offset_of(member), size_of(type_of(member)), endian::order::native);
  }
  return {
    .size    = sizeof(T),
    .members = {std::from_range, member_layouts},
  };
}

template<wirable_class T>
  requires(not detail::has_annotation(^^T, pack))
consteval auto get_wire_layout() -> struct_layout {
  auto members = detail::nsdm(^^T);

  std::vector<member_layout> member_layouts;
  member_layouts.reserve(members.size());

  for (auto const& member: members) {
    member_layouts.emplace_back(offset_of(member), size_of(type_of(member)), detail::get_member_endianness(member));
  }

  return {
    .size    = size_of(^^T),
    .members = {std::from_range, member_layouts},
  };
}

template<wirable_class T>
  requires(detail::has_annotation(^^T, pack))
consteval auto get_wire_layout() -> struct_layout {
  auto members = detail::nsdm(^^T);

  std::vector<member_layout> member_layouts;
  member_layouts.reserve(members.size());

  // TODO: this implementation do not support bit fields
  member_offset current_offset {};
  for (auto const& member: members) {
    auto const member_size = size_of(type_of(member));
    member_layouts.emplace_back(current_offset, member_size, detail::get_member_endianness(member));
    current_offset.bytes += static_cast<std::ptrdiff_t>(member_size);
  }

  return {
    .size    = static_cast<std::size_t>(current_offset.bytes),
    .members = {std::from_range, member_layouts},
  };
}

} // namespace rbe
