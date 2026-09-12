/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame.hpp
 * @date 08/09/2026
 * @brief Rbe frame
 */

#pragma once

// --- Includes ---
#include <rbe/framing/detail/to_dsrl_type.hpp>
#include <rbe/framing/detail/to_srl_type.hpp>
#include <rbe/framing/dsrl/frame.hpp>
#include <rbe/framing/frame_concepts.hpp>
#include <rbe/framing/srl/frame.hpp>
#include <rbe/framing/value_type.hpp>

// --- STD ---

namespace rbe {

template<frame_header HeaderType, frame_payload PayloadType>
  requires(frame_compatible<HeaderType, PayloadType>)
struct frame {
  using header_type  = HeaderType;
  using payload_type = PayloadType;
  using dsrl_type    = dsrl::frame<header_type, detail::to_dsrl_t<payload_type>>;
  // using srl_type     = srl::frame<header_type, detail::to_srl_t<payload_type>>;
  // using value_type   = rbe::value_type<header_type, payload_type>;
};

} // namespace rbe
