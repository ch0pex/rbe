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
#include <rbe/frames/dsrl/flatten.hpp>
#include <rbe/frames/dsrl/header.hpp>
#include <rbe/frames/dsrl/payload.hpp>
#include <rbe/frames/frame_concepts.hpp>
#include <rbe/frames/length.hpp>

// --- STD ---
#include <cstddef>
#include <span>

namespace rbe::frames::dsrl {

// NOTE: the payload view is built eagerly -- in make() and in the narrow constructor -- and stored, so a nested
// frame resolves the whole chain down to the innermost payload at construction. Two reasons, in the order that
// actually decides it:
//
//   1. In a wide-contract make(), constructing the payload *is* the precondition check: a proxy verifies that T's
//      wire size fits, an any that the candidate its id selects fits, a nested frame recurses through its own
//      make(). There is no cheaper oracle for "do these bytes hold this payload", and a separate validation pass
//      would have to repeat exactly what those constructors already do.
//   2. It resolves once what a payload rebuilt on access would resolve per call. This only pays off for the frames
//      whose length comes from the payload rather than from a header field: a dispatch-delimited frame costs an id
//      lookup per length(), and length() is what as_span(), payload_span() and many's iteration all end up
//      calling. This was taken to be the main reason and it is not -- the win is narrower than expected (cboe TOP
//      looked like an instance of it and turned out to be terminator-delimited, not id-delimited).
//
// The price is size, and work a caller may never use:
//
//   - sizeof is the header view -- the buffer span, 16 bytes -- plus the payload view, so it follows the
//     composition of the frame instead of staying at 16: a many is 40 bytes, a frame of many 56, a frame of a frame
//     of many 72. Frames travel by value while iterating (many::current(), operator*), so this is per element.
//   - the whole nested chain is built even when the caller only reads a header field.
//
// What the alternatives really trade is not eager against lazy -- make() has to construct either way, per (1) --
// but how much of what make() built survives it:
//
//   - Keep nothing: make() checks its preconditions and throws the result away, the narrow constructor just keeps
//     the span, and payload() rebuilds on every call. sizeof returns to a constant 16 bytes and nothing is paid up
//     front, at the price of rebuilding on every length() too, which for an implicitly delimited chain means one
//     level rebuilt per level, on each call.
//   - Keep the extent make() resolved, one size_type: sizeof is a constant 24 bytes, length() stays O(1) for every
//     delimiting category with no re-dispatch, and only payload() rebuilds -- free for a proxy, a span or a many,
//     one id lookup again for an any.
//   - Keep the view, which is what this does: nothing is ever resolved twice, and sizeof grows with the nesting.
//
// Keeping the view is the decision: it is the only one that does not get more expensive when variable-size
// payloads arrive, and the only one that does not put a third construction protocol on every payload
// category. The measurements behind it, and the one refinement still open (caching the frame length), are
// in docs/explanation/frame-storage.md.
//

/**
 * @brief The view_interface of frames
 *
 * A derived class supplies the two primitives, `static make(buffer) -> std::optional<D>` and
 * `length() const`, and gets everything derived from them for free. base_frame deliberately has neither
 * primitive: leaving one out is a compile error in many<D> or dsrl::is_frame<D>, not an iteration that
 * silently advances by the base's length. There are no protected resolution helpers either -- a derived
 * class that wants the default resolution calls the same public building blocks dsrl::frame calls
 * (header<H>::make, payload_extent, try_construct_payload), which serve a frame that holds a frame as a member
 * just as well as one that derives.
 *
 * The derived operations dispatch to the most derived type (deducing this), so as_span(), payload_span()
 * and flatten() follow a custom length().
 */
template<frames::header HeaderType, frames::payload PayloadType>
class base_frame {
public:
  // --- Type traits ---

  using header_type  = HeaderType;
  using payload_type = PayloadType;
  using header_return_type =
      rbe::frames::dsrl::header<header_type>; // qualified: header() below would change its meaning
  using payload_return_type = payload_view_t<payload_type>;
  using buffer_type         = std::span<std::byte const>;
  using size_type           = std::size_t;

  // --- Member functions ---

  [[nodiscard]] constexpr auto header() const -> header_return_type { return header_; }

  [[nodiscard]] constexpr auto payload() const -> payload_return_type { return payload_; }

  /// Tuple-like access so a frame decomposes into its two parts: `auto [header, payload] = frame`
  template<std::size_t I>
    requires(I < 2)
  [[nodiscard]] constexpr auto get() const -> std::conditional_t<I == 0, header_return_type, payload_return_type> {
    if constexpr (I == 0) {
      return header();
    }
    else {
      return payload();
    }
  }

  [[nodiscard]] constexpr auto flatten(this auto const& self) { return rbe::frames::dsrl::flatten(self); }

  template<typename Self>
    requires frames::constant_length<Self>
  [[nodiscard]] constexpr auto as_span(this Self const& self) -> buffer_type {
    return buffer_type {self.data(), self.length()};
  }

  [[nodiscard]] constexpr auto header_span() const -> buffer_type { return header_.as_span(); }

  template<typename Self>
    requires frames::constant_length<Self>
  [[nodiscard]] constexpr auto payload_span(this Self const& self) -> buffer_type {
    return self.as_span().subspan(self.header().length());
  }

  [[nodiscard]] constexpr auto data() const -> std::byte const* { return header_.data(); }

  [[nodiscard]] constexpr auto buffer() const -> buffer_type { return header_.buffer(); }

protected:
  constexpr base_frame(header_return_type const hdr, payload_return_type const payload) :
    header_(hdr), payload_(payload) { }

private:
  header_return_type header_;
  payload_return_type payload_;
};

} // namespace rbe::frames::dsrl

// A frame is a pair of header and payload, which is what makes `for (auto [header, payload] : many)` work. Any
// frame qualifies, the library's or a user's, as long as it provides get<0> (header) and get<1> (payload): base_frame
// does, a frame of the user that does not derive from it supplies its own.
template<rbe::frames::dsrl::decomposable_frame T>
struct std::tuple_size<T> : std::integral_constant<std::size_t, 2> { };

template<std::size_t I, rbe::frames::dsrl::decomposable_frame T>
  requires(I < 2)
struct std::tuple_element<I, T> {
  using type = std::conditional_t<I == 0, typename T::header_return_type, typename T::payload_return_type>;
};
