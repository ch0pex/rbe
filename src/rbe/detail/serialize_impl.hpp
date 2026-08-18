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

#include <rbe/core/memory_layout.hpp>
#include <rbe/detail/normalize.hpp>

#include "rbe/concepts/trivially_wirable.hpp"
#include "rbe/concepts/well_annotated.hpp"

namespace rbe::detail {

template<typename T>
concept trivially_serializable = trivially_wirable<T> and well_annotated<T>;

template<typename T>
concept custom_serializable = custom_wirable<T> and well_annotated<T>;

template<typename T>
concept serializable_class = well_annotated<T> and wirable_class<T> //
                             and not trivially_wirable<T> //
                             and not custom_wirable<T>; //


/// Serializes a trivially wirable type by direct memory copy.
template<trivially_serializable T, context ctx = context {^^T}>
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  detail::memcpy_constexpr(out, value);
  return sizeof(value);
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
template<serializable_class T, context ctx = context {^^T}>
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  using std::ranges::to;

  static constexpr auto wire    = get_wire_layout<T, ctx>();
  static constexpr auto members = nsdm(^^T) | to<static_array>();

  std::size_t bytes_written = 0;
  template for (constexpr auto [layout, member]: std::views::zip(wire.members, members)) {
    bytes_written +=
        serialize<typename[:type_of(member):], ctx>(out.subspan<layout.offset.bytes, layout.size>(), value.[:member:]);
  }

  return bytes_written;
}

/// Serializes a custom-wirable type via its `custom<T>::serialize` specialization.
template<custom_serializable T, context ctx = context {^^T}>
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  return custom<std::remove_cvref_t<decltype(value)>>::serialize(out, value);
}

// bad annotated fallback
template<wirable T, context ctx = context {^^T}>
  requires(not well_annotated<T>)
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  static_assert(well_annotated<T>); // TODO: diagnose_badly_annotated
  return {};
}

// template<trivially_wirable_primitive T, context ctx = {}>
// constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
//   endian::store<T, ctx.endianness>(out.data(), normalize_primitive(value));
//   return wire_size_of<std::remove_cvref_t<decltype(value)>>();
// }

// template<wirable_range T, endian::order Ord>
//   requires(Ord == endian::order::native)
// constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
//   detail::memcpy_constexpr(out, value);
//   return wire_size_of<std::remove_cvref_t<decltype(value)>>();
// }
//
// template<wirable_range T, endian::order Ord>
// constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
//   std::size_t size = 0;
//   for (auto const& element: value) {
//     serialize<std::ranges::range_value_t<T>, Ord>(out, element);
//   }
//   return wire_size_of<T>();
// }
//
//
// /// Serializes a trivially wirable type by direct memory copy.
// template<trivially_wirable T, endian::order Ord>
//   requires(not trivially_wirable_primitive<T> and not wirable_range<T> and Ord == endian::order::native)
// constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
//   detail::memcpy_constexpr(out, value);
//   return wire_size_of<std::remove_cvref_t<decltype(value)>>();
// }
//
// /// Serializes a custom-wirable type via its `custom<T>::serialize` specialization.
// template<custom_wirable T, endian::order Ord>
// constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
//   return custom<std::remove_cvref_t<decltype(value)>>::serialize(out, value);
// }
//
// /**
//  * @brief Serializes a wirable aggregate into a buffer member-by-member.
//  *
//  * Iterates over each non-static data member, normalizes its endianness
//  * according to the wire layout, and recursively serializes it into the
//  * corresponding offset within the output buffer.
//  *
//  * @tparam T The aggregate type to serialize. Must satisfy `wirable_class`.
//  * @param out Output buffer large enough to hold the serialized data.
//  * @param value The object to serialize.
//  * @return Number of bytes written to the buffer.
//  */
// template<wirable_class T, endian::order Ord>
//   requires(not trivially_wirable<T> and not custom_wirable<T>)
// constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
//   using std::ranges::to;
//
//   static constexpr auto wire    = get_wire_layout<T>();
//   static constexpr auto members = detail::nsdm(^^T) | to<static_array>();
//
//   template for (constexpr auto [layout, member]: std::views::zip(wire.members, members)) {
//     using member_type = std::remove_cvref_t<typename[:type_of(member):]>;
//     // TODO: With 2 deep structures this is not gonna work, should I propagate toppest endianness to bottom?
//     static constexpr auto endianness = get_member_endianness(^^T, member);
//     serialize<member_type, endianness>(out.subspan<layout.offset.bytes, layout.size>(), value.[:member:]);
//   }
//
//   return wire_size_of<T>();
// }


} // namespace rbe::detail
