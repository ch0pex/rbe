/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file memcpy_constexpr.hpp
 * @date 24/06/2026
 * @brief Constexpr-safe memcpy and load helpers built on std::bit_cast
 */

#pragma once

// --- Includes ---

// --- STD ---
#include <cassert>
#include <ranges>
#include <span>

// --- System ---


namespace rbe::detail {


// NOTE: this function can only copy in compiletime if the src struct
// doesn't have any padding bytes, otherwise it will fail to compile.
template<typename T>
  requires(std::is_trivially_copyable_v<T>)
constexpr void memcpy_constexpr(std::span<std::byte> dst, T const& src) {
  assert(dst.size() >= sizeof(T));
  auto bytes = std::bit_cast<std::array<std::byte, sizeof(T)>>(src);
  std::ranges::copy(bytes, std::ranges::begin(dst));
}

template<typename T>
  requires(std::is_trivially_copyable_v<T>)
constexpr void memcpy_constexpr(std::byte* dst, T const& src) {
  memcpy_constexpr(std::span<std::byte>(dst, sizeof(T)), src);
}

template<typename T>
  requires(std::is_trivially_copyable_v<T>)
constexpr T load(std::span<std::byte const> const source) {
  assert(source.size() >= sizeof(T));
  std::array<std::byte, sizeof(T)> buffer {};
  std::ranges::copy(source, std::ranges::begin(buffer));
  return std::bit_cast<T>(buffer);
}

} // namespace rbe::detail
