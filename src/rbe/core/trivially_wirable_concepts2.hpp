
/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file wirable_concepts2.hpp
 * @date 21/10/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/utils.hpp>
#include <rbe/annotations/friendly.hpp>
#include <rbe/core/detail/introspection.hpp>
#include <rbe/core/detail/invoke_concept.hpp>
#include <rbe/core/detail/static_sized_range.hpp>

// --- STD ---
#include <algorithm>
#include <concepts>
#include <meta>
#include <ranges>
#include <vector>

namespace rbe {

template<typename T>
concept trivially_wirable = //
    wirable<T> //
    // and detail::wire_layout_of(^^T) == detail::host_layout_of(^^T)  TODO: uncomment this
    and std::is_trivially_copyable_v<T>; //

template<typename T>
concept trivially_wirable_class = wirable_class<T> and trivially_wirable<T>;

template<typename T>
concept trivially_wirable_range = //
    wirable_static_range<T> //
    and trivially_wirable<std::remove_cvref_t<std::ranges::range_value_t<T>>>; //


} // namespace rbe
