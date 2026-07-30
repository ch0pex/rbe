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

#include "rbe/core/serder.hpp"

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---


namespace rbe {

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
  return serder<T>::serialize(out, value);
}

} // namespace rbe
