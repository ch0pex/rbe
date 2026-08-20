/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file metadata.hpp
 * @date 20/08/2026
 * @brief Message metadata annotations
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/base.hpp>

namespace rbe {

/**
 * @brief Message metadata annotations
 */
inline constexpr struct : detail::base_annotation {} id {};     /// < message id
inline constexpr struct : detail::base_annotation {} length {}; /// < message length

} // namespace rbe
