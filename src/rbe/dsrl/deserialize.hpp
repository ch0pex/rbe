/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file deserialize.hpp
 * @date 12/07/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/dsrl/tags.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <cstddef>
#include <span>

// --- System ---

namespace rbe {

/*
 * @brief Deserializes a buffer into an object eagerly.
 *
 * The function takes a span of bytes as input and deserializes it into an object of type T.
 * The deserialization is performed eagerly, meaning that the entire object is constructed at once.
 * This function will calculate at compile time the layout of the type T and will perform the necessary byte swapping if
 * needed. If the type T is trivially serializable, it will perform a direct memory copy. Otherwise, it will deserialize
 * each member of the struct.
 *
 * @tparam T The type of the object to deserialize. Must be introspectable.
 * @param input A span of bytes containing the serialized data.
 * @param eager A tag indicating that the deserialization should be performed eagerly.
 * @return An object of type T constructed from the deserialized data.
 */
template<typename T>
auto deserialize(std::span<std::byte> input, dsrl::eager_t /**/) -> T {
  //
}

/*
 * @brief Deserializes a buffer into an object lazily.
 *
 * The function takes a span of bytes as input and deserializes it into an object of type dsrl::msg<T>.
 * The deserialization is performed lazily, meaning that the members of the object are deserialized on-demand when
 * accessed.
 *
 * @tparam T The type of the object to deserialize. Must be introspectable.
 * @param input A span of bytes containing the serialized data.
 * @param lazy A tag indicating that the deserialization should be performed lazily.
 */
template<typename T>
auto deserialize(std::span<std::byte> input, dsrl::lazy_t /**/) pre(input.size_bytes() >= sizeof(T)) {
  //
}

/**
 * @brief Deserializes a buffer into an object in-place.
 *
 * The function takes a span of bytes as input and deserializes it into an existing object of type T.
 * The deserialization is performed in-place, meaning that the data is deserialized directly into the
 * specified memory location of the existing object.
 *
 * @param input A span of bytes containing the serialized data.
 * @param in_place A tag indicating that the deserialization should be performed in-place.
 * @return A reference to the deserialized object of type T.
 *
 * @note If the input buffer does not meet the alignment requirements of the type T, the behavior is undefined.
 */
template<typename T>
auto deserialize(std::span<std::byte> input, dsrl::in_place_t /**/) -> T& {
  //
}

} // namespace rbe
