/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file payload_size.hpp
 * @date 29/09/2026
 * @brief Customization point for the byte extent of a frame's payload
 *
 * Not every frame_payload category names its extent the same way: a wirable payload is wrapped in a
 * proxy and only has size() (its wire size, there is no separate logical length for it), while any and
 * a nested frame keep their own length(). Rather than have frame hardcode that split -- or force every
 * future payload category to pick one of those two names -- payload_size is a customization point, the
 * same shape as std::ranges::size: it prefers a member size(), falls back to a member length(), and
 * falls back further to a free function found by ADL in the payload type's own namespace, for a payload
 * type that has neither and cannot be given one (e.g. it wraps a foreign type).
 */

#pragma once

// --- STD ---
#include <concepts>
#include <cstddef>

namespace rbe::dsrl::detail {

// Poison pill: keeps an unqualified call to payload_size from inside this namespace from ever
// resolving back to the rbe::dsrl::payload_size customization point object below -- ordinary
// unqualified lookup stops at the first enclosing scope with a matching name, which is this one, so
// only ADL into the argument's own namespace can add a real candidate.
void payload_size(auto&&) = delete;

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
  template<member_size T>
  [[nodiscard]] constexpr auto operator()(T const& payload) const -> std::size_t {
    return payload.size();
  }

  template<typename T>
    requires(not member_size<T> and member_length<T>)
  [[nodiscard]] constexpr auto operator()(T const& payload) const -> std::size_t {
    return payload.length();
  }

  template<typename T>
    requires(not member_size<T> and not member_length<T> and adl_payload_size<T>)
  [[nodiscard]] constexpr auto operator()(T const& payload) const -> std::size_t {
    return payload_size(payload); // found by ADL in T's own namespace
  }
};

} // namespace rbe::dsrl::detail

namespace rbe::dsrl {

inline constexpr detail::payload_size_fn payload_length {};

} // namespace rbe::dsrl
