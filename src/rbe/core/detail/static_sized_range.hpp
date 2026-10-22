/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file static_sized_range.hpp
 * @date 12/10/2026
 * @brief static_sized_range concept
 *
 * Until C++29 we can implement our selfs static_sized_range
 * to distinguish between dynamic sized ranges at runtime such as
 * vector and static sized ranges at compile time such as std::array.
 */

#pragma once

// --- STD ---
#include <ranges>
#include <utility>

namespace rbe::detail {

template<class T>
concept static_sized_range = std::ranges::sized_range<T> && requires(T& t) { std::cw<std::ranges::size(t)>; };

template<static_sized_range T>
consteval auto static_size_of() {
  return decltype([](T& t) { return std::cw<std::ranges::size(t)>; }(std::declval<T&>()))::value;
}

template<typename T>
concept owning_range = std::ranges::range<T> and not std::ranges::borrowed_range<std::remove_cvref_t<T>>;

template<typename T>
concept dynamic_sized_range = std::ranges::sized_range<T> and not static_sized_range<T>;

} // namespace rbe::detail
