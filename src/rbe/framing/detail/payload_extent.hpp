/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file payload_extent.hpp
 * @date 14/09/2026
 * @brief Compile-time classification of where a frame resolves its payload length from
 *
 * The classification only looks at the structure of a frame (its header_type and payload_type), so it applies
 * equally to rbe:: vocabulary frames and to their dsrl:: / srl:: lowerings: lowering never changes the extent.
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/length.hpp>
#include <rbe/annotations/well_annotated_concepts.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/framing/frame_concepts.hpp>

// --- STD ---
#include <cstdint>

namespace rbe::detail {

/**
 * @brief Where a frame resolves its payload length from, in priority order
 *
 * Wire values take precedence over static sizes. Shared by dsrl::frame::payload_length() and the
 * rbe::self_delimiting_frame / rbe::buffer_delimited_frame concepts, so the runtime resolution and the
 * compile-time classification cannot drift apart.
 */
enum class payload_extent : std::uint8_t {
  payload_length_field, ///< payload_length annotated header field
  frame_length_field, ///< frame_length annotated header field minus header_length
  nested_frame, ///< the nested frame's own length(), self-delimiting only if the nested frame is
  static_size, ///< rbe::wire_size_of<payload_type>()
  // TODO: any_id -- rbe::any whose alternatives imply their length from the id
  buffer_end, ///< the payload extends to the end of the buffer (blob, span constructible, any, many)
};

template<frame_header HeaderType, typename PayloadType>
[[nodiscard]] consteval auto payload_extent_of() -> payload_extent {
  if (contains_annotation<HeaderType, rbe::payload_length>) {
    return payload_extent::payload_length_field;
  }
  if (contains_annotation<HeaderType, rbe::frame_length>) {
    return payload_extent::frame_length_field;
  }
  if (is_frame<PayloadType>) {
    return payload_extent::nested_frame;
  }
  if (wirable<PayloadType>) {
    return payload_extent::static_size;
  }
  return payload_extent::buffer_end;
}

// concepts cannot be recursive, so the walk through nested frames lives here
template<is_frame FrameType>
[[nodiscard]] consteval auto is_self_delimiting() -> bool {
  constexpr auto extent = payload_extent_of<typename FrameType::header_type, typename FrameType::payload_type>();
  if constexpr (extent == payload_extent::nested_frame) {
    return is_self_delimiting<typename FrameType::payload_type>();
  }
  else {
    return extent != payload_extent::buffer_end;
  }
}

} // namespace rbe::detail
