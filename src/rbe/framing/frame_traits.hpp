/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame_traits.hpp
 * @date 11/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/framing/frame_concepts.hpp>

// --- STD ---

namespace rbe {

template<is_frame T>
using frame_header_t = T::header_type;

template<is_frame T>
using frame_payload_t = T::payload_type;

template<is_frame T>
using frame_value_t = T::value_type;

template<is_frame T>
using frame_dsrl_t = T::dsrl_type;

template<is_frame T>
using frame_srl_t = T::srl_type;

} // namespace rbe
