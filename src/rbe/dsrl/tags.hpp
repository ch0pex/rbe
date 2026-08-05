/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file tags.hpp
 * @date 12/07/2026
 * @brief Defines tags for eager and lazy deserialization strategies
 */

#pragma once

#include <concepts>

namespace rbe::dsrl {

namespace detail {

struct base_tag { };

} // namespace detail

/**
 * @brief Eager deserialization strategy tag.
 *
 * This tag indicates that the deserialization process should be performed eagerly,
 * meaning that the entire data structure is deserialized at once.
 * 'deserialize' function will return a fully constructed object of the specified type.
 */
struct eager_t : detail::base_tag { };

/**
 * @brief Lazy deserialization strategy tag.
 *
 * This tag indicates that the deserialization process should be performed lazily,
 * meaning that the data structure is deserialized on-demand, as needed.
 * 'deserialize' function will return a proxy object that will perform the actual deserialization when accessed.
 */
struct lazy_t : detail::base_tag { };

/**
 * @brief In-place deserialization strategy tag.
 *
 * This tag indicates that the deserialization process should be performed in-place,
 * meaning that the data structure is deserialized directly into the provided memory location.
 * 'deserialize' function will return a reference to the deserialized object in the provided memory.
 *
 * @note This strategy is only supported if the wire format and type format are compatible, and
 * if the pointer to the memory satisfies the alignment requirements of the type otherwise the .
 * behavior is undefined.
 */
struct in_place_t : detail::base_tag { };
struct in_place_mut_t : detail::base_tag { };

inline constexpr eager_t eager {};
inline constexpr lazy_t lazy {};
inline constexpr in_place_t in_place {};
inline constexpr in_place_mut_t in_place_mut {};

template<typename T>
concept strategy = std::derived_from<T, detail::base_tag>;

} // namespace rbe::dsrl
