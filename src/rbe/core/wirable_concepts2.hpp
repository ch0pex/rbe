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

namespace detail {

template<typename T>
struct wirable_trait : std::false_type { };

consteval auto members_accessible(std::meta::info const type) -> bool {
  return std::ranges::all_of(nsdm(type), std::meta::is_public) or has_annotations(type, friendly);
}

consteval auto is_wirable_class_type(std::meta::info type) -> bool {
  type = normalize_type(type);

  auto ctx = std::meta::access_context::unchecked();
  if (is_class_type(type) and not is_empty_type(type) and bases_of(type).empty() and members_accessible(type)) {
    auto is_wirable = [](std::meta::info mem) { // clang-format off
        return invoke_trait(^^wirable_trait, {std::meta::type_of(mem)} // clang-format on
      );
    };
    return std::ranges::all_of(nonstatic_data_members_of(type, ctx), is_wirable);
  }
  return false;
}

template<typename T>
concept wirable_range = //
    std::ranges::contiguous_range<T> //
    and detail::owning_range<T> //
    and std::ranges::sized_range<T> //
    and wirable_trait<std::remove_cvref_t<std::ranges::range_value_t<T>>>::value;


template<typename T>
concept wirable_class = std::is_class_v<T> and not std::ranges::range<T> and is_wirable_class_type(^^T);

} // namespace detail

template<typename T>
concept wirable = detail::wirable_trait<T>::value;

template<typename T>
concept wirable_range = wirable<T> and detail::wirable_range<T>;

template<typename T>
concept wirable_static_range = wirable_range<T> and detail::static_sized_range<T>;

template<typename T>
concept wirable_dynamic_range = wirable_range<T> and not detail::static_sized_range<T>;

template<typename T>
concept wirable_class = wirable<T> and detail::wirable_class<T>;

namespace detail {

// NOTE: All wirable types need to specialize this trait to true_type
// so we can use it in concepts and SFINAE contexts. All wirable
// concepts are more restrictive versions of wirable.

template<std::integral T>
struct wirable_trait<T> : std::true_type { };

template<typename T>
  requires std::is_enum_v<T>
struct wirable_trait<T> : std::true_type { };

template<detail::wirable_class T>
struct wirable_trait<T> : std::true_type { };

template<detail::wirable_range T>
struct wirable_trait<T> : std::true_type { };

} // namespace detail

} // namespace rbe
