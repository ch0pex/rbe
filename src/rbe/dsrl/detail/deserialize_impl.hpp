/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file deserialize_impl.hpp
 * @date 12/07/2026
 * @brief Context-aware dispatch implementing eager deserialization for wirable types.
 *
 * Internal machinery: fast-path/custom/primitive/aggregate/range overloads threading a `context`
 * through recursive calls so annotations propagate correctly through arbitrarily deep nesting. The
 * public entry point (`rbe::deserialize` for `dsrl::eager_t`, in `rbe/dsrl/deserialize.hpp`) always
 * starts from the default context and dispatches here.
 */

#pragma once

// --- Includes ---
#include <rbe/core/custom.hpp>
#include <rbe/core/detail/context.hpp>
#include <rbe/core/memory_layout.hpp>
#include <rbe/core/trivially_wirable_concepts.hpp>
#include <rbe/core/wirable_concepts.hpp>
#include <rbe/dsrl/tags.hpp>

// --- STD ---
#include <cstddef>
#include <span>

namespace rbe::detail {

// Forward declarations so every overload below can recurse into any sibling regardless of
// definition order -- e.g. the aggregate overload needs to see the range overload, and vice versa.

template<trivially_wirable T, context Ctx = context {}>
  requires(Ctx == context {})
constexpr auto deserialize(std::span<std::byte const>) -> T;

template<custom_wirable T, context Ctx = context {}>
constexpr auto deserialize(std::span<std::byte const>) -> T;

template<trivially_wirable_primitive T, context Ctx>
  requires(Ctx != context {})
constexpr auto deserialize(std::span<std::byte const>) -> T;

template<wirable_class T, context Ctx = context {}>
  requires(
      std::is_default_constructible_v<T> and not custom_wirable<T> and not wirable_range<T> and
      (not trivially_wirable<T> or Ctx != context {})
  )
constexpr auto deserialize(std::span<std::byte const>) -> T;

template<wirable_range T, context Ctx = context {}>
  requires(not trivially_wirable_range<T> or Ctx != context {})
constexpr auto deserialize(std::span<std::byte const>) -> T;

/**
 * @brief Eager deserialization for trivially wirable types with no ambient context forcing anything.
 *
 * Exactly today's fast path: a single direct memory load, no per-member processing.
 *
 * @tparam T The type to deserialize. Must satisfy `trivially_wirable`.
 * @param input A span of bytes containing the serialized data.
 * @return A fully constructed object of type T.
 */
template<trivially_wirable T, context Ctx>
  requires(Ctx == context {})
constexpr auto deserialize(std::span<std::byte const> const input) -> T {
  return load<T>(input);
}

/**
 * @brief Eager deserialization for types with a custom serder.
 *
 * Delegates to the user-provided `custom<T>::deserialize` implementation. The wire format is
 * entirely user-defined, so the ambient context never applies to it.
 *
 * @tparam T The type to deserialize. Must satisfy `custom_wirable`.
 * @param input A span of bytes containing the serialized data.
 * @return A fully constructed object of type T.
 */
template<custom_wirable T, context Ctx>
constexpr auto deserialize(std::span<std::byte const> const input) -> T {
  return rbe::custom<T>::deserialize(input);
}

/**
 * @brief Eager deserialization for a primitive forced to a non-default byte order by an ancestor.
 *
 * @tparam T The primitive type to deserialize. Must satisfy `trivially_wirable_primitive`.
 * @tparam Ctx The ambient context; must not be the default (otherwise the fast path above applies).
 * @param input A span of bytes containing the serialized data.
 * @return A value of type T with bytes swapped to native order.
 */
template<trivially_wirable_primitive T, context Ctx>
  requires(Ctx != context {})
constexpr auto deserialize(std::span<std::byte const> const input) -> T {
  return endian::load<T, Ctx.endianness>(input.data());
}

/**
 * @brief Eager deserialization for non-trivial, non-custom wirable types.
 *
 * Default-constructs the object, then deserializes each non-static data member individually,
 * threading the resolved ambient context one hop further down at each member so annotations
 * propagate correctly through arbitrarily deep unannotated nesting.
 *
 * @tparam T The type to deserialize. Must be default-constructible and wirable.
 * @param input A span of bytes containing the serialized data.
 * @return A fully constructed object of type T.
 */
template<wirable_class T, context Ctx>
  requires(
      std::is_default_constructible_v<T> and not custom_wirable<T> and not wirable_range<T> and
      (not trivially_wirable<T> or Ctx != context {})
  )
constexpr auto deserialize(std::span<std::byte const> const input) -> T {
  using std::ranges::to;

  static constexpr auto local    = merge_context(Ctx, ^^T);
  static constexpr auto wire     = get_wire_layout<T, local>();
  static constexpr auto members  = nsdm(^^T) | std::ranges::to<static_array>();

  T value;
  template for (constexpr auto [layout, member]: std::views::zip(wire.members, members)) {
    using member_type = [:type_of(member):];
    value.[:member:]  = deserialize<member_type, merge_context(local, member)>(
                         input.subspan<layout.offset.bytes, layout.size>()
                     );
  }
  return value;
}

/**
 * @brief Eager deserialization for a wirable range (`std::array`, a bounded C array, ...).
 *
 * Reached whenever the range's elements aren't uniformly loadable as one block: either they aren't
 * trivially wirable on their own, or an ancestor's context forces a byte order onto them that their
 * in-memory representation doesn't already have. Each element goes through its own dispatch.
 *
 * @tparam T The range type to deserialize. Must satisfy `wirable_range`.
 * @param input A span of bytes containing the serialized data.
 * @return A fully constructed range of type T.
 */
template<wirable_range T, context Ctx>
  requires(not trivially_wirable_range<T> or Ctx != context {})
constexpr auto deserialize(std::span<std::byte const> const input) -> T {
  using element_type = std::ranges::range_value_t<T>;

  T value;
  auto remaining = input;
  for (auto& element: value) {
    element   = deserialize<element_type, Ctx>(remaining);
    remaining = remaining.subspan(wire_size_of(^^element_type));
  }
  return value;
}

} // namespace rbe::detail
