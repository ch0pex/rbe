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

struct member_layout {
  std::ptrdiff_t offset {};
  std::size_t size {};
  std::endian endiannes {std::endian::little};

  constexpr bool operator==(member_layout const&) const = default;
};

struct struct_layout {
  bool packing {false};
  std::span<member_layout const> members {};

  constexpr friend bool operator==(struct_layout const& lhs, struct_layout const& rhs) {
    return lhs.packing == rhs.packing and std::ranges::equal(lhs.members, rhs.members); //
  }
};

template<introspectable T>
consteval auto get_layout() -> struct_layout {
  auto members = detail::nsdm(^^T);

  std::vector<member_layout> member_layouts;
  member_layouts.reserve(members.size());

  for (auto const& member: members) {
    member_layouts.emplace_back(
        offset_of(member).bytes, // TODO: look for packing annotation, cummulative offsets, have bits version
        size_of(type_of(member)), // TODO: custom size tag
        endian::order::little // TODO: endiannes tag
    );
  }
  return {
    .packing = detail::has_annotation(^^T, pack), //
    .members = std::define_static_array(member_layouts),
  };
}

} // namespace rbe
