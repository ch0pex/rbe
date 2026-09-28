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
#include <rbe/dsrl/proxy.hpp>
#include <rbe/dsrl/return_type.hpp>
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
 *
 * Preconditions:
 * - The input buffer must be sufficiently large to contain a full serialized representation of `T`.
 *
 * If these preconditions are not met, behavior is undefined. For a safer version with bounds
 * checking, use `try_deserialize` instead.
 *
 * @tparam T The type to deserialize. Must satisfy `wirable`.
 * @param input A read-only span of bytes containing the serialized data.
 * @param eager Tag for eager deserialization strategy.
 * @return A fully constructed object of type T.
 */
template<wirable T>
constexpr auto deserialize(std::span<std::byte const> const input, dsrl::eager_t /**/) -> T {
  return detail::deserialize<T, detail::context {}>(input);
}

// ──────────────────────────────────────────────────────────────────
// in-place deserialization
// ──────────────────────────────────────────────────────────────────

/**
 * @brief In-place deserialization returning a const reference (narrow contract).
 *
 * Interprets the buffer as an object of type T without copying data.
 * Only viable when the wire layout matches the in-memory layout.
 *
 * Preconditions:
 * - The input buffer must be at least `sizeof(T)` bytes in size.
 * - The input buffer must be aligned to `alignof(T)`.
 *
 * If these preconditions are not met, behavior is undefined. `try_deserialize` offers a
 * safer version of this function that checks them.
 *
 * @tparam T The type to interpret. Must satisfy `trivially_wirable`.
 * @param input A read-only span of bytes containing the serialized data.
 * @param in_place Tag for in-place deserialization strategy.
 * @return A const reference to the object in the buffer.
 */
template<trivially_wirable T>
constexpr auto deserialize(std::span<std::byte const> const input, dsrl::in_place_t /**/) -> T const& {
  auto* ptr = std::start_lifetime_as<T>(input.data());
  return *ptr;
}

/**
 * @brief In-place deserialization returning a mutable reference (narrow contract).
 *
 * Interprets the writable buffer as an object of type T and returns a reference.
 * No data copying occurs; the caller must ensure the buffer contains valid serialized data.
 *
 * Preconditions:
 * - The input buffer must be at least `sizeof(T)` bytes in size.
 * - The input buffer must be aligned to `alignof(T)`.
 *
 * If these preconditions are not met, behavior is undefined. `try_deserialize` offers a
 * safer version of this function that checks them.
 *
 * @tparam T The type to interpret. Must satisfy `trivially_wirable`.
 * @param input A mutable span of bytes containing the serialized data.
 * @param in_place_mut Tag for in-place mutable deserialization strategy.
 * @return A mutable reference to the object in the buffer.
 */
template<trivially_wirable T>
constexpr auto deserialize(std::span<std::byte> const input, dsrl::in_place_mut_t /**/) -> T& {
  auto* ptr = std::start_lifetime_as<T>(input.data());
  return *ptr;
}

// ──────────────────────────────────────────────────────────────────
// lazy deserialization
// ──────────────────────────────────────────────────────────────────

/**
 * @brief Deserializes a buffer into an object lazily (narrow contract).
 *
 * The function takes a span of bytes as input and deserializes it into an object of type dsrl::proxy<T>.
 * The deserialization is performed lazily, meaning that the members of the object are deserialized on-demand when
 * accessed.
 *
 * Preconditions:
 * - The input buffer must be large enough for the proxy to operate on, given `T`'s wire layout.
 *
 * If this precondition is violated, accessing proxy fields will result in undefined behavior.
 * `try_deserialize` offers a safer version of this function that checks it.
 *
 * @tparam T The type of the object to deserialize. Must be a wirable_class.
 * @param input A read-only span of bytes containing the serialized data.
 * @param lazy A tag indicating that the deserialization should be performed lazily.
 * @return A `dsrl::proxy<T>` that deserializes fields on-demand when accessed.
 */
template<wirable T>
constexpr auto deserialize(std::span<std::byte const> const input, dsrl::lazy_t /**/) -> dsrl::proxy<T> {
  return dsrl::proxy<T> {input};
}

// ──────────────────────────────────────────────────────────────────
// safe / hardened deserialization
// ──────────────────────────────────────────────────────────────────

/**
 * @brief Safer wrapper for `deserialize` that checks buffer size and alignment.
 *
 * This function provides a safer alternative to the narrow-contract `deserialize` API by
 * validating, upfront, the preconditions that actually gate the call itself: minimum buffer
 * size and, for strategies that interpret the buffer in place, its alignment relative to
 * `alignof(T)`. If a check fails, it falls back to an empty optional instead of faulting.
 *
 * @note This function remains a narrow contract: the caller is still responsible
 *       for ensuring `input` refers to live, valid memory for its duration.
 *
 * @tparam T The `wirable` target type to deserialize the buffer into.
 * @tparam S The `strategy` applied for deserialization (e.g., zero-copy view, deep copy).
 * @param input A read-only span of bytes representing the binary payload.
 * @param strategy The strategy instance dictating the deserialization semantics.
 * @return std::optional<return_type<T, S>> The deserialized object/view if all preconditions
 *         are satisfied, or std::nullopt if the buffer is invalid or misaligned.
 */
template<wirable T, dsrl::strategy S>
constexpr auto try_deserialize(std::span<std::byte const> const input, S str)
    -> std::optional<dsrl::return_type<S, T>> //
{
  return detail::satisfy_preconditions<T>(input, str) //
             ? std::optional<dsrl::return_type<S, T>> {deserialize<T>(input, str)} //
             : std::nullopt; //
}

} // namespace rbe
