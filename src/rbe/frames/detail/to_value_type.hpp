/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file value_type.hpp
 * @date 11/09/2026
 * @brief Value type converter from frame
 */

#pragma once

// --- Includes ---
#include <rbe/frames/frame_serder_concepts.hpp>

// --- STD ---


namespace rbe::frames {

namespace detail { }

// TODO:
template<frames::header HeaderType, frames::serder_payload PayloadType>
using value_type = void;

} // namespace rbe::frames
