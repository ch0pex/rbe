/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file many.hpp
 * @date 11/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/framing/dsrl/many.hpp>
#include <rbe/framing/frame_concepts.hpp>

// --- STD ---

namespace rbe {

template<is_frame T>
struct many {
  using dsrl_type = dsrl::many<T>;
  // TODO:
  // using srl_type  = srl::many<Args>;
};

} // namespace rbe
