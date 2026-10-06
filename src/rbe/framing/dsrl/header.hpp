/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file header.hpp
 * @date 05/10/2026
 * @brief A frame header view: a proxy over H plus the lengths the wire declares
 */

#pragma once

// --- Includes ---
#include <rbe/dsrl/proxy.hpp>

// --- STD ---

namespace rbe::dsrl {

template<frame_header T>
class header : private rbe::dsrl::proxy<T> {
  using base = proxy<T>;
  constexpr explicit header(base const b) : base(b) { }

public:
  // --- Type traits ---

  using value_type  = base::value_type;
  using buffer_type = base::buffer_type;
  using size_type   = base::size_type;

  // --- Constants ---

  static constexpr bool has_header_length  = contains_annotation<T, rbe::header_length>;
  static constexpr bool has_payload_length = contains_annotation<T, rbe::payload_length>;
  static constexpr bool has_frame_length   = contains_annotation<T, rbe::frame_length>;
  static constexpr bool has_id             = contains_annotation<T, rbe::id>;

  /// Whether this header settles where its payload ends, through payload_length or frame_length. When it
  /// does, the field is the one authoritative source for the payload's extent, whatever the payload is.
  static constexpr bool delimits_payload = has_payload_length or has_frame_length;

  // --- Factory static member function ---

  [[nodiscard]] static constexpr auto make(buffer_type const data) -> std::optional<header> {
    auto const hdr = base::make(data);
    if (not hdr) {
      return std::nullopt;
    }

    if constexpr (has_header_length) {
      auto declared_length = static_cast<size_type>(hdr->template field<rbe::header_length>());
      if (data.size() < declared_length) {
        return std::nullopt;
      }
    }
    return header {*hdr};
  }

  // --- Constructors ---

  using base::base;

  // --- Member functions ---

  // Everything that is purely about T's layout carries over unchanged so we
  // can directly expose proxy's interface for those members.

  using base::buffer; // the buffer this view was handed, untrimmed -- not an extent of anything
  using base::data;
  using base::field;
  using base::size; // size of T, known to the code, not the length the wire declares
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
  // size of the header type (size(), known to the code). is_extended() can be used to check
  // this condition, and extension_span() hands back the bytes T does not account for.
  // The behaivour is undefined if the length the wire declares is smaller than the size of T (size()).
  [[nodiscard]] constexpr auto length() const -> size_type {
    if constexpr (has_header_length) {
      assert(this->template field<rbe::header_length>() >= this->size());
      return this->template field<rbe::header_length>();
    }
    else {
      return this->size();
    }
  }

  [[nodiscard]] constexpr auto is_extended() const -> bool { return length() > this->size(); }

  // Payload length can be derived sorted by priority as follows:
  // - payload_length field if present
  // - frame_length - header_length if frame_length field is present
  // if this value is wrong the framing library behaivour is undefined
  [[nodiscard]] constexpr auto payload_length() const -> size_type
    requires(has_payload_length)
  {
    return this->template field<rbe::payload_length>();
  }

  // NOTE: same with payload_length as with header_length
  // the payload length the wire declares might differ from the payload size
  // if the payload is a wirable_class.
  // The behaivour is undefined if the payload is a wirable_class
  // and the payload_length is smaller than the size of the payload type.
  [[nodiscard]] constexpr auto payload_length() const -> size_type
    requires(has_frame_length and not has_payload_length)
  {
    assert(this->frame_length() >= this->length());
    return this->frame_length() - this->length();
  }

  // Frame length can be derived sorted by priority as follows:
  // - frame_length field if present
  // - payload_length + header_length if payload_length field is present
  // if this value is wrong the framing library behaivour is undefined
  [[nodiscard]] constexpr auto frame_length() const -> size_type
    requires(has_frame_length)
  {
    assert(this->template field<rbe::frame_length>() >= this->length());
    return this->template field<rbe::frame_length>();
  }

  [[nodiscard]] constexpr auto frame_length() const -> size_type
    requires(has_payload_length and not has_frame_length)
  {
    return this->length() + payload_length();
  }

  // --- Spans ---

  // Three extents over the same data(): the one the wire delimits, the part of it T knows how to
  // decode, and the part it does not. as_span() == known_span() + extension_span().

  /// The header as the wire delimits it: what has to be skipped to reach the payload, and what has to
  /// be re-emitted to reproduce the header verbatim.
  /// precondition: the underlying buffer holds at least length() bytes -- guaranteed when the header
  /// came from make(), the caller's responsibility when it was built from a raw buffer.
  [[nodiscard]] constexpr auto as_span() const -> buffer_type { return buffer_type {this->data(), length()}; }

  /// The bytes T declares fields for, and therefore the only ones field() may read. Never longer than
  /// as_span().
  [[nodiscard]] constexpr auto known_span() const -> buffer_type { return base::as_span(); }

  /// The trailing bytes the wire declared but T has no field for, so there is no way to decode them --
  /// appended by a newer version of the protocol. Empty unless is_extended().
  [[nodiscard]] constexpr auto extension_span() const -> buffer_type {
    return is_extended() ? as_span().subspan(this->size()) : buffer_type {};
  }
};

} // namespace rbe::dsrl
