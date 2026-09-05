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
#include <rbe/core/detail/introspection.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/core/message_concepts.hpp>
#include <rbe/core/metadata_layout.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/dsrl/msg.hpp>
#include "rbe/core/detail/context.hpp"

// --- STD ---

// --- System ---

namespace rbe {

template<identificable... T>
  requires(sizeof...(T) >= 1)
struct msg_list {
  static constexpr std::size_t size                        = sizeof...(T);
  static constexpr std::array<std::meta::info, size> types = {^^T...};
  static constexpr auto metadata                           = get_metadata_layout(types[0]);

  using variant_type       = std::variant<T...>;
  using proxy_variant_type = std::variant<dsrl::msg<T>...>;
  using id_type            = [:std::meta::type_of(get_annotated_member<id>(types[0]).value().info):];

  // The canonical id of every candidate in `T::types`, in the same order -- built once so `match`/
  // `as_variant` don't redo it per call. Relies on `is_msg_list`'s guarantee that `id`'s layout (and,
  // in practice, its type) is the same across every candidate.
  static constexpr auto ids = [] {
    std::array<id_type, size> ids {};
    template for (std::size_t index = 0; constexpr auto candidate: types) {
      using msg_type = typename[:candidate:];
      ids[index++]   = default_annotation_value<rbe::id, msg_type>();
    }
    return ids;
  }();
};

} // namespace rbe
