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
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/dsrl/deserialize.hpp>
#include <rbe/dsrl/tags.hpp>
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

template<frame_header HeaderType, frame_payload PaylaodType>
class frame {
public:
  // --- Type traits ---

  using header_type  = HeaderType;
  using payload_type = PaylaodType;
  using buffer_type  = std::span<std::byte const>;
  using size_type    = std::size_t;

  // --- Constructors ---

  /**
   * @brief Construct a new frame object from a span of bytes
   *
   * Preconditions:
   *   - the span must be at least as large as the frame length
   */
  constexpr explicit frame(buffer_type const data) : data_(data) { }

  // --- Member accessors ---

  template<strategy S>
  [[nodiscard]] constexpr auto header(S strategy = lazy) const -> return_type<S, header_type> {
    return rbe::deserialize<header_type>(header_span(), strategy);
  }

  [[nodiscard]] constexpr auto payload(strategy auto strategy = lazy) const -> payload_type
    requires(wirable<payload_type>)
  {
    return rbe::deserialize<payload_type>(payload_span(), strategy);
  }

  [[nodiscard]] constexpr auto payload() const -> payload_type
    requires(std::constructible_from<payload_type, buffer_type>)
  {
    return payload_type {payload_span()};
  }

  [[nodiscard]] constexpr auto payload() const -> payload_type
    requires(is_any<payload_type>)
  {
    // TODO: any construction from id and span
    return payload_type {payload_span()};
  }

  [[nodiscard]] constexpr auto flatten(strategy auto strategy = lazy) { return flatten(*this, strategy); }

  // --- Size accessors ---

  /**
   * @brief Return the length of the frame in bytes, excluding any padding
   * @return The length of the frame in bytes (header + payload), excluding any padding
   */
  [[nodiscard]] constexpr auto length() const -> size_type {
    // frame_length can either be gathered from:
    //   - frame_length annotated field value if specified
    //   - otherwise from header_length() + payload_length() if both are known
    return 0;
  }

  [[nodiscard]] constexpr auto header_length() const -> size_type {
    // header_length can either be gathered from:
    //   - header_length annotated field value if specified
    //   - otherwise from rbe::wire_size_of<header_type>() (variable-size header wouldbe malformed)
    return 0;
  }

  [[nodiscard]] constexpr auto payload_length() const -> size_type {
    // payload_length can either be gathered from:
    //   - payload_length annotated field value if specified
    //   - otherwise from rbe::wire_size_of<payload_type>() (if variable-size payload, then the frame is malformed)
    return 0;
  }

  // --- Buffer accessors ---

  /**
   * @brief Return a span of bytes representing the header portion of the frame
   * @return A span of bytes representing the header portion of the frame
   */
  [[nodiscard]] constexpr auto header_span() const -> buffer_type {
    // TODO: Determine proper header length and payload length based on
    // annotations and wire layout
    return data_.first(header_length());
  }

  /**
   * @brief Return a span of bytes representing the payload portion of the frame
   * @return A span of bytes representing the payload portion of the frame
   */
  [[nodiscard]] constexpr auto payload_span() const -> buffer_type {
    // TODO: Determine proper header length and payload length based on
    // annotations and wire layout
    return data_.subspan(header_length());
  }

  /**
   * @brief Return a span of bytes representing the entire frame, excluding any padding
   * @return A span of bytes representing the entire frame, excluding any padding
   */
  [[nodiscard]] constexpr auto as_span() const -> buffer_type { return data_.subspan(length()); }

  /**
   * @brief Return a span of bytes representing the entire frame, including any padding
   * @return A span of bytes representing the entire frame, including any padding
   */
  [[nodiscard]] constexpr auto data() const -> buffer_type { return data_; }


private:
  buffer_type data_;
};


} // namespace rbe::dsrl
