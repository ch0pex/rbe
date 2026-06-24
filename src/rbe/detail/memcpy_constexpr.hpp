/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file memcpy_constexpr.hpp
 * @date 24/06/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---


namespace rbe::detail {

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


} // namespace rbe::detail
