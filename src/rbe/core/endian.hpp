/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file endian.hpp
 * @date 27/06/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/core/detail/memcpy_constexpr.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <bit>
#include <concepts>
#include <cstddef>

// --- System ---

namespace rbe::endian {

using order = std::endian;

template<order From = order::native>
constexpr std::integral auto to_native(std::integral auto value) {
  if constexpr (From == order::native) {
    return value;
  }
  else {
    return std::byteswap(value);
  }
}

// NOTE: This function is an alias for to_native<O>(value) and is provided for convenience.
// it might seem confusing but actually both operate the same way, the difference is in the
// semantics of the function name
template<order To = order::native>
constexpr std::integral auto native_to(std::integral auto value) {
  return to_native<To>(value);
}


template<std::integral T, order O = order::native>
constexpr T load(std::byte const* src) {
  std::array<std::byte, sizeof(T)> bytes {};
  std::ranges::copy_n(src, sizeof(T), std::ranges::begin(bytes));
  return to_native<O>(std::bit_cast<T>(bytes));
}

template<std::integral T, order O = order::native>
constexpr void store(std::byte* dst, T value) {
  detail::memcpy_constexpr(dst, native_to<O>(value));
}

} // namespace rbe::endian
