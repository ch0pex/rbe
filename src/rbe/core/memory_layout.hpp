/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file memory_layout.hpp
 * @date 27/06/2026
 * @brief Public wire size query
 */

#pragma once

// --- Includes ---
#include <rbe/core/detail/memory_layout.hpp>

// --- STD ---
#include <cstddef>

namespace rbe {

/**
 * @brief Number of bytes `T` occupies on the wire, honoring its annotations (packing, ...).
 */
template<typename T>
consteval auto wire_size_of() -> std::size_t {
  return detail::wire_size_of<T>();
}

} // namespace rbe
