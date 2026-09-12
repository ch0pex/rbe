/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file dsrl_type_converter.hpp
 * @date 12/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/framing/frame_concepts.hpp>

// --- STD ---

namespace rbe::detail {

template<typename T>
struct to_dsrl {
  using type = T;
};

template<frame_serder T>
struct to_dsrl<T> {
  using type = typename T::dsrl_type;
};

template<typename T>
using to_dsrl_t = typename to_dsrl<T>::type;

} // namespace rbe::detail
