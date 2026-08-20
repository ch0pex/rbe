/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file deserialize.hpp
 * @date 12/07/2026
 * @brief Deserialization routines for DSRL (Data Serialization and Retrieval Library).
 *
 * Provides eager, lazy, and in-place deserialization strategies for converting
 * serialized byte buffers back into C++ objects.
 */

#pragma once

// --- Includes ---
#include <rbe/core/trivially_wirable_concepts.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/dsrl/detail/deserialize_member.hpp>
#include <rbe/dsrl/msg.hpp>
#include <rbe/dsrl/tags.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <cstddef>
#include <memory>
#include <span>

// --- System ---

namespace rbe {

// ──────────────────────────────────────────────────────────────────
// eager deserialization
// ──────────────────────────────────────────────────────────────────

/**
 * @brief Eager deserialization for non-trivial, non-custom wirable types.
 *
 * Default-constructs the object, then deserializes each non-static data member
 * individually using the compile-time wire layout. Byte swapping is applied
 * per-member according to the layout's endianness.
 *
 * @tparam T The type to deserialize. Must be default-constructible, wirable, and
 *          neither trivially_wirable nor custom_wirable.
 * @param input A span of bytes containing the serialized data.
 * @param eager Tag for eager deserialization strategy.
 * @return A fully constructed object of type T.
 */
template<wirable T>
  requires(std::is_default_constructible_v<T> and not trivially_wirable<T> and not custom_wirable<T>)
constexpr auto deserialize(std::span<std::byte const> const input, dsrl::eager_t eager [[maybe_unused]]) -> T {
  using std::ranges::to;

  static constexpr auto wire    = get_wire_layout<T>();
  static constexpr auto members = detail::nsdm(^^T) | std::ranges::to<static_array>();

  T value;
  template for (constexpr auto [layout, member]: std::views::zip(wire.members, members)) {
    using member_type = [:type_of(member):];
    value.[:member:]  = detail::deserialize_member<member_type, layout.endianness>(
                         input.subspan<layout.offset.bytes, layout.size>()
                     );
  }
  return value;
}

/**
 * @brief Eager deserialization for types with a custom serder.
 *
 * Delegates to the user-provided `custom<T>::deserialize` implementation.
 *
 * @tparam T The type to deserialize. Must satisfy `custom_wirable`.
 * @param input A span of bytes containing the serialized data.
 * @param eager Tag for eager deserialization strategy.
 * @return A fully constructed object of type T.
 */
template<custom_wirable T>
constexpr auto deserialize(std::span<std::byte const> const input, dsrl::eager_t eager [[maybe_unused]]) -> T {
  return custom<T>::deserialize(input);
}

/**
 * @brief Eager deserialization for trivially wirable types (non-primitive).
 *
 * For types whose in-memory layout matches the wire layout, deserialization
 * is performed via a direct memory load without per-member processing.
 *
 * @tparam T The type to deserialize. Must satisfy `trivially_wirable` but not `trivially_wirable_primitive`.
 * @param input A span of bytes containing the serialized data.
 * @param eager Tag for eager deserialization strategy.
 * @return A fully constructed object of type T.
 */
template<trivially_wirable T>
  requires(not trivially_wirable_primitive<T>)
constexpr auto deserialize(std::span<std::byte const> const input, dsrl::eager_t eager [[maybe_unused]]) -> T {
  return detail::load<T>(input);
}

/**
 * @brief Eager deserialization for trivially wirable primitives.
 *
 * Loads an integral or enum type from the buffer, performing byte swapping
 * according to the specified endianness order.
 *
 * @tparam T The primitive type to deserialize. Must satisfy `trivially_wirable_primitive`.
 * @tparam Ord The endianness of the serialized data.
 * @param input A span of bytes containing the serialized data.
 * @param eager Tag for eager deserialization strategy.
 * @return A value of type T with bytes swapped to native order if necessary.
 */
template<trivially_wirable_primitive T, endian::order Ord = endian::order::native>
constexpr auto deserialize(std::span<std::byte const> const input, dsrl::eager_t eager [[maybe_unused]]) -> T {
  return endian::load<T, Ord>(input.data());
}

// ──────────────────────────────────────────────────────────────────
// in-place deserialization
// ──────────────────────────────────────────────────────────────────
/**
 * @brief In-place deserialization returning a const reference.
 *
 * Interprets the buffer as an object of type T without copying data.
 * Only viable when the wire layout matches the in-memory layout.
 *
 * @tparam T The type to interpret. Must satisfy `trivially_wirable`.
 * @param input A span of bytes containing the serialized data.
 * @param in_place Tag for in-place deserialization strategy.
 * @return A const reference to the object in the buffer.
 *
 * @note If the input buffer does not meet the alignment requirements of type T, behavior is undefined.
 */
template<trivially_wirable T>
constexpr auto deserialize(
    std::span<std::byte const> const input, //
    dsrl::in_place_t in_place [[maybe_unused]]
) -> T const& {
  auto* ptr = std::start_lifetime_as<T>(input.data());
  return *ptr;
}

/**
 * @brief In-place deserialization returning a mutable reference.
 *
 * Interprets the writable buffer as an object of type T and returns a reference.
 * No data copying occurs; the caller must ensure the buffer contains valid serialized data.
 *
 * @tparam T The type to interpret. Must satisfy `trivially_wirable`.
 * @param input A mutable span of bytes containing the serialized data.
 * @param in_place_mut Tag for in-place mutable deserialization strategy.
 * @return A mutable reference to the object in the buffer.
 *
 * @note If the input buffer does not meet the alignment requirements of type T, behavior is undefined.
 */
template<trivially_wirable T>
constexpr auto deserialize(std::span<std::byte> const input, dsrl::in_place_mut_t in_place_mut [[maybe_unused]]) -> T& {
  auto* ptr = std::start_lifetime_as<T>(input.data());
  return *ptr;
}

// ──────────────────────────────────────────────────────────────────
// lazy deserialization
// ──────────────────────────────────────────────────────────────────

/**
 * @brief Deserializes a buffer into an object lazily.
 *
 * The function takes a span of bytes as input and deserializes it into an object of type dsrl::msg<T>.
 * The deserialization is performed lazily, meaning that the members of the object are deserialized on-demand when
 * accessed.
 *
 * @tparam T The type of the object to deserialize. Must be introspectable.
 * @param input A span of bytes containing the serialized data.
 * @param lazy A tag indicating that the deserialization should be performed lazily.
 * @return A `dsrl::msg<T>` proxy that deserializes fields on-demand when accessed.
 */
template<wirable T>
constexpr auto deserialize(std::span<std::byte const> const input, dsrl::lazy_t lazy [[maybe_unused]]) -> dsrl::msg<T> {
  return dsrl::msg<T> {input};
}

} // namespace rbe
