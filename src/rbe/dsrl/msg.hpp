/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file msg.hpp
 * @date 24/06/2026
 * @brief Lazy deserialization proxy providing field-by-field access into a byte buffer
 */

#pragma once

// --- Includes ---
#include <rbe/core/detail/static_string.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/core/wirable_concepts.hpp>

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
    return field<rbe::detail::nsdm_index(^^value_type, Name.get())>();
  }

  template<std::size_t Index>
  constexpr auto field() const {
    using member_type                   = [:type_of(rbe::detail::nsdm(^^value_type, Index)):];
    static constexpr auto member_layout = wire.members[Index];

    return rbe::detail::deserialize_member<member_type, member_layout.endianness>(
        data_.subspan<member_layout.offset.bytes, member_layout.size>()
    );
  }

private:
  std::span<std::byte const> data_;
};

} // namespace rbe::dsrl
