/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file annotation_concepts.hpp
 * @date 10/08/2026
 * @brief well_annotated concept verifying the annotation correctness of a type
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/correctness.hpp>
#include <rbe/annotations/detail/utils.hpp>

// --- STD ---

namespace rbe {

template<typename T>
concept well_annotated = detail::annotations::well_annotated(^^T);

template<typename T, auto Annotation>
concept contains_annotation = detail::has_annotations_deep(^^T, Annotation);

} // namespace rbe
