/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file overload_set.hpp
 * @date 15/09/2026
 * @brief Overload set utility for dispatching over multiple callables
 */

#pragma once

// --- Includes ---

// --- STD ---

namespace rbe {

template<typename... Args>
struct overload : Args... {
  using Args::operator()...;
};

} // namespace rbe
