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
#include <cstddef>
#include <ranges>

// --- System ---


namespace rbe::detail {


using member_offset = std::meta::member_offset;

struct member_layout {
  member_offset offset {};
  std::size_t size {};
  endian::order endianness {std::endian::little};
  bool is_array {false};

  constexpr bool operator==(member_layout const&) const = default;
};

//  TODO: add support for bit fields
//  TODO: add support for nested structs
struct struct_layout {
  std::size_t size {};
  static_array<member_layout> members {};

  constexpr bool operator==(struct_layout const& /**/) const = default;
};

struct Child {
  int child1;
  int child2;
  int child3;
};

struct[[= rbe::big]] Child2 {
  Child child1;
  int child2;
  [[= rbe::little]] int child3;
};

struct[[= rbe::pack]] Parent {
  [[= rbe::little]] Child2 child1;
  int child2;
  int child3;
};

// When it comes to support struct_layouts that are nested this are the following requirements:
// - All childs must inherit parent annotations as default values
// - Member annotations can overwrite type annotations:
//    - Member annotation > parent type annotations
//    - Member annotation over class types overwrite default class annotations but not member annotations


// recursively aggregates the size of the member variables
consteval auto wire_size_of(std::meta::info const info) -> std::size_t {
  if (not has_pack_annotation(info)) {
    return size_of(info);
  }

  std::size_t result = 0;
  for (auto const member: detail::nsdm(info)) {
    result += wire_size_of(type_of(member));
  }
  return result;
}

consteval auto normalize_info_type(std::meta::info const info) { return not is_type(info) ? type_of(info) : info; }

consteval auto get_members_layout(std::meta::info const info) -> std::vector<member_layout> {
  auto const type = normalize_info_type(info);
  if (is_trivially_wirable_primitive(remove_all_extents(type))) { // leaf case
    return {
      member_layout {
        .offset     = offset_of(info),
        .size       = size_of(type),
        .endianness = endian::order::native,
      },
    };
  }
  if (is_custom_wirable(type)) {
    throw std::meta::exception("Custom types are not fully implemented yet", ^^get_members_layout);
    // TODO:
    // Some considerations to support custom types:
    // - it should manually define its member layout for wire layout
    // - for structure layout we might be able to use actual custom structure layout
    return {};
  }

  std::vector<member_layout> member_layouts;

  for (auto const members = nsdm(type); auto const& member: members) {
    member_layouts.insert_range(std::ranges::end(member_layouts), get_members_layout(member));
  }

  return member_layouts;
}


consteval auto get_struct_layout(std::meta::info const info) -> struct_layout {
  return {
    .size    = size_of(info),
    .members = {std::from_range, get_members_layout(info)},
  };
}

consteval auto get_wire_layout_native(std::meta::info const info) -> struct_layout {
  auto members = detail::nsdm(info);

  std::vector<member_layout> member_layouts;

  for (auto const m: members) {
    member_layouts.emplace_back(offset_of(m), wire_size_of(type_of(m)), detail::get_member_endianness(info, m));
  }

  return {
    .size    = wire_size_of(info),
    .members = {std::from_range, member_layouts},
  };
}

consteval auto get_wire_layout_packed(std::meta::info const info) -> struct_layout {
  auto members = detail::nsdm(info);

  std::vector<member_layout> member_layouts;
  member_layouts.reserve(members.size());

  // TODO: this implementation do not support bit fields
  member_offset current_offset {};
  for (auto const& member: members) {
    auto const member_size = wire_size_of(type_of(member));
    member_layouts.emplace_back(current_offset, member_size, detail::get_member_endianness(info, member));
    current_offset.bytes += static_cast<std::ptrdiff_t>(member_size);
  }

  return {
    .size    = static_cast<std::size_t>(current_offset.bytes),
    .members = {std::from_range, member_layouts},
  };
}

consteval auto get_wire_layout(std::meta::info const info) -> struct_layout {
  if (detail::has_annotation(info, pack)) {
    return get_wire_layout_packed(info);
  }
  return get_wire_layout_native(info);
}


// NOTE: at some point if compile times get really bad maybe we should consider caching the results
// of these functions in static constexpr variables. Referencing to static constexpr however is
// buggy in clang so returning by value is the only safe option for now.
// Related threads:
// - https://github.com/llvm/llvm-project/issues/82994
// - https://github.com/llvm/llvm-project/issues/61425

template<wirable_class T>
consteval auto get_struct_layout() -> struct_layout {
  return get_struct_layout(^^T);
}

template<wirable_class T>
consteval auto get_wire_layout() -> struct_layout {
  return get_wire_layout(^^T);
}

} // namespace rbe::detail
