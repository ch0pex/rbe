/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file derive.hpp
 * @date 20/08/2026
 * @brief Annotation grouping facility and builtin annotation presets
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/alignment.hpp>
#include <rbe/annotations/detail/annotation.hpp>
#include <rbe/annotations/endianness.hpp>
#include <rbe/annotations/format.hpp>

namespace rbe {

/**
 * @brief Groups rbe annotations into one variable
 *
 * Users can use rbe::derive in annotations directly:
 * [[=rbe::derive<rbe::pack, rbe::little>]]
 *
 * Or they can use it to create reusable group of annotations:
 * inline constexpr auto packed_le = rbe::derive<rbe::pack, rbe::little>;
 *
 * And then use it like:
 * [[=packed_le]]
 *
 * @tparam Args Annotations to be grouped
 */
template<auto... Args>
inline constexpr detail::annotations_t<Args...> derive {};

// --- Builtin Annotation lists ---

inline constexpr auto pack_le = derive<pack, little>;
inline constexpr auto pack_be = derive<pack, big>;
inline constexpr auto debug   = derive<fmt>;

} // namespace rbe
