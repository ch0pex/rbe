/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file packet.hpp
 * @date 04/09/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/core/message_list.hpp>
#include <rbe/core/metadata_layout.hpp>
#include <rbe/dsrl/detail/annotated_field.hpp>

// --- STD ---
#include <concepts>
#include <tuple>
#include "rbe/annotations/metadata.hpp"
#include "rbe/dsrl/tags.hpp"


namespace rbe::dsrl {

template<typename T>
concept packet_header = wirable<T>;

template<typename T>
concept packet_payload = std::constructible_from<T, std::span<std::byte const>>;

template<typename T>
concept is_packet = requires(T const ct) {
  { ct.header(eager) } -> result_type<eager_t, T::header_type>;
  { ct.header(lazy) } -> result_type<lazy_t, T::header_type>;
  { ct.header(in_place) } -> result_type<in_place, T::header_type>;
  { ct.header(in_place_mut) } -> result_type<in_place_mut, T::header_type>;
  { ct.payload() } -> T::payload_type;
  { ct.size() } -> T::size_type;
  { ct.size_bytes() } -> T::size_type;
  { ct.as_span() } -> T::buffer_type;

  requires packet_header<T::header_type>;
  requires packet_payload<T::payload_type>;
};

template<is_packet T, strategy S>
constexpr auto flatten(T packet, S dsrl_strategy) {
  if constexpr (is_packet<T::payload_type>) {
    return std::tuple_cat(
        std::make_tuple(packet.header(dsrl_strategy)), //
        flatten(packet.payload(), dsrl_strategy) //
    );
  }
  else {
    return std::make_tuple(packet.header(dsrl_strategy), packet.payload());
  }
}

template<packet_header Hdr, packet_payload Payload>
class packet {
  static constexpr auto hdr_metadata = get_metadata_layout(^^Hdr);

public:
  using header_type  = Hdr;
  using payload_type = Payload;
  using buffer_type  = std::span<std::byte const>;
  using size_type    = std::size_t;

  constexpr packet(buffer_type const buffer) : data_(buffer);

  template<strategy S>
  constexpr auto header(S dsrl_strategy) -> result_type<S, header_type> {
    return deserialize<header_type>(data_.first<wire_size_of(^^Hdr)>(), dsrl_strategy);
  }

  constexpr auto payload() -> payload_type { return {payload_as_span()}; }

  constexpr auto flatten(strategy auto dsrl_strategy) const { return rbe::dsrl::flatten(*this, dsrl_strategy); }

  constexpr auto as_span() const -> buffer_type { return data_; }

  constexpr auto header_as_span() const -> buffer_type { return data_.first<wire_size_of(^^Hdr)>(); }

  constexpr auto payload_as_span() const -> buffer_type { return data_.subspan<wire_size_of(^^Hdr)>(); }

  constexpr auto size() const -> std::size_t { return data_.size(); }

  constexpr auto size_bytes() const -> std::size_t { return data_.size(); }

  // length annotation in header must express the size of the header + payload
  constexpr auto length() const -> size_type
    requires(hdr_metadata.length.has_value())
  {
    return rbe::detail::read_annotated_field<rbe::length, hdr_type>(header_as_span());
  }

  constexpr auto truncated() const -> bool
    requires(hdr_metadata.length.has_value())
  {
    return length() > size();
  }

private:
  buffer_type data_;
};


void foo() {

  for (auto [hdr, msg]: packet | rbe::views::messages(dsrl::eager, dsrl::lazy)) {
  }
}

} // namespace rbe::dsrl
