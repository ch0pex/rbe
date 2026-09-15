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
#include <rbe/framing/dsrl/frame_concepts.hpp>

// --- STD ---


namespace rbe::dsrl {

template<is_frame T, strategy S>
constexpr auto flatten(T const frame, S strategy) {
  auto get_payload = [&]() { // clang-format off
    if constexpr (wirable<typename T::payload_type>) { return frame.payload(strategy); }
    else { return frame.payload(); }
  }; // clang-format on

  return std::make_tuple(frame.header(strategy), get_payload());
}

template<is_frame T, strategy S>
  requires is_frame<typename T::payload_type>
constexpr auto flatten(T const frame, S strategy) {
  return std::tuple_cat( //
        std::make_tuple(frame.header(strategy)), //
        flatten(frame.payload(), strategy) //
    );
}

} // namespace rbe::dsrl
