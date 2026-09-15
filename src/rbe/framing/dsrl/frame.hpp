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
#include <rbe/annotations/id.hpp>
#include <rbe/annotations/length.hpp>
#include <rbe/annotations/well_annotated_concepts.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/dsrl/deserialize.hpp>
#include <rbe/dsrl/tags.hpp>
#include <rbe/framing/detail/payload_extent.hpp>
#include <rbe/framing/dsrl/flatten.hpp>

// --- STD ---
#include <cstddef>
#include <span>

namespace rbe::dsrl {

template<frame_header HeaderType, frame_payload PaylaodType>
// TODO: compatibility between header and payload
class frame {
public:
  // --- Type traits ---

  using header_type  = HeaderType;
  using payload_type = PaylaodType;
  using buffer_type  = std::span<std::byte const>;
  using size_type    = std::size_t;

  // --- static constants ---
  /**
   * @brief The fixed header length in bytes, as computed by rbe::wire_size_of<header_type>()
   */
  static constexpr auto fixed_header_length = wire_size_of<header_type>();

  // --- Constructors ---

  /**
   * @brief Construct a new frame object from a span of bytes
   *
   * The frame is a non-owning view. Construction resolves the frame's layout, not its values: it computes
   * length_of(data) and narrows the span to exactly the frame, while header and payload values are still
   * decoded lazily by the accessors. How the span size is interpreted depends on how the frame is delimited,
   * which is classified at compile time by rbe::detail::payload_extent_of() (lowering an rbe::frame never
   * changes it):
   *
   *  - self-delimiting frames (rbe::self_delimiting_frame) know their length from a header field or from
   *    static sizes. The span may be larger than the frame (e.g. a whole datagram, or the remainder of a
   *    stream while iterating): trailing bytes are not kept.
   *  - buffer-delimited frames (rbe::buffer_delimited_frame) have a payload that extends to the end of the
   *    span, so the span size *is* the frame length: trailing bytes are taken as payload, never as padding.
   *
   *  - explicitly delimited frames are preferable for multiple reasons:
   *    - performance, reading an explicit frame_length or payload_length + header_length is cheap
   *    - frames can be skipped while iterating
   *  - implicitly delimited frames might have some problems:
   *    - if payload type is any, a dispatching is needed on construction to know compute header_length + frame_length
   *    - if the id is unknown iteration cannot skip the frame
   *
   * To find out how many bytes a partially received frame needs before constructing it, use length_of().
   *
   * Preconditions:
   *   - data.size() >= length_of(data)
   *   - buffer-delimited frames: the span covers exactly the frame
   *   - annotated lengths are consistent: wire_size_of<header_type>() <= header_length <= frame_length
   *  Under these preconditions length_of(data) is guaranteed to return a value
   */
  constexpr explicit frame(buffer_type const data) : data_(data.first(*length_of(data))) { }

  // struct resolved_type { };
  //
  // constexpr explicit frame(resolved_type /**/, buffer_type const data)
  //   requires(is_any<payload_type>)
  //   : data_(data) { }

  // --- Member accessors ---

  template<strategy S = lazy_t>
  [[nodiscard]] constexpr auto header(S strategy = lazy) const -> return_type<S, header_type> {
    return rbe::deserialize<header_type>(header_span(), strategy);
  }

  template<strategy S = lazy_t>
    requires(wirable<payload_type>)
  [[nodiscard]] constexpr auto payload(S strategy = lazy) const -> return_type<S, payload_type> {
    return rbe::deserialize<payload_type>(payload_span(), strategy);
  }

  [[nodiscard]] constexpr auto payload() const -> payload_type
    requires(std::constructible_from<payload_type, buffer_type>)
  {
    return payload_type {payload_span()};
  }

  [[nodiscard]] constexpr auto flatten(strategy auto strategy = lazy) { return flatten(*this, strategy); }

  // --- Size accessors ---

  /**
   * @brief Return the length of the frame in bytes
   *
   * Resolved once at construction by length_of(), so it is the size of the viewed span.
   *
   * @return The length of the frame in bytes
   */
  [[nodiscard]] constexpr auto length() const -> size_type { return data_.size(); }

