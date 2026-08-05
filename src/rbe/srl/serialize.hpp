/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file serialize.hpp
 * @date 02/07/2026
 * @brief Serialization routines for writing wirable types to byte buffers.
 *
 * Provides overloads for trivially wirable types, custom-wirable types,
 * integral primitives, and general wirable aggregates.
 */

#pragma once

// --- Includes ---
#include <rbe/core/memory_layout.hpp>

// --- Dependencies ---
#include <rbe/concepts/trivially_wirable.hpp>
#include <rbe/concepts/wirable.hpp>
#include <rbe/core/custom.hpp>
#include <rbe/core/endian.hpp>
#include <rbe/detail/memcpy_constexpr.hpp>
#include <rbe/detail/normalize.hpp>

// --- External dependencies ---

// --- STD ---
#include <cstddef>

// --- System ---

namespace rbe {

/// Serializes a trivially wirable type by direct memory copy.
constexpr auto serialize(std::span<std::byte> const out, trivially_wirable auto const& value) -> std::size_t {
  detail::memcpy_constexpr(out, value);
  return sizeof(value);
}

/// Serializes a custom-wirable type via its `custom<T>::serialize` specialization.
constexpr auto serialize(std::span<std::byte> const out, custom_wirable auto const& value) -> std::size_t {
  return custom<std::remove_cvref_t<decltype(value)>>::serialize(out, value);
}

/**
 * @brief Serializes a wirable aggregate into a buffer member-by-member.
 *
 * Iterates over each non-static data member, normalizes its endianness
 * according to the wire layout, and recursively serializes it into the
 * corresponding offset within the output buffer.
 *
 * @tparam T The aggregate type to serialize. Must satisfy `wirable_class`.
 * @param out Output buffer large enough to hold the serialized data.
 * @param value The object to serialize.
 * @return Number of bytes written to the buffer.
 */
template<wirable_class T>
  requires(not trivially_wirable<T> and not custom_wirable<T>)
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  using std::ranges::to;

  static constexpr auto wire    = get_wire_layout<T>();
  static constexpr auto members = detail::nsdm(^^T) | to<static_array>();

  std::size_t bytes_written = 0;
  template for (constexpr auto [layout, member]: std::views::zip(wire.members, members)) {
    bytes_written = serialize(
        out.subspan<layout.offset.bytes, layout.size>(),
        detail::normalize_endianness<layout.endianness>(value.[:member:])
    );
  }

  return bytes_written;
}


} // namespace rbe
