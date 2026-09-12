/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame_concepts.hpp
 * @date 11/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/framing/dsrl/frame_concepts.hpp>

// --- STD ---
#include <concepts>

namespace rbe {

template<typename T>
concept is_any = true;

template<typename T>
concept is_many = true;

template<typename T>
concept frame_serder = requires(T const ct) {
  typename T::dsrl_type;
  // typename T::srl_type;

  // requires std::constructible_from<typename T::dsrl_type, std::span<std::byte const>>;
  // requires std::constructible_from<typename T::srl_type, std::span<std::byte>>;
};

template<typename T>
concept frame_header = wirable<T>;

/**
 * A frame payload can be the following forms:
 *   - A wirable type (struct, class, array, etc.)
 *   - Something constructible from a span of bytes (e.g. custom parser, std::span, rbe::many)
 *   - rbe::any<Args...>
 */
template<typename T>
concept frame_payload = wirable<T> or frame_serder<T>;

/// Verifies wether a header and a payload ar compatible to conform a frame
template<typename HeaderType, typename PayloadType>
// TODO: and rbe::detail::is_compatible<HeaderType, PayloadType>
concept frame_compatible = frame_header<HeaderType> and frame_payload<PayloadType>;


template<typename T>
concept is_frame = requires(T const ct) {
  requires frame_header<typename T::header_type>;
  requires frame_payload<typename T::payload_type>;
  requires frame_compatible<typename T::header_type, typename T::payload_type>;
  requires dsrl::is_frame<typename T::dsrl_type>;

  // TODO:
  // requires srl::is_frame<typeanme T::srl_type>;
  // requires value_type_of<typename T::value_type, T>;
};

} // namespace rbe
