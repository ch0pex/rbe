/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file serder.hpp
 * @date 17/07/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/concepts/trivially_wirable.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <meta>

// --- System ---

namespace rbe {
//
// template<class T>
// struct serder;
//
// template<class T>
//   requires wirable<T>
// struct serder<T> {
//   static constexpr std::size_t serialize(std::span<std::byte> const dst, T const& value) {
//     using std::ranges::to;
//
//     static constexpr auto wire    = get_wire_layout<T>();
//     static constexpr auto members = detail::nsdm(^^T);
//
//     template for (constexpr auto [layout, member]: std::views::zip(wire.members, members) | to<static_array>()) {
//       detail::memcpy_constexpr(
//           dst.subspan<layout.offset.bytes, layout.size>(), //
//           endian::native_to<layout.endianness>(value.[:member:]) //
//       );
//     }
//
//     return wire.size;
//   }
//
//   static constexpr T deserialize(std::span<std::byte const> const data) { return detail::load<T>(data); }
// };
//
// template<class T>
//   requires trivially_wirable<T>
// struct serder<T> {
//   static constexpr std::size_t serialize(std::span<std::byte> dst, T const& value) {
//     detail::memcpy_constexpr(dst, value);
//     return sizeof(value);
//   }
//
//   static constexpr T deserialize(std::span<std::byte const> const data) { return detail::load<T>(data); }
// };

} // namespace rbe
