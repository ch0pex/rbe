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
#include <rbe/core/annotations.hpp>
#include <rbe/core/concepts.hpp>
#include <rbe/core/endian.hpp>
#include <rbe/detail/introspection.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <algorithm>
#include <cstddef>
#include <span>

// --- System ---


namespace rbe {

using member_offset = std::meta::member_offset;

struct member_layout {
  member_offset offset {};
  std::size_t size {};
  endian::order endianness {std::endian::little};

  constexpr bool operator==(member_layout const&) const = default;
};

struct struct_layout {
  std::size_t size {};
  // std::size_t size_bits {}; TODO: add support for bit fields
  std::span<member_layout const> members {};

  constexpr friend bool operator==(struct_layout const& lhs, struct_layout const& rhs) {
    return lhs.size == rhs.size and std::ranges::equal(lhs.members, rhs.members); //
  }
};

namespace detail {

consteval endian::order get_member_endianness(std::meta::info const member) {
  if (has_annotation(type_of(member), little)) {
    return endian::order::little;
  }
  else if (has_annotation(type_of(member), big)) {
    return endian::order::big;
  }
  else {
    return endian::order::native;
  }
}


} // namespace detail

// NOTE: at some point if compile times get really bad maybe we should consider caching the results
// of these functions in static constexpr variables. Referencing to static constexpr however is
// buggy in clang so returning by value is the only safe option for now.
// Related threads:
// - https://github.com/llvm/llvm-project/issues/82994
// - https://github.com/llvm/llvm-project/issues/61425

template<introspectable T>
consteval auto get_struct_layout() -> struct_layout {
  auto members = detail::nsdm(^^T);

  std::vector<member_layout> member_layouts;
  member_layouts.reserve(members.size());

  for (auto const& member: members) {
    member_layouts.emplace_back(offset_of(member), size_of(type_of(member)), endian::order::native);
  }
  return {
    .size    = sizeof(T),
    .members = std::define_static_array(member_layouts),
  };
}

template<introspectable T>
  requires(not detail::has_annotation(^^T, pack))
consteval auto get_wire_layout() -> struct_layout {
  auto members = detail::nsdm(^^T);

  std::vector<member_layout> member_layouts;
  member_layouts.reserve(members.size());

  for (auto const& member: members) {
    member_layouts.emplace_back(offset_of(member), size_of(type_of(member)), detail::get_member_endianness(member));
  }

  return {
    .size    = size_of(type_of(^^T)),
    .members = std::define_static_array(member_layouts),
  };
}

template<introspectable T>
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
    .members = std::define_static_array(member_layouts),
  };
}

/**
 * @brief Concept to check if a type is trivially serializable.
 *
 * A type is considered trivially serializable if its struct layout matches its wire layout.
 * Meaning that the in-memory representation of the type can be directly used for serialization without any additional
 * processing. Therefore, serialization and deserialization can be performed by simply memcpy'ing the data to and from a
 * buffer.
 *
 */
template<typename T>
concept trivially_serializable = //
    get_struct_layout<T>() == get_wire_layout<T>() //
    and std::is_trivially_copyable_v<T> //
    and introspectable<T>; //

} // namespace rbe
