/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file context.hpp
 * @brief Ambient annotation context threaded through recursive serialize/deserialize calls
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/alignment.hpp>
#include <rbe/annotations/detail/utils.hpp>
#include <rbe/annotations/endianness.hpp>
#include <rbe/core/endian.hpp>

// --- STD ---
#include <meta>

namespace rbe::detail {

/**
 * @brief Aggregates every dimension's resolved value (today: endianness, pack) so it can be threaded
 * down through recursive serialize/deserialize calls as a single non-type template parameter.
 *
 * A member nested arbitrarily deep under an annotated ancestor, but with no explicit annotation of its
 * own anywhere along the way, must still inherit the ancestor's annotation. `get_wire_layout`/`serialize`/
 * `deserialize` recompute a fresh `context` at every recursive step via `merge_context`, so the value
 * threads down one hop at a time instead of resetting to the dimension's default at each level.
 */
struct context {
  endian::order endianness = endian::order::native; ///< one field per dimension
  bool packed              = false; ///< `rbe::pack`: no inter-member padding once inherited

  friend constexpr bool operator==(context, context) = default; ///< required: NTTPs must be structural types
};

/// Overwrites `ambient` with whatever `entity` explicitly annotates itself; leaves everything else
/// inherited from `ambient` untouched. `pack` has no payload to extract (it's a presence-only marker,
/// unlike endianness), so once inherited it can only be turned on, never back off.
consteval auto merge_context(context const ambient, std::meta::info const entity) -> context {
  context result = ambient;
  if (auto v = resolve_in_scope<endian::order>(entity)) {
    result.endianness = *v;
  }
  if (has_annotation(entity, pack)) {
    result.packed = true;
  }
  return result;
}

} // namespace rbe::detail
