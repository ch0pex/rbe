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
#include <rbe/concepts/wirable.hpp>
#include <rbe/detail/deserialize_fwd.hpp>
#include "rbe/concepts/wirable_primitives.hpp"
#include "rbe/core/memory_layout.hpp"

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---

namespace rbe::detail {

/**
 * @brief Fill a wirable member by recursive deserialization.
 *
 * @tparam T The member type. Must satisfy `wirable`.
 * @tparam Ord Endianness of the serialized data (unused here, forwarded to nested calls).
 * @param member Reference to the member to populate.
 * @param input Byte span containing the member's serialized data.
 */
template<wirable T, endian::order Ord>
auto deserialize_member(std::span<std::byte const> const input) -> T {
  return deserialize<T>(input, dsrl::eager);
}

/**
 * @brief Fill a trivially wirable primitive member with endianness handling.
 *
 * @tparam T The primitive member type. Must satisfy `trivially_wirable_primitive`.
 * @tparam Ord Endianness of the serialized data.
 * @param member Reference to the member to populate.
 * @param input Byte span containing the member's serialized data.
 */
template<trivially_wirable_primitive T, endian::order Ord>
auto deserialize_member(std::span<std::byte const> const input) -> T {
  return deserialize<T, Ord>(input, dsrl::eager);
}

// template<std::meta::info Parent, std::meta::info Member>
//   requires(is_trivially_wirable_primitive(Member))
// auto deserialize_member(std::span<std::byte const> const input) -> [ : type_of(MemberInfo) : ] {
//   using member_type                   = [:type_of(rbe::detail::nsdm(^^value_type, Index)):];
//   static constexpr auto member_layout = wire.members[Index];
// }

} // namespace rbe::detail
