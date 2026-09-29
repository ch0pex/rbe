/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame_header.hpp
 * @date 28/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/annotation_concepts.hpp>
#include <rbe/annotations/id.hpp>
#include <rbe/annotations/length.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/dsrl/proxy.hpp>
#include <rbe/framing/frame_concepts.hpp>

// --- STD ---
#include <concepts>
#include <cstddef>
#include <optional>
#include <span>

namespace rbe::dsrl {

/**
 * @brief A frame header view: a proxy over T plus the framing lengths the wire carries
 *
 * The inheritance from proxy is private on purpose. header_proxy is implemented in terms of a proxy
 * but is not substitutable for one: proxy::as_span() is the *physical* extent of T -- size() bytes,
 * all that layer can know, since it has no notion of what the content means -- whereas
 * header_proxy::as_span() is the *logical* extent the wire declares, matching what frame, any and
 * many already return. Deriving publicly would let a header_proxy bind to a proxy const& and
 * silently answer with the wrong extent; and proxy is a non-polymorphic value type passed by value
 * throughout the library, so that slice is the ordinary way to use it, not a corner case.
 */
template<frame_header T>
class header_proxy : private proxy<T> {
  using base = proxy<T>;
  constexpr explicit header_proxy(base const b) : base(b) { }

public:
  // --- Type traits ---

  using value_type  = T;
  using buffer_type = std::span<std::byte const>;
  using size_type   = std::size_t;


  // --- Constants ---

  static constexpr bool has_header_length  = contains_annotation<T, rbe::header_length>;
  static constexpr bool has_payload_length = contains_annotation<T, rbe::payload_length>;
  static constexpr bool has_frame_length   = contains_annotation<T, rbe::frame_length>;
  static constexpr bool has_id             = contains_annotation<T, rbe::id>;

  // --- Factory static member function ---

  [[nodiscard]] static constexpr auto make(buffer_type const data) -> std::optional<header_proxy> {
    auto const hdr = base::make(data);
    if (not hdr) {
      return std::nullopt;
    }

    if constexpr (has_header_length) {
      auto logic_length = static_cast<size_type>(hdr->template field<rbe::header_length>());
      if (data.size() < logic_length) {
        return std::nullopt;
      }
    }
    return header_proxy {*hdr};
  }

  // --- Constructors ---

  using base::base;

  // --- Member functions ---

  // Everything that is purely about T's layout carries over unchanged so we
  // can directly expose proxy's interface for those members.

  using base::data;
  using base::field;
  using base::size; // physical size of T, not the logical length the wire declares
  using base::value;
  using base::operator*;

  [[nodiscard]] constexpr auto id() const
    requires(has_id)
  {
    return this->template field<rbe::id>();
  }

  // NOTE: header length doesn't have to be always the same
  // as size(). Header length mainly exists
  // to improve backwards compatibiility whenn adding new fields to a header
  // so the user could have a reduced version of the header meaning
  // that header_length (value comming through the wire) could be bigger than the
  // wire size of the header type (size()). is_extended() can be used to check
  // this condition, and extension() hands back the bytes T does not account for.
  [[nodiscard]] constexpr auto length() const -> size_type {
    if constexpr (has_header_length) {
      return this->template field<rbe::header_length>();
    }
    else {
      return this->size();
    }
  }

  // Payload length can be derived sorted by priority as follows:
  // - payload_length field if present
  // - frame_length - header_length if frame_length field is present

  [[nodiscard]] constexpr auto payload_length() const -> size_type
    requires(has_payload_length)
  {
    return this->template field<rbe::payload_length>();
  }

  [[nodiscard]] constexpr auto payload_length() const -> size_type
    requires(has_frame_length and not has_payload_length)
  {
    return this->frame_length() - this->length();
  }

  // Frame length can be derived sorted by priority as follows:
  // - frame_length field if present
  // - payload_length + header_length if payload_length field is present

  [[nodiscard]] constexpr auto frame_length() const -> size_type
    requires(has_frame_length)
  {
    return this->template field<rbe::frame_length>();
  }

  [[nodiscard]] constexpr auto frame_length() const -> size_type
    requires(has_payload_length and not has_frame_length)
  {
    return this->length() + payload_length();
  }

  [[nodiscard]] constexpr auto is_extended() const -> bool { return length() > this->size(); }

  // --- Spans ---
  //
  // Three distinct extents over the same data(): the logical one the wire declares, the physical one
  // T maps onto, and whatever the wire declared past the end of T.

  /// The header as the wire delimits it: what has to be skipped to reach the payload, and what has to
  /// be re-emitted to reproduce the header verbatim.
  /// precondition: the underlying buffer holds at least length() bytes -- guaranteed when the
  /// header_proxy came from make(), the caller's responsibility when it was built from a raw buffer.
  [[nodiscard]] constexpr auto logical_span() const -> buffer_type { return buffer_type {data(), length()}; }

  /// The bytes T maps onto, and therefore the only ones field() may read. Never longer than as_span().
  [[nodiscard]] constexpr auto physical_span() const -> buffer_type { return base::as_span(); }

  /// The trailing bytes the wire declared but T does not know how to decode -- fields appended by a
  /// newer version of the protocol. Empty unless is_extended().
  [[nodiscard]] constexpr auto extension() const -> buffer_type {
    return is_extended() ? logical_span().subspan(this->size()) : buffer_type {};
  }
};

} // namespace rbe::dsrl
