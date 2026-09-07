/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file proxy.hpp
 * @date 24/06/2026
 * @brief Lazy deserialization proxy providing field-by-field access into a byte buffer
 */

#pragma once

// --- Includes ---
#include <rbe/core/detail/context.hpp>
#include <rbe/core/detail/static_string.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/dsrl/detail/deserialize_impl.hpp>
#include <rbe/dsrl/detail/deserialize_member.hpp>

// --- STD ---

// --- System ---

namespace rbe::dsrl {

template<wirable T, rbe::detail::context Ctx = rbe::detail::context {}>
  requires(not custom_wirable<T>)
class proxy {
  static constexpr auto local = rbe::detail::merge_context(Ctx, ^^T);
  static constexpr auto wire  = get_wire_layout<T, local>();

public:
  // --- Type traits ---

  using value_type  = T;
  using size_type   = std::size_t;
  using buffer_type = std::span<std::byte const>;

  // --- Constructors ---

  constexpr explicit proxy(buffer_type const data) : data_(data) { }

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

  [[nodiscard]] constexpr auto value() const -> value_type {
    return rbe::detail::deserialize<value_type, local>(data_);
  }

  [[nodiscard]] constexpr auto length() const -> size_type { return wire_size_of<value_type, local>(); }

  [[nodiscard]] constexpr auto as_span() const -> buffer_type { return data_.first(length()); }

  [[nodiscard]] constexpr auto size() const -> size_type { return data_.size(); }

  [[nodiscard]] constexpr auto size_bytes() const -> size_type { return data_.size(); }

  [[nodiscard]] constexpr auto data() const -> buffer_type { return data_; }

private:
  std::span<std::byte const> data_;
};

} // namespace rbe::dsrl
