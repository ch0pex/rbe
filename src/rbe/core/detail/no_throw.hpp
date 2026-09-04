/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file no_throw.hpp
 * @date 05/09/2026
 * @brief Turns a throwing consteval diagnostic into a bool, so it can double as a concept's requirement
 */

#pragma once

// --- STD ---
#include <functional>
#include <utility>

namespace rbe::detail {

/**
 * @brief Returns `true` if invoking `diagnose(args...)` doesn't throw, `false` if it does.
 *
 * @param diagnose Any callable, e.g. `diagnose_foo<Args...>` -- passed as an ordinary argument rather than
 * a non-type template parameter, so it isn't limited to plain functions (a capturing lambda works too).
 */
template<class Diagnose, class... Args>
constexpr auto no_throw(Diagnose&& diagnose, Args&&... args) noexcept -> bool try {
  std::invoke(std::forward<Diagnose>(diagnose), std::forward<Args>(args)...);
  return true;
}
catch (...) {
  return false;
}

} // namespace rbe::detail
