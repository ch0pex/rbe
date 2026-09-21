/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame_serder_concepts.hpp
 * @date 11/09/2026
 * @brief Vocabulary level of the framing concepts
 *
 * Only what the vocabulary level adds on top of the shared core (frame_concepts.hpp) lives here: the traits
 * a type must expose to be lowered (rbe::serder_traits), the payload a user may write
 * (rbe::frame_serder_payload) and the check that both lowerings of a frame are well formed
 * (rbe::frame_serder).
 */

#pragma once

// --- Includes ---
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/framing/dsrl/concepts.hpp>
#include <rbe/framing/frame_concepts.hpp>
#include <rbe/framing/srl/concepts.hpp>

// --- STD ---
#include <concepts>

namespace rbe {

/**
 * @brief A vocabulary type that knows how to lower itself to a (de)serializer
 *
 * rbe::frame, rbe::blob, rbe::many and rbe::any are the vocabulary types the user writes; each one names its
 * dsrl / srl counterpart, which is what rbe::detail::to_dsrl_t and rbe::detail::to_srl_t map through.
 */
template<typename T>
concept serder_traits = requires {
  typename T::dsrl_type;
  // typename T::srl_type;
};

/**
 * @brief The payload of a vocabulary frame
 *
 * The vocabulary level does not spell the payload categories out the way rbe::frame_payload does:
 * a payload is either a plain wirable message, lowered as-is, or a vocabulary type that lowers itself, which
 * is what blob, many, any and nested frames have in common.
 */
template<typename T>
concept frame_serder_payload = wirable<T> or serder_traits<T>;

/**
 * @brief A vocabulary frame: a frame shape whose lowerings are both well formed
 *
 * This is what rbe::frame produces, and what a user writes their protocol with. Its lowerings are checked by
 * the concept of their own level, so a malformed frame is rejected where it is written, not where it is used.
 */
template<typename T>
concept frame_serder = is_frame<T> and serder_traits<T> and requires {
  requires frame_serder_payload<typename T::payload_type>;
  requires dsrl::is_frame<typename T::dsrl_type>;

  // TODO:
  // requires srl::is_frame<typename T::srl_type>;
  // requires value_type_of<typename T::value_type, T>;
};

template<frame_serder T>
using frame_value_t = T::value_type;

template<frame_serder T>
using frame_dsrl_t = T::dsrl_type;

template<frame_serder T>
using frame_srl_t = T::srl_type;

} // namespace rbe
