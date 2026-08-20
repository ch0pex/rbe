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

template<wirable T>
  requires(not custom_wirable<T>)
class msg;

}

template<wirable T>
  requires(std::is_default_constructible_v<T> and not trivially_wirable<T> and not custom_wirable<T>)
constexpr auto deserialize(std::span<std::byte const>, dsrl::eager_t) -> T;

template<custom_wirable T>
constexpr auto deserialize(std::span<std::byte const>, dsrl::eager_t) -> T;

template<trivially_wirable T>
  requires(not trivially_wirable_primitive<T>)
constexpr auto deserialize(std::span<std::byte const>, dsrl::eager_t) -> T;

template<trivially_wirable_primitive T, endian::order Ord>
constexpr auto deserialize(std::span<std::byte const>, dsrl::eager_t) -> T;

template<trivially_wirable T>
constexpr auto deserialize(std::span<std::byte const>, dsrl::in_place_t) -> T const&;

template<trivially_wirable T>
constexpr auto deserialize(std::span<std::byte>, dsrl::in_place_mut_t) -> T&;

template<wirable T>
constexpr auto deserialize(std::span<std::byte const>, dsrl::lazy_t) -> dsrl::msg<T>;

} // namespace rbe
