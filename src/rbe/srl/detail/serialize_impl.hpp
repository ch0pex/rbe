/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file serialize.hpp
 * @date 03/08/2026
 * @brief Generic serialization implementation
 */

#pragma once

#include <rbe/core/detail/normalize.hpp>
#include <rbe/core/memory_layout.hpp>

namespace rbe::detail {

template<wirable_range T, endian::order Ord>
  requires(Ord == endian::order::native)
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  detail::memcpy_constexpr(out, value);
  return wire_size_of<std::remove_cvref_t<decltype(value)>>();
}

template<wirable_range T, endian::order Ord>
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  std::size_t size = 0;
  for (auto const& element: value) {
    serialize<std::ranges::range_value_t<T>, Ord>(out, element);
  }
  return wire_size_of<T>();
}

template<trivially_wirable_primitive T, endian::order Ord>
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  endian::store<normalize_primitive_t<T>, Ord>(out.data(), normalize_primitive(value));
  return wire_size_of<std::remove_cvref_t<decltype(value)>>();
}

/// Serializes a trivially wirable type by direct memory copy.
template<trivially_wirable T, endian::order Ord>
  requires(not trivially_wirable_primitive<T> and not wirable_range<T> and Ord == endian::order::native)
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  detail::memcpy_constexpr(out, value);
  return wire_size_of<std::remove_cvref_t<decltype(value)>>();
}

/// Serializes a custom-wirable type via its `custom<T>::serialize` specialization.
template<custom_wirable T, endian::order Ord>
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
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
template<wirable_class T, endian::order Ord>
  requires(not trivially_wirable<T> and not custom_wirable<T>)
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  using std::ranges::to;

  static constexpr auto wire    = get_wire_layout<T>();
  static constexpr auto members = detail::nsdm(^^T) | to<static_array>();

  template for (constexpr auto [layout, member]: std::views::zip(wire.members, members)) {
    using member_type = std::remove_cvref_t<typename[:type_of(member):]>;
    // TODO: With 2 deep structures this is not gonna work, should I propagate toppest endianness to bottom?
    static constexpr auto endianness = get_member_endianness(^^T, member);
    serialize<member_type, endianness>(out.subspan<layout.offset.bytes, layout.size>(), value.[:member:]);
  }

  return wire_size_of<T>();
}


} // namespace rbe::detail
