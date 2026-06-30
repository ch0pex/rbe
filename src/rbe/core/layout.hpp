/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file layout.hpp
 * @date 27/06/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/core/annotations.hpp>
#include <rbe/detail/introspection.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <algorithm>
#include <cstddef>
#include <span>
#include <type_traits>
#include <vector>

// --- System ---


namespace rbe {

// NOTE: For now lets just support aggregate types, we can expand this latter to support more complex types
template<typename T>
concept introspectable = std::meta::is_enumerable_type(^^T);

// NOTE: this type contains
struct member_layout {
  std::ptrdiff_t offset {};
  std::size_t size {};
  std::endian endiannes {std::endian::little};

  constexpr bool operator==(member_layout const&) const = default;
};

struct struct_layout {
  bool packing {true};
  std::span<member_layout const> members {};

  constexpr friend bool operator==(struct_layout const& lhs, struct_layout const& rhs) {
    return lhs.packing == rhs.packing and std::ranges::equal(lhs.members, rhs.members); //
  }
};

template<introspectable T>
consteval auto get_layout() -> struct_layout {
  auto members = detail::nsdm(^^T);
  struct_layout layout {};

  std::vector<member_layout> member_layouts;
  member_layouts.reserve(members.size());

  for (auto const& member: members) {
    member_layouts.emplace_back(
        offset_of(member).bytes, //
        size_of(type_of(member)), //
        endian::order::little // TODO: endiannes tag
    );
  }
  layout.members = std::define_static_array(member_layouts);
  return layout;
}

} // namespace rbe
