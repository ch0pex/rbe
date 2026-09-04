/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file normalize.hpp
 * @date 03/08/2026
 * @brief Endianness normalization helpers for serialization
 */

#pragma once

// --- Includes ---
#include <rbe/core/trivially_wirable_concepts.hpp>
#include <rbe/core/wirable_concepts.hpp>

// --- STD ---

// --- System ---

namespace rbe::detail {

/// Identity forwarder for non-enum trivially wirable types.
auto normalize_primitive(trivially_wirable auto const value) { return value; }

/// Convert an enum to its underlying integral type for byte-swapping.
template<trivially_wirable_primitive T>
  requires(std::is_enum_v<T>)
auto normalize_primitive(T const value) -> std::underlying_type_t<T> {
  return std::to_underlying(value);
}

/// Normalize a trivially wirable primitive to the target endianness.
template<trivially_wirable_primitive T, endian::order Order>
auto normalize_endianness(T const value) -> T {
  return static_cast<T>(endian::native_to<Order>(normalize_primitive(value)));
}

/// Identity forwarder for non-primitive types (no byte-swapping needed).
template<endian::order Order>
auto normalize_endianness(auto const& value) {
  return value;
}

} // namespace rbe::detail
