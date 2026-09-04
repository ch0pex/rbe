/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file return_type_impl.hpp
 * @date 03/09/2026
 * @brief Maps a deserialization strategy tag to the type its `deserialize` overload returns.
 */

#pragma once

// --- Includes ---
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/dsrl/msg.hpp>
#include <rbe/dsrl/tags.hpp>

// --- STD ---

namespace rbe::dsrl::detail {

template<strategy Tag, wirable T>
struct return_type;

template<wirable T>
struct return_type<eager_t, T> {
  using type = T;
};

template<wirable T>
struct return_type<lazy_t, T> {
  using type = dsrl::msg<T>;
};

template<wirable T>
struct return_type<in_place_t, T> {
  using type = T const&;
};

template<wirable T>
struct return_type<in_place_mut_t, T> {
  using type = T&;
};

} // namespace rbe::dsrl::detail
