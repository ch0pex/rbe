/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file agnostic.hpp
 * @date 10/08/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once
#include "rbe/detail/annotations_correctness.hpp"

// --- Includes ---

// --- STD ---


namespace rbe {

template<typename T>
concept well_annotated = detail::is_well_annotated(^^T);

} // namespace rbe
