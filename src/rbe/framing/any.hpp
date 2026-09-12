/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file any.hpp
 * @date 11/09/2026
 * @brief Short description
 */

#pragma once

// --- Includes ---
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/framing/dsrl/any.hpp>
#include <rbe/framing/srl/any.hpp>


namespace rbe {

template<wirable_class... Args>
struct any {
  using dsrl_type = dsrl::any<Args...>;
  // using srl_type  = srl::any<Args...>;
};

} // namespace rbe
