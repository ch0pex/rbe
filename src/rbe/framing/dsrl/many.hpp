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
#include <rbe/framing/detail/base_tags.hpp>
#include <rbe/framing/dsrl/frame_concepts.hpp>
#include <rbe/framing/frame_delimiting_concepts.hpp>

// --- STD ---

namespace rbe::dsrl {

template<is_frame T>
  requires self_delimiting_frame<T>
class many : public rbe::detail::many_tag { };

} // namespace rbe::dsrl
