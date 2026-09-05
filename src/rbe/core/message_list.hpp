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
#include <rbe/core/metadata_layout.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/dsrl/msg.hpp>
#include "rbe/core/detail/context.hpp"

// --- STD ---

// --- System ---

namespace rbe {

consteval auto is_identificable(std::meta::info type, detail::context ctx = {}) -> bool {
  return get_id_layout(type, ctx).has_value();
}

consteval auto has_length_field(std::meta::info type, detail::context ctx = {}) -> bool {
  return get_length_layout(type, ctx).has_value();
}

template<typename T>
concept identificable = wirable<T> and is_identificable(^^T);

template<typename T>
consteval auto is_msg_list_type() -> bool {
  if (T::types.empty()) {
    return false;
  }

  auto id_layout     = get_id_layout(T::types[0]);
  auto length_layout = get_length_layout(T::types[0]);

  if (not id_layout.has_value() or not length_layout.has_value()) {
    return false;
  }

  return std::ranges::all_of(T::types, [&](std::meta::info type) {
    return get_id_layout(type) == id_layout and get_length_layout(type) == length_layout;
  });
}

template<typename T>
concept is_msg_list = is_msg_list_type<T>();

/// True when `T`'s candidates carry an explicit `length` field on the wire, readable straight from
/// the buffer without first knowing which candidate it is.
template<typename T>
concept explicit_length = is_msg_list<T> and has_length_field(T::types[0]);

/// True when `T`'s candidates carry no `length` field -- the size is only known once the buffer's id
/// resolves it to a concrete candidate, via that candidate's own wire size.
template<typename T>
concept implicit_length = is_msg_list<T> and not explicit_length<T>;

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
