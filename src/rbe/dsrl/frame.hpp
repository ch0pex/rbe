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

// --- STD ---

namespace rbe::dsrl {

// TODO:
template<typename T>
concept is_any = true;

template<typename T>
using frame_header = wirable<T>;

/**
 * A frame payload can be the following forms:
 *   - A wirable type (struct, class, array, etc.)
 *   - Something constructible from a span of bytes (e.g. custom parser, std::span, rbe::many)
 *   - rbe::any<Args...>
 */
template<typename T>
using frame_payload = wirable<T> //
                      or is_any<T> //
                      or std::constructible_from<T, std::span<std::byte const>>; //

template<typename T>
conept is_frame = requires(T const ct) {
  typename T::header_type;
  typename T::payload_type;
  typename T::buffer_type;
  typename T::size_type;

  { ct.header(lazy) } -> result_type<lazy_t, typename T::header_type>;
  { ct.header(eager) } -> result_type<eager_t, typename T::header_type>;
  { ct.header(in_place) } -> result_type<in_place_t, typename T::header_type>;
  // TOOO: payload getter

  { ct.header_span() } -> std::same_as<typename T::buffer_type>;
  { ct.payload_span() } -> std::same_as<typename T::buffer_type>;
  { ct.frame_length() } -> std::same_as<typename T::size_type>;
  { ct.header_length() } -> std::same_as<typename T::size_type>;
  { ct.payload_length() } -> std::same_as<typename T::size_type>;
  { ct.truncated() } -> std::same_as<typename T::size_type>;
  { ct.as_span() } -> std::same_as<typename T::buffer_type>;
  { ct.data() } -> std::same_as<typename T::buffer_type>;


  requires std::constructible_from<T, typename T::buffer_type>;
  requires frame_header<typename T::header_type>;
  requires frame_payload<typename T::payload_type>;
};

template<frame_header HeaderType, frame_payload PaylaodType>
class frame {
public:
  // --- Type traits ---

  using header_type  = HeaderType;
  using payload_type = PaylaodType;
  using buffer_type  = std::span<std::byte const>;
  using size_type    = size_type;

  // --- Constructors ---

  /**
   * @brief Construct a new frame object from a span of bytes
   *
   * Preconditions:
   *   - the span must be at least as large as the frame length
   *
   * The span might be larger than the actual frame length, in which case the extra bytes are ignored.
   * The frame length is determined by the header + payload size.
   *
   * Truncated frames are allowed, but will be indicated by the truncated() method.
   * Deserializing the payload of a truncated frame will result in undefined behavior.
   */
  constexpr explicit frame(buffer_type const data) : data_(data) { }

  // --- Member accessors ---

  [[nodiscard]] constexpr auto header(strategy auto strategy) const -> header_type {
    return rbe::deserialize<header_type>(header_span(), strategy);
  }

  [[nodiscard]] constexpr auto payload(strategy auto strategy) const
    requires(wirable<payload_type>)
  -> payload_type {
    return rbe::deserialize<payload_type>(payload_span(), strategy);
  }

  [[nodiscard]] constexpr auto payload() const
    requires(std::constructible_from<payload_type, buffer_type>)
  -> payload_type {
    return payload_type {payload_span()};
  }

  [[nodiscard]] constexpr auto payload() const
    requires(is_any<payload_type>)
  -> payload_type {
    // TODO: any construction from id and span
    return payload_type {payload_span()};
  }

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
  [[nodiscard]] constexpr auto as_span() const -> buffer_type { return data_.subspan(frame_length()); }

  /**
   * @brief Return a span of bytes representing the entire frame, including any padding
   * @return A span of bytes representing the entire frame, including any padding
   */
  [[nodiscard]] constexpr auto data() const -> buffer_type { return data_; }


private:
};


template<is_frame T, strategy S>
constexpr auto flatten(T const frame, S strategy) {
  auto get_payload = [&]() { // clang-format off
    if constexpr (wirable<typename T::payload_type>) { return frame.payload(strategy); }
    else { return frame.payload(); }
  }; // clang-format off

  if constexpr (is_frame<typename T::payload_type>) {
    return std::tuple_cat( //
        std::make_tuple(frame.header(strategy)), //
        flatten(get_payload(), strategy) //
    );
    else {
      return std::make_tuple(frame.header(strategy), get_payload());
    }
  }
}

} // namespace rbe::dsrl
