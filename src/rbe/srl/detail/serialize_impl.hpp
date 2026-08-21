/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file serialize_impl.hpp
 * @date 02/07/2026
 * @brief Context-aware dispatch implementing serialization for wirable types.
 *
 * Internal machinery: fast-path/custom/primitive/aggregate/range overloads threading a `context`
 * through recursive calls so annotations propagate correctly through arbitrarily deep nesting. The
 * public entry point (`rbe::serialize`, in `rbe/srl/serialize.hpp`) always starts from the default
 * context and dispatches here.
 */

#pragma once

// --- Includes ---
#include <rbe/core/custom.hpp>
#include <rbe/core/detail/context.hpp>
#include <rbe/core/detail/memcpy_constexpr.hpp>
#include <rbe/core/detail/normalize.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/core/trivially_wirable_concepts.hpp>
#include <rbe/core/wirable_concepts.hpp>

// --- STD ---
#include <cstddef>

namespace rbe::detail {

// Forward declarations so every overload below can recurse into any sibling regardless of
// definition order -- e.g. an aggregate containing an array member needs to see the range
// overload, and a range of aggregates needs to see the aggregate overload right back.

template<trivially_wirable T, context Ctx = context {}>
  requires(Ctx == context {})
constexpr auto serialize(std::span<std::byte>, T const&) -> std::size_t;

template<custom_wirable T, context Ctx = context {}>
constexpr auto serialize(std::span<std::byte>, T const&) -> std::size_t;

template<trivially_wirable_primitive T, context Ctx>
  requires(Ctx != context {})
constexpr auto serialize(std::span<std::byte>, T const&) -> std::size_t;

template<wirable_class T, context Ctx = context {}>
  requires(not custom_wirable<T> and not wirable_range<T> and (not trivially_wirable<T> or Ctx != context {}))
constexpr auto serialize(std::span<std::byte>, T const&) -> std::size_t;

template<wirable_range T, context Ctx = context {}>
  requires(not trivially_wirable_range<T> or Ctx != context {})
constexpr auto serialize(std::span<std::byte>, T const&) -> std::size_t;

/// Fast path: nothing has forced a non-default context onto this member, so it's safe to serialize
/// with a single direct memory copy -- exactly today's behavior/optimization, unchanged.
template<trivially_wirable T, context Ctx>
  requires(Ctx == context {})
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  memcpy_constexpr(out, value);
  return sizeof(value);
}

/// Serializes a custom-wirable type via its `custom<T>::serialize` specialization. The wire format is
/// entirely user-defined, so the ambient context never applies to it.
template<custom_wirable T, context Ctx>
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  return rbe::custom<std::remove_cvref_t<decltype(value)>>::serialize(out, value);
}

/// A primitive that would otherwise be memcpy-able alone, but an ancestor's annotation forces a
/// specific byte order onto it -- write it with that order applied explicitly.
template<trivially_wirable_primitive T, context Ctx>
  requires(Ctx != context {})
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  memcpy_constexpr(out, normalize_endianness<T, Ctx.endianness>(value));
  return sizeof(value);
}

/**
 * @brief Serializes a wirable aggregate into a buffer member-by-member.
 *
 * Iterates over each non-static data member, threading the resolved ambient context one hop further
 * down at each member (`merge_context`) so annotations propagate correctly through arbitrarily deep
 * unannotated nesting, and recursively serializes it into the corresponding offset within the output
 * buffer. Reached whenever `T` isn't trivially wirable on its own, or an ancestor's context forces
 * something even though `T` would otherwise have taken the fast path above.
 *
 * @tparam T The aggregate type to serialize. Must satisfy `wirable_class`.
 * @param out Output buffer large enough to hold the serialized data.
 * @param value The object to serialize.
 * @return Number of bytes written to the buffer, including any trailing padding -- matching `T`'s
 *         wire size exactly, the same way the memcpy fast path's `sizeof(value)` always does.
 */
template<wirable_class T, context Ctx>
  requires(not custom_wirable<T> and not wirable_range<T> and (not trivially_wirable<T> or Ctx != context {}))
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  using std::ranges::to;

  static constexpr auto local   = merge_context(Ctx, ^^T);
  static constexpr auto wire    = get_wire_layout<T, local>();
  static constexpr auto members = nsdm(^^T) | to<static_array>();

  template for (constexpr auto [layout, member]: std::views::zip(wire.members, members)) {
    using member_type = [:type_of(member):];
    serialize<member_type, merge_context(local, member)>(
        out.subspan<layout.offset.bytes, layout.size>(), value.[:member:]
    );
  }

  return wire.size;
}

/**
 * @brief Serializes a wirable range (`std::array`, a bounded C array, ...) element-by-element.
 *
 * Reached whenever the range's elements aren't uniformly memcpy-able as one block: either they
 * aren't trivially wirable on their own, or an ancestor's context forces a byte order onto them that
 * their in-memory representation doesn't already have. Each element goes through its own dispatch, so
 * a range of aggregates or of primitives needing a byte swap both work correctly.
 *
 * @tparam T The range type to serialize. Must satisfy `wirable_range`.
 * @param out Output buffer large enough to hold the serialized data.
 * @param value The range to serialize.
 * @return Number of bytes written to the buffer.
 */
template<wirable_range T, context Ctx>
  requires(not trivially_wirable_range<T> or Ctx != context {})
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  using element_type = std::ranges::range_value_t<T>;

  std::size_t bytes_written = 0;
  for (auto const& element: value) {
    bytes_written += serialize<element_type, Ctx>(out.subspan(bytes_written), element);
  }

  return bytes_written;
}

} // namespace rbe::detail
