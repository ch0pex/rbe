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
#include <rbe/detail/annotations_correctness.hpp>
#include <rbe/detail/introspection.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <cstddef>
#include <ranges>

// --- System ---


namespace rbe {

namespace detail {

consteval auto endianness_from_annotation(std::meta::info const info) -> endian::order {
  if (has_annotation(info, little)) {
    return endian::order::little;
  }
  if (has_annotation(info, big)) {
    return endian::order::big;
  }
  return not is_type(info) ? endianness_from_annotation(type_of(info)) : endian::order::native;
}

consteval auto has_endianness_annotation(std::meta::info const info) -> bool {
  if (has_annotation(info, little) or has_annotation(info, big))
    return true;
  return not is_type(info) ? has_endianness_annotation(type_of(info)) : false;
}

consteval auto has_pack_annotation(std::meta::info const info) -> bool {
  if (has_annotation(info, pack))
    return true;
  return not is_type(info) ? has_annotation(type_of(info), pack) : false;
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
struct struct_layout {
  std::size_t size {};
  static_array<member_layout> members {};

  constexpr bool operator==(struct_layout const& /**/) const = default;
};

struct context {
  consteval explicit context(std::meta::info info) :
    endianness(
        detail::has_endianness_annotation(info) //
            ? detail::endianness_from_annotation(info)
            : endian::order::native
    ),
    alignment(detail::has_pack_annotation(info) ? 1 : alignment_of(info)) { }

  endian::order endianness;
  std::size_t alignment;
};

// recursively aggregates the size of the member variables
consteval auto wire_size_of(std::meta::info const info, context const ctx) -> std::size_t {
  if ((not detail::has_pack_annotation(info) and ctx.alignment == alignment_of(info)) or
      is_trivially_wirable_primitive(info)) {
    return size_of(info);
  }

  std::size_t result = 0;
  for (auto const member: detail::nsdm(info)) {
    result += wire_size_of(type_of(member), ctx);
  }
  return result;
}

consteval auto wire_size_of(std::meta::info const info) -> std::size_t { return wire_size_of(info, context {info}); }

// NOTE: at some point if compile times get really bad maybe we should consider caching the results
// of these functions in static constexpr variables. Referencing to static constexpr however is
// buggy in clang so returning by value is the only safe option for now.
// Related threads:
// - https://github.com/llvm/llvm-project/issues/82994
// - https://github.com/llvm/llvm-project/issues/61425

consteval auto get_struct_layout(std::meta::info const info) -> struct_layout {
  auto const members = detail::nsdm(info);

  std::vector<member_layout> member_layouts;

  for (auto const& member: members) {
    member_layouts.emplace_back(
        member_layout {
          .offset     = offset_of(member),
          .size       = size_of(type_of(member)),
          .endianness = endian::order::native,
        }
    );
  }
  return {
    .size    = size_of(info),
    .members = {std::from_range, member_layouts},
  };
}

consteval auto get_wire_layout_padded(std::meta::info const info, context const ctx) -> struct_layout {
  std::vector<member_layout> member_layouts;

  for (auto const m: detail::nsdm(info)) {
    member_layouts.emplace_back(
        member_layout {
          .offset     = offset_of(m),
          .size       = wire_size_of(type_of(m), ctx),
          .endianness = detail::has_endianness_annotation(m) ? detail::endianness_from_annotation(m) : ctx.endianness,
        }
    );
  }

  return {
    .size    = wire_size_of(info, ctx),
    .members = {std::from_range, member_layouts},
  };
}

consteval auto get_wire_layout_packed(std::meta::info const info, context ctx) -> struct_layout {
  auto members = detail::nsdm(info);

  std::vector<member_layout> member_layouts;
  member_layouts.reserve(members.size());

  // TODO: this implementation do not support bit fields
  member_offset current_offset {};
  for (auto const& member: members) {
    auto const member_size = wire_size_of(type_of(member), ctx);
    member_layouts.emplace_back(
        member_layout {
          .offset = current_offset,
          .size   = member_size,
          .endianness =
              detail::has_endianness_annotation(member) ? detail::endianness_from_annotation(member) : ctx.endianness,
        }
    );
    current_offset.bytes += static_cast<std::ptrdiff_t>(member_size);
  }

  return {
    .size    = static_cast<std::size_t>(current_offset.bytes),
    .members = {std::from_range, member_layouts},
  };
}

consteval auto get_wire_layout(std::meta::info info, context ctx) -> struct_layout {
  if (detail::has_endianness_annotation(info)) {
    ctx.endianness = detail::endianness_from_annotation(info);
  }
  if (detail::has_annotation(info, pack)) {
    return get_wire_layout_packed(info, ctx);
  }
  return get_wire_layout_padded(info, ctx);
}

consteval auto get_wire_layout(std::meta::info const info) -> struct_layout {
  return get_wire_layout(info, context {info});
}

template<wirable T, context ctx = context {^^T}>
consteval auto wire_size_of() -> std::size_t {
  return wire_size_of(^^T, ctx);
}

template<wirable_class T>
consteval auto get_struct_layout() -> struct_layout {
  return get_struct_layout(^^T);
}

template<wirable_class T, context ctx = context {^^T}>
consteval auto get_wire_layout() -> struct_layout {
  return get_wire_layout(^^T, ctx);
}

} // namespace rbe
