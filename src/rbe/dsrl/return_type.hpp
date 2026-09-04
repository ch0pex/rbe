/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file return_type.hpp
 * @date 03/09/2026
 * @brief Maps a deserialization strategy tag to the type its `deserialize`
 * overload returns.
 */

#pragma once

// --- Includes ---
#include <rbe/dsrl/detail/return_type_impl.hpp>

// --- STD ---

namespace rbe::dsrl {

/**
 * @brief The type `rbe::deserialize(input, Tag{})` returns for a wirable `T`.
 *
 * `type` is:
 * - `T` for `eager_t`.
 * - `dsrl::msg<T>` for `lazy_t`.
 * - `T const&` for `in_place_t`.
 * - `T&` for `in_place_mut_t`.
 *
 * Kept as an alias over the specializations in detail/return_type_impl.hpp -- rather than exposing
 * them directly -- so that mapping can change without breaking callers, e.g. if a strategy later
 * needs to report failure and starts returning `std::expected<type, error>` instead.
 *
 * @tparam Tag The deserialization strategy tag.
 * @tparam T The wirable type being deserialized.
 */
template<strategy Tag, wirable T>
using return_type = detail::return_type<Tag, T>;

} // namespace rbe::dsrl
