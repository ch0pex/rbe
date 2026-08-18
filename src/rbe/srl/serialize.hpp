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
#include <rbe/detail/serialize_impl.hpp>

namespace rbe {

template<wirable T>
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  return detail::serialize(out, value);
}

} // namespace rbe
