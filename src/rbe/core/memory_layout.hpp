/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file memory_layout.hpp
 * @date 27/06/2026
 * @brief In-memory and wire struct layout computation via reflection
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/alignment.hpp>
#include <rbe/annotations/detail/correctness.hpp>
#include <rbe/annotations/endianness.hpp>
#include <rbe/core/detail/context.hpp>
#include <rbe/core/detail/introspection.hpp>
#include <rbe/core/detail/static_array.hpp>
#include <rbe/core/endian.hpp>
#include <rbe/core/wirable_concepts.hpp>

// --- STD ---
#include <cstddef>
#include <ranges>

// --- System ---


namespace rbe {

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


/**
 * @brief Wire size of `info` given an inherited `ctx` -- an unannotated member's packing falls back
 * to `ctx` (whatever an outer ancestor resolved) instead of always assuming native padding, making
 * `pack` propagate correctly through arbitrarily deep unannotated nesting, exactly like endianness.
 */
consteval auto wire_size_of(std::meta::info const info, detail::context const ctx) -> std::size_t {
  auto const local = detail::merge_context(ctx, info); // info's own annotation overrides the ambient
  // remove_all_extents: an array of primitives (e.g. std::array's own raw C-array member, reached one
  // recursion hop below) is a leaf here too -- it has no inter-element padding to strip regardless of
  // packing, and unlike a genuine aggregate it has no reflectable non-static data members to recurse
  // into at all.
  if (local.alignment == alignment_mode::native or is_trivially_wirable_primitive(remove_all_extents(info))) {
    return size_of(info);
  }

  std::size_t result = 0;
  for (auto const member: detail::nsdm(info)) {
    result += wire_size_of(type_of(member), detail::merge_context(local, member));
  }
  return result;
}

/// Context-free overload: no ambient annotation is inherited from anywhere -- the behavior every
/// existing caller already relies on, unchanged.
consteval auto wire_size_of(std::meta::info const info) -> std::size_t {
  return wire_size_of(info, detail::context {});
}

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

/**
 * @brief Wire layout of `info` given an inherited `ctx` -- an unannotated member's endianness falls
 * back to `ctx` (whatever an outer ancestor resolved) instead of the dimension's global default,
 * making annotations propagate correctly through arbitrarily deep unannotated nesting.
 */
consteval auto get_wire_layout_padded(std::meta::info const info, detail::context const ctx) -> struct_layout {
  auto const local = detail::merge_context(ctx, info); // info's own annotation overrides the ambient
  std::vector<member_layout> member_layouts;

  for (auto const m: detail::nsdm(info)) {
    auto const member_ctx = detail::merge_context(local, m); // member's own annotation wins over local
    member_layouts.emplace_back(
        member_layout {
          .offset     = offset_of(m),
          .size       = wire_size_of(type_of(m), member_ctx),
          .endianness = member_ctx.endianness,
        }
    );
  }

  return {
    .size    = wire_size_of(info, ctx),
    .members = {std::from_range, member_layouts},
  };
}

consteval auto get_wire_layout_packed(std::meta::info const info, detail::context const ctx) -> struct_layout {
  auto const local   = detail::merge_context(ctx, info);
  auto const members = detail::nsdm(info);

  std::vector<member_layout> member_layouts;
  member_layouts.reserve(members.size());

  // TODO: this implementation do not support bit fields
  member_offset current_offset {};
  for (auto const& member: members) {
    auto const member_ctx  = detail::merge_context(local, member);
    auto const member_size = wire_size_of(type_of(member), member_ctx);
    member_layouts.emplace_back(
        member_layout {
          .offset     = current_offset,
          .size       = member_size,
          .endianness = member_ctx.endianness,
        }
    );
    current_offset.bytes += static_cast<std::ptrdiff_t>(member_size);
  }

  return {
    .size    = static_cast<std::size_t>(current_offset.bytes),
    .members = {std::from_range, member_layouts},
  };
}

/**
 * @brief Wire layout of `info` given an inherited `ctx` -- an unannotated member's packing falls back
 * to `ctx` (whatever an outer ancestor resolved) instead of always assuming native padding, making
 * `pack` propagate correctly through arbitrarily deep unannotated nesting, exactly like endianness.
 */
consteval auto get_wire_layout(std::meta::info const info, detail::context const ctx) -> struct_layout {
  if (detail::merge_context(ctx, info).alignment != alignment_mode::native) {
    return get_wire_layout_packed(info, ctx);
  }
  return get_wire_layout_padded(info, ctx);
}

/// Context-free overload: no ambient annotation is inherited from anywhere -- the behavior every
/// existing caller (is_trivially_wirable, get_wire_layout<T>(), ...) already relies on, unchanged.
consteval auto get_wire_layout(std::meta::info const info) -> struct_layout {
  return get_wire_layout(info, detail::context {});
}

template<wirable T>
consteval auto wire_size_of() -> std::size_t {
  return wire_size_of(^^T);
}

template<wirable_class T>
consteval auto get_struct_layout() -> struct_layout {
  return get_struct_layout(^^T);
}

template<wirable_class T, detail::context Ctx = detail::context {}>
consteval auto get_wire_layout() -> struct_layout {
  return get_wire_layout(^^T, Ctx);
}

} // namespace rbe
