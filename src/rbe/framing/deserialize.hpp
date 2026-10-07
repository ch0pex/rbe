/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file deserialize.hpp
 * @date 07/10/2026
 * @brief deserialize and try_deserialize for vocabulary frames
 *
 * A frame is only ever read through its view, so no strategy is asked for: `deserialize<aquis::packet>(buffer)`
 * is the same as building `aquis::packet::dsrl_type` over the buffer.
 */

#pragma once

// --- Includes ---
#include <rbe/framing/frame_serder_concepts.hpp>

// --- STD ---
#include <cstddef>
#include <optional>
#include <span>

namespace rbe {

/**
 * @brief Views a buffer as the frame `T` (narrow contract)
 *
 * Preconditions:
 * - The buffer holds the whole frame, header and payload.
 *
 * If this precondition is violated, reading the frame is undefined behavior. `try_deserialize` checks it.
 *
 * @tparam T A vocabulary frame, e.g. `rbe::frame<Header, Payload>` or a custom serder like `cboe::top::line`.
 * @param input The bytes of the frame, which the view keeps referring to: they must outlive it.
 * @return The `T::dsrl_type` view over `input`.
 */
template<frame_serder T>
[[nodiscard]] constexpr auto deserialize(std::span<std::byte const> const input) -> typename T::dsrl_type {
  return typename T::dsrl_type {input};
}

/**
 * @brief Views a buffer as the frame `T`, or nothing if the buffer does not hold it
 *
 * @tparam T A vocabulary frame.
 * @param input The bytes of the frame, which the view keeps referring to: they must outlive it.
 * @return The `T::dsrl_type` view over `input`, or `std::nullopt` if the header or the payload do not fit in it.
 */
template<frame_serder T>
[[nodiscard]] constexpr auto try_deserialize(std::span<std::byte const> const input)
    -> std::optional<typename T::dsrl_type> //
{
  return T::dsrl_type::make(input);
}

} // namespace rbe
