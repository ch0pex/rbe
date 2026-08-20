/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file format.hpp
 * @date 20/08/2026
 * @brief Debugging annotations
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/base.hpp>

namespace rbe {

/**
 * @brief Debugging annotations
 */
inline constexpr struct : detail::base_annotation {} fmt {}; /// < format message for debugging purposes

} // namespace rbe
