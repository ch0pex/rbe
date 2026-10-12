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
#include <rbe/frames/detail/to_dsrl_type.hpp>
#include <rbe/frames/detail/to_srl_type.hpp>
#include <rbe/frames/dsrl/frame.hpp>
#include <rbe/frames/frame_delimiting_concepts.hpp>
#include <rbe/frames/srl/frame.hpp>
#include <rbe/frames/value_type.hpp>

// --- STD ---

namespace rbe::frames {


/**
 * @brief Frame serder builder class
 *
 * This empty class's sole purpose is to offer a single type with both serialization
 * and deserialization type aliases following serder_traits. This way, the user can define
 * their frame only once and extract both srl and dsrl types from it.
 */
template<header HeaderType, serder_payload PayloadType>
struct frame {
  using header_type        = HeaderType;
  using payload_type       = PayloadType;
  using delimiting_concept = frame_delimiting_tag<HeaderType, PayloadType>;

  using dsrl_type = dsrl::frame<header_type, detail::to_dsrl_t<payload_type>>;
  // using srl_type     = srl::frame<header_type, detail::to_srl_t<payload_type>>;
  // using value_type   = rbe::frames::value_type<header_type, payload_type>;
};

} // namespace rbe::frames
