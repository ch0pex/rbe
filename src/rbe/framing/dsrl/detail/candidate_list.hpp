/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file message_list.hpp
 * @date 05/08/2026
 * @brief Type list and variant/tuple aggregation of wirable message types
 */

#pragma once

// --- Includes ---
#include <rbe/core/detail/context.hpp>
#include <rbe/core/detail/introspection.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/core/wirable_concepts.hpp>

// --- STD ---

// --- System ---

namespace rbe {

template<wirable... T>
  requires(sizeof...(T) >= 1)
struct candidate_list {
  static constexpr auto size  = sizeof...(T);
  static constexpr auto types = std::array<std::meta::info, size> {^^T...};

  using variant_type       = std::variant<std::monostate, T...>;
  using proxy_variant_type = std::variant<dsrl::msg<T>...>;
  using id_type            = [:std::meta::type_of(get_annotated_member<id_field>(types[0]).value().info):];

  // The canonical id of every candidate in `T::types`, in the same order
  static constexpr auto ids = [] {
    std::array<id_type, size> ids {};
    template for (std::size_t index = 0; constexpr auto candidate: types) {
      using msg_type = typename[:candidate:];
      ids[index++]   = default_annotation_value<rbe::id_field, msg_type>();
    }
    return ids;
  }();
};

} // namespace rbe
