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
#include <rbe/core/message_concepts.hpp>
#include <rbe/core/metadata_layout.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/dsrl/detail/annotated_field.hpp>
#include <rbe/dsrl/detail/deserialize_member.hpp>
#include "rbe/annotations/metadata.hpp"

// --- STD ---
#include <algorithm>

// --- System ---


namespace rbe::dsrl {

template<wirable T, rbe::detail::context Ctx = rbe::detail::context {}>
  requires(not custom_wirable<T>)
class msg {
  static constexpr auto local = rbe::detail::merge_context(Ctx, ^^T);
  static constexpr auto wire  = get_wire_layout<T, local>();

public:
  // --- Type traits ---

  using value_type  = T;
  using size_type   = std::size_t;
  using buffer_type = std::span<std::byte const>;

  // --- Constructors ---

  constexpr explicit msg(buffer_type const data) : data_(data) { }

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

  [[nodiscard]] constexpr auto id() const
    requires(rbe::identificable<value_type>)
  {
    return rbe::detail::read_id_field<value_type, Ctx>(data_);
  }

  [[nodiscard]] constexpr auto length() const { return rbe::detail::read_length_field<value_type, Ctx>(data_); }

  /**
   * @brief Whether `length()` claims more bytes than the buffer actually holds
   *
   * A `length` field is only as trustworthy as the bytes it came from -- corruption, a truncated
   * capture, or a wire message this build doesn't know about can all make it lie. Checking this before
   * trusting `length()` (or the clamping already done by `as_span`/`remainder`) is how callers tell
   * "this message is short" apart from "this message is empty".
   *
   * @return `true` if `length()` exceeds the buffer's size, `false` otherwise
   */
  [[nodiscard]] constexpr auto truncated() const -> bool { return length() > data_.size(); }

  /**
   * @brief Exposes the underlying buffer trimmed to this message's own length
   * @return This message's bytes, sized to `length()`, clamped to the buffer's actual size if
   * `length()` claims more than is available (see `truncated()`)
   */
  [[nodiscard]] constexpr auto as_span() const -> buffer_type {
    return data_.first(std::min<std::size_t>(length(), data_.size()));
  }

  /**
   * @brief Exposes the underlying buffer as-is, without trimming it to this message's own length
   * @return The raw buffer this proxy was constructed with, which may extend past this message
   */
  [[nodiscard]] constexpr auto data() const -> buffer_type { return data_; }

  /**
   * @brief Exposes the bytes left over past this message's own length
   * @return The buffer's bytes past `length()`, or an empty span if the buffer holds none or
   * `length()` claims more than is available (see `truncated()`)
   */
  [[nodiscard]] constexpr auto remainder() const -> buffer_type {
    return data_.subspan(std::min<std::size_t>(length(), data_.size()));
  }

private:
  buffer_type data_;
};

} // namespace rbe::dsrl
