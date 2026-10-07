/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file flatten.hpp
 * @date 14/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/framing/dsrl/concepts.hpp>

// --- STD ---
#include <tuple>


namespace rbe::dsrl {

/// The header of every nested frame followed by the innermost payload: `auto [soup, itch, payload] = flatten(frame)`
template<is_frame T>
constexpr auto flatten(T const& frame) {
  if constexpr (is_frame<typename T::payload_return_type>) {
    return std::tuple_cat(std::make_tuple(frame.header()), flatten(frame.payload()));
  }
  else {
    return std::make_tuple(frame.header(), frame.payload());
  }
}

} // namespace rbe::dsrl
