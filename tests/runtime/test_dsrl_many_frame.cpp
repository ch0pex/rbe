/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_dsrl_many_frame.cpp
 * @date 23/09/2026
 * @brief Short description
 *
 * Longer description
 */

// --- Includes ---
#include "common_structs.hpp"

// --- STD ---

namespace {

using fixed_frame = rbe::dsrl::frame<PlainHeader, std::uint32_t>;
using many_fixed  = rbe::dsrl::many<fixed_frame>;


constexpr auto test_many_frame_fixed() { }


} // namespace
