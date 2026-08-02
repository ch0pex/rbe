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

#include "rbe/concepts/trivially_wirable.hpp"
#include "rbe/concepts/wirable.hpp"
#include "rbe/core/custom.hpp"
#include "rbe/core/endian.hpp"

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---


namespace rbe {

namespace detail {

template<endian::order Order>
auto normalize_endiannes(std::integral auto const value) -> std::integral auto {
  return endian::native_to<Order>(value);
}

template<endian::order Order>
auto normalize_endianness(auto const& value) -> auto const& {
  return value;
}

} // namespace detail

constexpr auto serialize(std::span<std::byte> const out, trivially_wirable auto const& value) -> std::size_t {
  detail::memcpy_constexpr(out, value);
  return sizeof(value);
}

constexpr auto serialize(std::span<std::byte> const out, custom_wirable auto const& value) -> std::size_t {
  return custom<std::remove_cvref_t<decltype(value)>>::serialize(out, value);
}

constexpr auto serialize(std::span<std::byte> const out, std::integral auto const& value) -> std::size_t {
  endian::store<decltype(value), endian::order::native>(out.data(), value);
  return sizeof(value);
}

template<typename T>
concept serializable = requires(T const& value, std::span<std::byte> const out) {
  { serialize(out, value) } -> std::same_as<std::size_t>;
};

/**
 * @brief Serializes a wirable type into a buffer.
 *
 * This function serializes the given value of type `T` into the provided buffer. The type `T` must be wirable
 *
 * @tparam T The type of the value to serialize. Must be trivially serializable.
 * @param out A span of bytes where the serialized data will be stored. The size of the buffer must be at least
 * sizeof(T).
 * @param value The value to serialize.
 * @return The number of bytes written to the buffer, which is equal to sizeof(T).
 */

template<wirable T>
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  using std::ranges::to;

  static constexpr auto wire    = get_wire_layout<T>();
  static constexpr auto members = detail::nsdm(^^T);

  std::size_t bytes_written = 0;
  template for (constexpr auto [layout, member]: std::views::zip(wire.members, members) | to<static_array>()) {
    bytes_written = serialize(
        out.subspan<layout.offset.bytes, layout.size>(),
        detail::normalize_endianness<layout.endianness>(value.[:member:])
    );
  }

  return bytes_written;
}


} // namespace rbe
