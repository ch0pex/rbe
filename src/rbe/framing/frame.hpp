/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame.hpp
 * @date 08/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/framing/frame_concepts.hpp>

// --- STD ---

namespace rbe {

template<frame_header HeaderType, frame_payload PayloadType>
  requires(frame_compatible<HeaderType, PayLoadType>)
struct frame {
  using header_type = HeaderType;
  using PayloadType = PayloadType;
  using dsrl_type   = detail::dsrl_type<header_type, payload_type>;
  using srl_type    = detail::srl_type<header_type, payload_type>;
  using value_type  = detail::value_type<header_type, payload_type>;
};

} // namespace rbe
