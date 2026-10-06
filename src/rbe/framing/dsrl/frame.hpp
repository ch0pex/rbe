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
#include <rbe/framing/dsrl/base_frame.hpp>
#include <rbe/framing/dsrl/detail/payload_size.hpp>
#include <rbe/framing/dsrl/payload.hpp>
#include <rbe/framing/frame_concepts.hpp>
#include <rbe/framing/frame_delimiting_concepts.hpp>

// --- STD ---
#include <cstddef>
#include <optional>
#include <span>

namespace rbe::dsrl {

/**
 * @brief A frame view over a header and the payload it describes
 *
 * frame adds to base_frame the two primitives in their default form. make() is the canonical composition
 * of the three building blocks -- locate the payload (header<H>::make), settle its extent
 * (try_payload_extent) and build the view (make_payload) -- and length() follows the same precedence as
 * the extent: a header that declares a length is the authority, otherwise header plus payload.
 */
template<frame_header HeaderType, frame_payload PayloadType>
class frame : public base_frame<HeaderType, PayloadType> {
  using base = base_frame<HeaderType, PayloadType>;

public:
  // --- Type traits ---

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
    return make_payload<payload_type>(*hdr, *payload_extent).transform([&](payload_return_type const payload) {
      return frame {*hdr, payload};
    });
  }

  // --- Constructors ---

  /// precondition: data holds the whole frame, i.e. make(data) would succeed
  constexpr explicit frame(buffer_type const data) : frame(resolve(data)) { }

  // --- Member functions ---

  [[nodiscard]] constexpr auto length() const -> size_type {
    if constexpr (header_return_type::delimits_payload) {
      return this->header().frame_length();
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

} // namespace rbe::dsrl
