/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file string_literals.hpp
 * @date 24/06/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---
#include <meta>
#include <string_view>

namespace rbe::literals {

/**
 * @brief User-defined literal for static strings.
 */
consteval auto operator""_ss(char const* str, [[maybe_unused]] size_t length) -> char const* {
  std::string_view const sv {str, length};
  return std::define_static_string(sv);
}

} // namespace rbe::literals
