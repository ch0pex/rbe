/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file overload.hpp
 * @date 21/08/2026
 * @brief The classic overload-set idiom for building a callable out of several lambdas
 */

#pragma once

namespace rbe {

/**
 * @brief Combines several callables into one overload set, e.g. for `std::visit` or `any_msg.match(...)`.
 *
 * @tparam T The callables to combine, each contributing its own `operator()` overload(s).
 */
template<class... T>
struct overload : T... {
  using T::operator()...;
};

template<class... Ts>
overload(Ts...) -> overload<Ts...>;

} // namespace rbe
