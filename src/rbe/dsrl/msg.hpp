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
#include <rbe/core/detail/context.hpp>
#include <rbe/core/detail/static_string.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/core/wirable_concepts.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---


namespace rbe::dsrl {

template<wirable T, rbe::detail::context Ctx>
  requires(not custom_wirable<T>)
class msg {
  // Resolved once, at construction type: T's own annotations override whatever ambient context was
  // inherited, so a msg<T> nested arbitrarily deep still propagates correctly instead of resetting.
  // NOTE: must reflect ^^T directly, not ^^value_type -- std::meta::annotations_of does not see
  // through a type alias to the annotations on the type it names.
  static constexpr auto local = rbe::detail::merge_context(Ctx, ^^T);
  static constexpr auto wire  = get_wire_layout<T, local>();

public:
  // --- Type traits ---

  using value_type = T;
  using size_type  = std::size_t;


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
    static constexpr auto member_ctx    = rbe::detail::merge_context(local, rbe::detail::nsdm(^^value_type, Index));

    return rbe::detail::deserialize_member<member_type, member_ctx>(
        data_.subspan<member_layout.offset.bytes, member_layout.size>()
    );
  }

private:
  std::span<std::byte const> data_;
};

} // namespace rbe::dsrl
