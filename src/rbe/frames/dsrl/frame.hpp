/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file frame.hpp
 * @date 08/09/2026
 * @brief The default frame view: base_frame plus the default make() and length()
 */

#pragma once

// --- Includes ---
#include <rbe/frames/dsrl/base_frame.hpp>
#include <rbe/frames/dsrl/detail/payload_size.hpp>
#include <rbe/frames/dsrl/payload.hpp>
#include <rbe/frames/frame_concepts.hpp>
#include <rbe/frames/frame_delimiting_concepts.hpp>

// --- STD ---
#include <cstddef>
#include <optional>
#include <span>

namespace rbe::frames::dsrl {

/**
 * @brief A frame view over a header and the payload it describes
 *
 * frame adds to base_frame the two primitives in their default form. make() is the canonical composition
 * of the three building blocks -- locate the payload (header<H>::make), settle its extent
 * (try_payload_extent) and build the view (try_construct_payload) -- and length() follows the same precedence as
 * the extent: a header that declares a length is the authority, otherwise header plus payload.
 */
template<frames::header HeaderType, frames::payload PayloadType>
class frame : public base_frame<HeaderType, PayloadType> {
  using base = base_frame<HeaderType, PayloadType>;

public:
  // --- Type traits ---

  /// The delimiting category, deduced from the shape. A frame that swaps the extent out builds on
  /// base_frame instead, which declares none, and states its own (see cboe::top::line_view).
  using delimiting_concept = frames::frame_delimiting_tag<HeaderType, PayloadType>;
  using typename base::buffer_type;
  using typename base::header_return_type;
  using typename base::header_type;
  using typename base::payload_return_type;
  using typename base::payload_type;
  using typename base::size_type;

  // --- Factory static member function ---

  [[nodiscard]] static constexpr auto make(buffer_type const data) -> std::optional<frame> {
    // safely constructs the header
    auto const hdr = header_return_type::make(data);
    if (not hdr.has_value()) {
      return std::nullopt;
    }

    // trims the paylaod to the extent the header declares
    auto const payload_extent = try_payload_extent<payload_type>(*hdr, data.subspan(hdr->length()));
    if (not payload_extent.has_value()) {
      return std::nullopt;
    }

    // builds the payload
    return try_construct_payload<payload_type>(*hdr, *payload_extent).transform([&](payload_return_type const payload) {
      return frame {*hdr, payload};
    });
  }

  // --- Constructors ---

  /// precondition: data holds the whole frame, i.e. make(data) would succeed
  constexpr explicit frame(buffer_type const data) : frame(resolve(data)) { }

  // --- Member functions ---

  /// Only for a frame whose category chain resolves in O(1): everything else answers through
  /// frames::distance, and has no length() for a caller to reach for by mistake (see frames::length).
  ///
  /// The shape is not the whole condition: a nested frame has to be able to answer too, and a
  /// hand-written one may have declared a category while supplying no length() at all. Asking its member
  /// is what keeps the two halves of the axis recursing by the same rule, so this exists only when the
  /// body below actually compiles.
  [[nodiscard]] constexpr auto length() const -> size_type
    requires(frames::detail::is_constant_length<frame>() and
             (not frames::is_frame<PayloadType> or frames::detail::member_length<PayloadType>))
  {
    if constexpr (frames::buffer_delimited<frame>) {
      return this->buffer().size();
    }
    else if constexpr (frames::length_delimited<frame>) {
      return this->header().frame_length();
    }
    // the count bounds the run without reading an element, so the payload view is never asked (a `many`
    // reports no extent of its own; what it was handed is this, settled in step 2 by counted_extent)
    else if constexpr (frames::static_stride_delimited<frame>) {
      return this->header().length() + counted_extent<payload_type>(this->header());
    }
    else {
      return this->header().length() + detail::payload_length(this->payload());
    }
  }

private:
  constexpr frame(header_return_type const hdr, payload_return_type const payload) : base(hdr, payload) { }

  /// The narrow counterpart of make(): the same three steps, assuming their preconditions instead of checking them
  [[nodiscard]] static constexpr auto resolve(buffer_type const data) -> frame {
    auto const hdr   = header_return_type {data};
    auto const bytes = payload_extent<payload_type>(hdr, data.subspan(hdr.length()));
    return frame {hdr, construct_payload<payload_type>(hdr, bytes)};
  }
};

} // namespace rbe::frames::dsrl