  /**
   * @brief Return the length of the header in bytes
   *
   * header_length can either be gathered from:
   *  - header_length annotated field value if specified
   *  - otherwise from rbe::wire_size_of<header_type>() (variable-size header would be malformed)
   *
   * @return The length of the header in bytes
   */
  [[nodiscard]] constexpr auto header_length() const -> size_type { return header_length_of(data_); }

  /**
   * @brief Return the length of the payload in bytes
   *
   * Derived from the frame length resolved at construction: frame_length = header_length + payload_length.
   *
   * @return The length of the payload in bytes
   */
  [[nodiscard]] constexpr auto payload_length() const -> size_type { return length() - header_length(); }

  /**
   * @brief Resolve the length of the frame starting at `data`, without constructing it
   *
   * Usable over a partially received buffer, e.g. to know how many bytes to wait for while reassembling a
   * stream: it only reads the bytes the length depends on, which are the fixed-size header prefix for length
   * fields and static sizes, plus the nested frames' headers when the payload is a nested frame.
   *
   * The length is gathered from (wire values take precedence over static sizes, see
   * rbe::detail::payload_extent_of()):
   *  - header length + the payload_length annotated field value if specified
   *  - otherwise the frame_length annotated field value if specified
   *  - otherwise header length + the nested frame's own length_of() if the payload is a frame
   *  - otherwise header length + rbe::wire_size_of<payload_type>() if the payload is wirable
   *  - otherwise (span constructible, any, many) the whole buffer, which makes the frame buffer-delimited
   *
   * Length fields are read over the fixed-size header prefix, never over a span that depends on a length.
   *
   * Preconditions:
   *   - data.size() >= rbe::wire_size_of<header_type>()
   *
   * @return The length of the frame in bytes
   */
  [[nodiscard]] static constexpr auto length_of(buffer_type const data) -> std::optional<size_type> {
    static constexpr auto extent = rbe::detail::payload_extent_of<header_type, payload_type>();
    if (data.size() < rbe::wire_size_of<header_type>()) {
      return std::nullopt;
    }

    if constexpr (extent == rbe::detail::payload_extent::payload_length_field) {
      return header_length_of(data) + fixed_header(data).template field<rbe::payload_length>();
    }
    else if constexpr (extent == rbe::detail::payload_extent::frame_length_field) {
      return fixed_header(data).template field<rbe::frame_length>();
    }
    else if constexpr (extent == rbe::detail::payload_extent::nested_frame) {
      auto const header_length = header_length_of(data);
      return header_length + *payload_type::length_of(data.subspan(header_length));
    }
    else if constexpr (extent == rbe::detail::payload_extent::static_size) {
      return header_length_of(data) + wire_size_of<payload_type>();
    }
    else {
      return data.size();
    }
  }

  // --- Buffer accessors ---

  /**
   * @brief Return a span of bytes representing the header portion of the frame
   *
   * Some protocols may specify a header length to avoid breaking compatibility when adding new fields,
   * so the header may be larger than the wire size of the header type.
   *
   * @return A span of bytes representing the header portion of the frame
   */
  [[nodiscard]] constexpr auto header_span() const -> buffer_type { return data_.first(header_length()); }

  /**
   * @brief Return a span of bytes representing the payload portion of the frame
   * @return A span of bytes representing the payload portion of the frame
   */
  [[nodiscard]] constexpr auto payload_span() const -> buffer_type { return data_.subspan(header_length()); }

  /**
   * @brief Return a span of bytes representing the entire frame
   * @return A span of bytes representing the entire frame
   */
  [[nodiscard]] constexpr auto as_span() const -> buffer_type { return data_; }

  [[nodiscard]] constexpr auto data() const -> std::byte const* { return data_.data(); }

private:
  // the length fields are read over the fixed-size header prefix, which never depends on a length itself
  [[nodiscard]] static constexpr auto fixed_header(buffer_type const data) {
    return rbe::deserialize<header_type>(data.first(rbe::wire_size_of<header_type>()), lazy);
  }

  [[nodiscard]] static constexpr auto header_length_of(buffer_type const data) -> size_type {
    if constexpr (contains_annotation<header_type, rbe::header_length>) {
      return fixed_header(data).template field<rbe::header_length>();
    }
    else {
      return wire_size_of<header_type>();
    }
  }

  buffer_type data_;
};

} // namespace rbe::dsrl
