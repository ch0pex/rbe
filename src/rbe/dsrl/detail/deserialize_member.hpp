/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file deserialize_member.hpp
 * @date 03/08/2026
 * @brief Helper to populate struct members during deserialization
 */

#pragma once

// --- Includes ---
#include <rbe/core/detail/context.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/core/wirable_primitives.hpp>
#include <rbe/dsrl/detail/deserialize_impl.hpp>

// --- STD ---

namespace rbe::detail {

/**
 * @brief Fill a wirable member by recursive deserialization.
 *
 * @tparam T The member type. Must satisfy `wirable`.
 * @tparam Ctx The ambient context resolved for this member, threaded one hop further down into `T`'s
 *          own deserialization so annotations keep propagating through arbitrarily deep nesting.
 * @param input Byte span containing the member's serialized data.
 */
template<wirable T, context Ctx>
auto deserialize_member(std::span<std::byte const> const input) -> T {
  return deserialize<T, Ctx>(input, dsrl::eager);
}

/**
 * @brief Fill a trivially wirable primitive member with endianness handling.
 *
 * @tparam T The primitive member type. Must satisfy `trivially_wirable_primitive`.
 * @tparam Ctx The ambient context; its `endianness` is applied to this member.
 * @param input Byte span containing the member's serialized data.
 */
template<trivially_wirable_primitive T, context Ctx>
auto deserialize_member(std::span<std::byte const> const input) -> T {
  return deserialize<T, Ctx>(input, dsrl::eager);
}

template<trivially_wirable_range T, context Ctx>
  requires(Ctx.endianness == endian::order::native or sizeof(std::ranges::range_value_t<T>) == 1)
auto deserialize_member(std::span<std::byte const> input) -> T {
  return load<T>(input);
}

template<wirable_range T, context Ctx>
auto deserialize_member(std::span<std::byte const> input) -> T {
  T array;
  using element_type = std::ranges::range_value_t<T>;
  for (auto& e: array) {
    e     = deserialize<element_type, Ctx>(input, dsrl::eager);
    input = input.subspan(wire_size_of(^^element_type));
  }
  return array;
}

} // namespace rbe::detail
