/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file message_concepts.hpp
 * @date 05/09/2026
 * @brief Concepts classifying wirable types by their `id`/`length` metadata
 *
 * Kept apart from message_list.hpp -- which needs the full definition of `dsrl::msg` for
 * `msg_list::proxy_variant_type` -- so that `dsrl::msg` (which itself constrains on `identificable`)
 * can depend on these concepts without the two headers including each other.
 */

#pragma once

// --- Includes ---
#include <rbe/core/detail/context.hpp>
#include <rbe/core/metadata_layout.hpp>
#include <rbe/core/wirable_concepts.hpp>

// --- STD ---
#include <algorithm>
#include <ranges>

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

/// True when `U` is one of the wirable candidates listed in `MsgList::types`.
template<typename U, typename MsgList>
concept belongs_to = is_msg_list<MsgList> and wirable<U> and std::ranges::contains(MsgList::types, ^^U);

} // namespace rbe
