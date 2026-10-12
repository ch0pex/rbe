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
#include <rbe/frames/detail/base_tags.hpp>
#include <rbe/frames/dsrl/many.hpp>
#include <rbe/frames/frame_serder_concepts.hpp>
#include <rbe/frames/srl/many.hpp>

// --- STD ---

namespace rbe::frames {

template<frame_serder T>
struct many : detail::many_tag {
  using frame_type = T; ///< the element, as rbe::frames::many_element_t spells it at every level
  using dsrl_type  = dsrl::many<typename T::dsrl_type>;
  // TODO: using srl_type  = srl::many<typename T::srl_type>;
};

} // namespace rbe::frames
