/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file fill_member.hpp
 * @date 03/08/2026
 * @brief Helper to populate struct members during deserialization
 */

#pragma once

// --- Includes ---
#include <rbe/concepts/wirable.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <span>

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
auto fill_member(T& member, std::span<std::byte const> const input) -> void {
  member = deserialize<type_of(member)>(input);
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
auto fill_member(T& member, std::span<std::byte const> const input) -> void {
  member = deserialize<type_of(member), Ord>(input);
}

} // namespace rbe::detail
