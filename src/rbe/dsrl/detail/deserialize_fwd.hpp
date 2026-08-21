/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file fill_member.hpp
 * @date 03/08/2026
 * @brief Helper to populate struct members during deserialization
 */

#pragma once

// --- Includes ---
#include <rbe/core/detail/context.hpp>
#include <rbe/core/endian.hpp>
#include <rbe/core/trivially_wirable_concepts.hpp>
#include <rbe/dsrl/tags.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <span>

// --- System ---

namespace rbe {

namespace dsrl {

template<wirable T, rbe::detail::context Ctx = rbe::detail::context {}>
  requires(not custom_wirable<T>)
class msg;

}

namespace detail {

template<trivially_wirable T, context Ctx = context {}>
  requires(Ctx == context {})
constexpr auto deserialize(std::span<std::byte const>, dsrl::eager_t) -> T;

template<custom_wirable T, context Ctx = context {}>
constexpr auto deserialize(std::span<std::byte const>, dsrl::eager_t) -> T;

template<trivially_wirable_primitive T, context Ctx>
  requires(Ctx != context {})
constexpr auto deserialize(std::span<std::byte const>, dsrl::eager_t) -> T;

template<wirable_class T, context Ctx = context {}>
  requires(
      std::is_default_constructible_v<T> and not custom_wirable<T> and not wirable_range<T> and
      (not trivially_wirable<T> or Ctx != context {})
  )
constexpr auto deserialize(std::span<std::byte const>, dsrl::eager_t) -> T;

template<wirable_range T, context Ctx>
  requires(not trivially_wirable_range<T> or Ctx != context {})
constexpr auto deserialize(std::span<std::byte const>, dsrl::eager_t) -> T;

} // namespace detail

template<wirable T>
constexpr auto deserialize(std::span<std::byte const>, dsrl::eager_t) -> T;

template<trivially_wirable T>
constexpr auto deserialize(std::span<std::byte const>, dsrl::in_place_t) -> T const&;

template<trivially_wirable T>
constexpr auto deserialize(std::span<std::byte>, dsrl::in_place_mut_t) -> T&;

template<wirable T>
constexpr auto deserialize(std::span<std::byte const>, dsrl::lazy_t) -> dsrl::msg<T>;

} // namespace rbe
