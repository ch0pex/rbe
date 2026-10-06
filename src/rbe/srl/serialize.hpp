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
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/srl/detail/serialize_impl.hpp>

// --- STD ---
#include <cstddef>

namespace rbe {

/**
 * @brief Serializes a wirable value into a buffer.
 *
 * This is the single public entry point for serialization: it always starts from the default
 * (empty) ambient context and dispatches internally, via `detail::serialize`, to whichever of the
 * fast-path/custom/primitive/aggregate implementations applies to `T`.
 *
 * Preconditions:
 *   - The output buffer must be large enough to hold the serialized data
 *
 * @tparam T The type to serialize. Must satisfy `wirable`.
 * @param out Output buffer large enough to hold the serialized data.
 * @param value The object to serialize.
 * @return Number of bytes written to the buffer.
 */
template<wirable T>
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  return detail::serialize<T>(out, value);
}

/**
 * @brief Serializes a wirable value into a buffer if the buffer is large enough.
 *
 * Serializes a wirable value into a buffer if the buffer is large enough to hold
 * the serialized data.
 *
 * @tparam T The type to serialize. Must satisfy `wirable`.
 * @param out Output buffer.
 * @param value The object to serialize.
 * @return Number of bytes written to the buffer if successful, or std::nullopt if the buffer is too small.
 */
template<wirable T>
constexpr auto try_serialize(std::span<std::byte> const out, T const& value) -> std::optional<std::size_t> {
  return out.size() >= wire_size_of<T>() ? std::optional<std::size_t>(serialize(out, value)) : std::nullopt;
}

} // namespace rbe
