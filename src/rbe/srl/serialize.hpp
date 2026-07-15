/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file serialize.hpp
 * @date 02/07/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <cstddef>
#include <rbe/core/memory_layout.hpp>
#include <rbe/detail/memcpy_constexpr.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---


namespace rbe {

/**
 * @brief Serializes a trivially serializable type into a buffer.
 *
 * This function serializes the given value of type `T` into the provided buffer. The type `T` must be trivially
 * serializable, meaning that its struct layout matches its wire layout, allowing for direct memory copying.
 *
 * @tparam T The type of the value to serialize. Must be trivially serializable.
 * @param buffer A span of bytes where the serialized data will be stored. The size of the buffer must be at least
 * sizeof(T).
 * @param value The value to serialize.
 * @return The number of bytes written to the buffer, which is equal to sizeof(T).
 */
template<trivially_serializable T>
constexpr auto serialize(std::span<std::byte> out, T const& value) -> std::size_t {
  detail::memcpy_constexpr(out, value);
  return sizeof(T);
}

// NOTE: this is a baseline implementation of the serialize function for introspectable types.
// It serializes each member of the struct and perform byte swapping if needed.
// future implementations could detect chunks of the struct that can be memcpy'ed directly to the buffer
// to reduce the number of memcpy calls and potentially improve performance (?).
template<introspectable T>
constexpr auto serialize(std::span<std::byte> out, T const& value) -> std::size_t {
  using std::ranges::to;

  static constexpr auto wire    = get_wire_layout<T>();
  static constexpr auto members = detail::nsdm(^^T);

  template for (constexpr auto [layout, member]: std::views::zip(wire.members, members) | to<static_array>()) {
    auto dst = out.subspan<layout.offset.bytes, layout.size>();
    detail::memcpy_constexpr(dst, endian::native_to<layout.endianness>(value.[:member:]));
  }

  return wire.size;
}

} // namespace rbe
