/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file msg.hpp
 * @date 24/06/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/concepts/wirable.hpp>
#include <rbe/core/endian.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/core/static_string.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---


namespace rbe::dsrl {

template<wirable T>
  requires(not custom_wirable<T>)
class msg {
public:
  // --- Type traits ---

  using value_type = T;
  using size_type  = std::size_t;

  static constexpr auto wire = get_wire_layout<T>();

  // --- Constructors ---

  constexpr explicit msg(std::span<std::byte const> const data) : data_(data) { }

  template<static_string Name>
  constexpr auto field() const {
    return field<detail::nsdm_index(^^value_type, Name.get())>();
  }

  template<std::size_t Index>
  constexpr auto field() const {
    using member_type                   = [:type_of(detail::nsdm(^^value_type, Index)):];
    static constexpr auto member_layout = wire.members[Index];
    auto const* ptr                     = std::addressof(data_[member_layout.offset.bytes]);
    return endian::load<member_type, member_layout.endianness>(ptr);
  }

private:
  std::span<std::byte const> data_;
};

} // namespace rbe::dsrl
