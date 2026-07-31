/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file trivially_wirable.hpp
 * @date 31/07/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/core/memory_layout.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---

namespace rbe {

/**
 * @brief Concept to check if a type is trivially wirable.
 *
 * A type is considered trivially wirable if its struct layout matches its wire layout.
 * Meaning that the in-memory representation of the type can be directly used for serialization without any additional
 * processing. Therefore, serialization and deserialization can be performed by simply memcpy'ing the data to and from a
 * buffer.
 *
 */
template<typename T>
concept trivially_wirable = //
    std::is_trivially_copyable_v<T> //
    and std::is_standard_layout_v<T> //
    and wirable<T> //
    and get_struct_layout<T>() == get_wire_layout<T>(); //

} // namespace rbe
