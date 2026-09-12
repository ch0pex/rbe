/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file blob.hpp
 * @date 12/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---

// --- STD ---

namespace rbe {

struct blob {
  using dsrl_type = std::span<std::byte const>;
  using srl_type  = std::span<std::byte>;
};

} // namespace rbe
