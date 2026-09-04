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
#include <rbe/core/detail/context.hpp>
#include <rbe/core/trivially_wirable_concepts.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/dsrl/detail/deserialize_impl.hpp>
#include <rbe/dsrl/detail/deserialize_member.hpp>
#include <rbe/dsrl/msg.hpp>
#include <rbe/dsrl/tags.hpp>

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
 * @brief Eager deserialization for a wirable type.
 *
 * This is the single public entry point for eager deserialization: it always starts from the
 * default (empty) ambient context and dispatches internally, via `detail::deserialize`, to whichever
 * of the fast-path/custom/primitive/aggregate implementations applies to `T`.
 *
 * @tparam T The type to deserialize. Must satisfy `wirable`.
 * @param input A span of bytes containing the serialized data.
 * @param eager Tag for eager deserialization strategy.
 * @return A fully constructed object of type T.
 */
template<wirable T>
constexpr auto deserialize(std::span<std::byte const> const input, dsrl::eager_t /*eager*/) -> T {
  return detail::deserialize<T, detail::context {}>(input);
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
constexpr auto deserialize(std::span<std::byte const> const input, dsrl::in_place_t /*in_place*/) -> T const& {
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
constexpr auto deserialize(std::span<std::byte> const input, dsrl::in_place_mut_t /*in_place_mut*/) -> T& {
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
constexpr auto deserialize(std::span<std::byte const> const input, dsrl::lazy_t /*lazy*/) -> dsrl::msg<T> {
  return dsrl::msg<T> {input};
}

} // namespace rbe
