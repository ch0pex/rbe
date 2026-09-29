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
#include <rbe/framing/dsrl/detail/payload_size.hpp>
#include <rbe/framing/dsrl/frame_header.hpp>
#include <rbe/framing/frame_concepts.hpp>
#include <rbe/framing/frame_delimiting_concepts.hpp>


// --- STD ---
#include <cstddef>
#include <span>

namespace rbe::dsrl {

namespace detail {

template<frame_payload T>
struct normalize_payload {
  using type = T;
};

template<frame_payload T>
  requires(frame_wirable<T>)
struct normalize_payload<T> {
  using type = proxy<T>;
};

template<frame_payload T>
using normalize_payload_t = typename normalize_payload<T>::type;

} // namespace detail


// TODO: compatible header and payload types concept
// this will bring an awful error message if the header
// does not have an id field and the payload type requires it
//  TODO: rbe::exact_length constructor overload
// Payload concept
// - needs make as constexpr static member function
// - needs length() as constexpr non static member function
// - constructible from std::span<std::byte const> (or a compatible type)
template<frame_header HeaderType, frame_payload PaylaodType>
class frame {
public:
  // --- Type traits ---

  using header_type         = HeaderType;
  using payload_type        = PaylaodType;
  using header_proxy_type   = header_proxy<header_type>;
  using payload_return_type = detail::normalize_payload_t<payload_type>;
  using buffer_type         = std::span<std::byte const>;
  using size_type           = std::size_t;

  // --- Factory static member function ---

  [[nodiscard]] static constexpr auto make(buffer_type const data) -> std::optional<frame> {
    auto const hdr = header_proxy_type::make(data);
    if (not hdr.has_value() or data.size() < hdr->length()) {
      return std::nullopt;
    }

    if constexpr (explicitly_delimited_frame<frame>) {
      // Fast path: if the current frame is explicitly delimited we can skip bound checking
      // for payload construction, as the payload is guaranteed to fit in the buffer.
      return data.size() >= hdr->frame_length() //
                 ? std::optional {frame {*hdr, frame::construct_payload(hdr.value(), data)}}
                 : std::nullopt;
    }
    else {
      auto const payload = frame::construct_payload_hardened(hdr.value(), data);
      return payload.transform([&](auto const p) { return frame {*hdr, p}; });
    }
  }

  // --- Constructors ---

  constexpr explicit frame(buffer_type const data) :
    header_(data), payload_(frame::construct_payload(header_, data)) { }

  [[nodiscard]] constexpr auto header() const -> header_proxy_type { return header_; }

  [[nodiscard]] constexpr auto payload() const -> payload_return_type { return payload_; }

  [[nodiscard]] constexpr auto flatten(strategy auto strategy = lazy) { return flatten(*this, strategy); }

  [[nodiscard]] constexpr auto length() const -> size_type {
    if constexpr (explicitly_delimited_frame<frame>) {
      assert(header_.frame_length() == payload_length(payload_) + header_.length());
      return header_.frame_length();
    }
    return header_.length() + payload_length(payload_);
  }

  [[nodiscard]] constexpr auto as_span() const -> buffer_type { return std::span {header_.data(), length()}; }

  [[nodiscard]] constexpr auto data() const -> std::byte const* { return header_.data(); }

private:
  // The payload type is wrapped in a proxy if it is wirable, otherwise it is stored as-is.

  constexpr frame(header_proxy_type const hdr, payload_return_type const payload) : header_(hdr), payload_(payload) { }

  // When the header declares a payload_length or a frame_length, that field is the one authoritative
  // source for the payload's extent, whatever category the payload falls into -- a wirable proxy or
  // an any narrow themselves anyway, a nested frame is self-delimiting regardless, and a blob or other
  // opaque span-constructible payload has no other way to learn where it ends.
  [[nodiscard]] static constexpr auto narrow_to_payload(header_proxy_type const hdr, buffer_type const data)
      -> buffer_type {
    auto const rest = data.subspan(hdr.length());
    if constexpr (header_proxy_type::has_payload_length or header_proxy_type::has_frame_length) {
      return rest.first(hdr.payload_length());
    }
    else {
      return rest;
    }
  }

  [[nodiscard]] static constexpr auto construct_payload(header_proxy_type const hdr, buffer_type const data)
      -> payload_return_type //
  {
    auto const payload_span = narrow_to_payload(hdr, data);
    if constexpr (is_any<payload_return_type>) {
      return payload_return_type {hdr.id(), payload_span};
    }
    else {
      return payload_return_type {payload_span};
    }
  }

  [[nodiscard]] static constexpr auto construct_payload_hardened(header_proxy_type const hdr, buffer_type const data)
      -> std::optional<payload_return_type> //
  {
    auto const payload_span = narrow_to_payload(hdr, data);
    if constexpr (is_any<payload_return_type>) {
      return payload_return_type::make(hdr.id(), payload_span);
    }
    else {
      return payload_return_type::make(payload_span);
    }
  }

  header_proxy_type header_;
  payload_return_type payload_;
};

} // namespace rbe::dsrl
