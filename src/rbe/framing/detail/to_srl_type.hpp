/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file to_srl_type.hpp
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
struct to_srl {
  using type = T;
};

template<frame_serder T>
struct to_srl<T> {
  using type = typename T::srl_type;
};

template<typename T>
using to_srl_t = typename to_srl<T>::type;

} // namespace rbe::detail
