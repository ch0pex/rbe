/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file payload_size.hpp
 * @date 29/09/2026
 * @brief Customization point for the byte extent of a frame's payload
 */

#pragma once

// --- STD ---
#include <concepts>
#include <cstddef>

namespace rbe::dsrl::detail {

template<typename T>
concept member_size = requires(T const& payload) {
  { payload.size() } -> std::convertible_to<std::size_t>;
};

template<typename T>
concept member_length = requires(T const& payload) {
  { payload.length() } -> std::convertible_to<std::size_t>;
};

template<typename T>
concept adl_payload_size = requires(T const& payload) {
  { payload_size(payload) } -> std::convertible_to<std::size_t>;
};

struct payload_size_fn {
  template<member_length T>
  [[nodiscard]] constexpr auto operator()(T const& payload) const -> std::size_t {
    return payload.length();
  }

  template<typename T>
    requires(not member_length<T> and member_size<T>)
  [[nodiscard]] constexpr auto operator()(T const& payload) const -> std::size_t {
    return payload.size();
  }

  template<typename T>
    requires(not member_length<T> and not member_size<T> and adl_payload_size<T>)
  [[nodiscard]] constexpr auto operator()(T const& payload) const -> std::size_t {
    return payload_size(payload); // found by ADL in T's own namespace
  }
};

inline constexpr payload_size_fn payload_length {};

} // namespace rbe::dsrl::detail
