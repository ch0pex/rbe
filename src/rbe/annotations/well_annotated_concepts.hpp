/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file well_annotated_concepts.hpp
 * @date 10/08/2026
 * @brief well_annotated concept verifying the annotation correctness of a type
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/correctness.hpp>

// --- STD ---

namespace rbe {

template<typename T>
concept well_annotated = detail::annotations::well_annotated(^^T);

} // namespace rbe
