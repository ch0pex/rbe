/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame_concepts.hpp
 * @date 16/09/2026
 * @brief Serialization level of the framing concepts
 *
 * Only what the srl lowering adds on top of the shared core (rbe/framing/frame_concepts.hpp) lives here.
 * The shape (rbe::is_frame), the header (rbe::frame_header) and the payload (rbe::frame_payload) are used
 * as-is from the enclosing namespace.
 */

#pragma once

// --- Includes ---
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/framing/frame_concepts.hpp>

// --- STD ---

namespace rbe::srl {

/**
 * @brief A frame serializer
 *
 * @note only the shape is required for now; the API requirements land with rbe::srl::frame
 */
template<typename T>
concept is_frame = rbe::is_frame<T> and frame_payload<typename T::payload_type>;

} // namespace rbe::srl
