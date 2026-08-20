/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file well_annotated_concepts.hpp
 * @date 10/08/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/annotations_correctness.hpp>

// --- STD ---

namespace rbe {

template<typename T>
concept well_annotated =
    detail::annotations::verify_no_local_duplications(^^T) //
    and detail::annotations::verify_dimension_correctness(^^T, detail::annotations::endianness) //
    and detail::annotations::verify_dimension_correctness(^^T, detail::annotations::alignment) //
    and detail::annotations::verify_global_unique_dimension(^^T, detail::annotations::global_unique); //

} // namespace rbe
