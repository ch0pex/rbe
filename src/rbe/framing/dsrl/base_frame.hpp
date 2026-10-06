/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file base_frame.hpp
 * @date 06/10/2026
 * @brief What every frame view shares: its header and payload, and the operations derived from them
 */

#pragma once

// --- Includes ---
#include <rbe/dsrl/tags.hpp>
#include <rbe/framing/dsrl/flatten.hpp>
#include <rbe/framing/dsrl/header.hpp>
#include <rbe/framing/dsrl/payload.hpp>
#include <rbe/framing/frame_concepts.hpp>

// --- STD ---
#include <cstddef>
#include <span>

namespace rbe::dsrl {

/**
 * @brief The view_interface of frames
 *
 * A derived class supplies the two primitives, `static make(buffer) -> std::optional<D>` and
 * `length() const`, and gets everything derived from them for free. base_frame deliberately has neither
 * primitive: leaving one out is a compile error in many<D> or dsrl::is_frame<D>, not an iteration that
 * silently advances by the base's length. There are no protected resolution helpers either -- a derived
 * class that wants the default resolution calls the same public building blocks dsrl::frame calls
 * (header<H>::make, payload_extent, make_payload), which serve a frame that holds a frame as a member
 * just as well as one that derives.
 *
 * The derived operations dispatch to the most derived type (deducing this), so as_span(), payload_span()
 * and flatten() follow a custom length().
 */
template<frame_header HeaderType, frame_payload PayloadType>
class base_frame {
public:
  // --- Type traits ---

  using header_type         = HeaderType;
  using payload_type        = PayloadType;
  using header_return_type  = rbe::dsrl::header<header_type>; // qualified: header() below would change its meaning
  using payload_return_type = payload_view_t<payload_type>;
  using buffer_type         = std::span<std::byte const>;
  using size_type           = std::size_t;

  // --- Member functions ---

  [[nodiscard]] constexpr auto header() const -> header_return_type { return header_; }

  [[nodiscard]] constexpr auto payload() const -> payload_return_type { return payload_; }

  [[nodiscard]] constexpr auto data() const -> std::byte const* { return header_.data(); }

  [[nodiscard]] constexpr auto buffer() const -> buffer_type { return header_.buffer(); }

  // --- Spans, all derived from length() ---

  [[nodiscard]] constexpr auto as_span(this auto const& self) -> buffer_type {
    return buffer_type {self.data(), self.length()};
  }

  [[nodiscard]] constexpr auto header_span() const -> buffer_type { return header_.as_span(); }

  [[nodiscard]] constexpr auto payload_span(this auto const& self) -> buffer_type {
    return self.as_span().subspan(self.header().length());
  }

  [[nodiscard]] constexpr auto flatten(this auto const& self, strategy auto strategy = lazy) {
    return rbe::dsrl::flatten(self, strategy);
  }

protected:
  constexpr base_frame(header_return_type const hdr, payload_return_type const payload) :
    header_(hdr), payload_(payload) { }

private:
  header_return_type header_;
  payload_return_type payload_;
};

} // namespace rbe::dsrl
