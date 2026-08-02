/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file wirable_primitives.hpp
 * @date 02/08/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/core/custom.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <concepts>
#include <span>

// --- System ---

namespace rbe {

template<typename T>
concept trivially_wirable_primitive = std::is_integral_v<T> or std::is_enum_v<T>;

template<typename T>
concept custom_wirable = not trivially_wirable_primitive<T> and requires(T t) {
  { custom<T>::deserialize(std::span<std::byte const> {}) } -> std::same_as<T>;
  { custom<T>::serialize(std::span<std::byte> {}, t) } -> std::same_as<std::size_t>;
};

template<typename T>
concept wirable_primitive = trivially_wirable_primitive<T> or custom_wirable<T>;

} // namespace rbe
