/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file annotated_field.hpp
 * @date 01/09/2026
 * @brief Shared implementation for reading a single annotated field (`id`, `length`) out of a buffer
 */

#pragma once

// --- Includes ---
#include <rbe/core/detail/context.hpp>
#include <rbe/core/metadata_layout.hpp>
#include <rbe/dsrl/detail/deserialize_member.hpp>

// --- STD ---
#include <cstddef>
#include <span>

namespace rbe::detail {

/**
 * @brief Reads the field annotated `Annotation` out of `data`, using `T`'s layout/type for it.
 *
 * `T` only supplies the layout -- it need not be the concrete type `data` actually holds, as long as
 * every candidate type shares the same layout for `Annotation` (`is_msg_list`'s own invariant for
 * `id`/`length`), which is what lets `any_msg` peek at these fields before it knows which concrete
 * alternative it is holding. This is the one place `msg<T>::id()`/`length()` and
 * `any_msg<T>::id()`/`length()` share, instead of duplicating the same body.
 *
 * @tparam Annotation The annotation identifying the field to read (e.g. `rbe::id`, `rbe::length`).
 * @tparam T The type whose layout locates and types `Annotation`'s field.
 * @tparam Ctx The ambient context `T`'s layout is resolved under.
 * @param data Buffer holding at least `T`'s wire representation.
 * @return The deserialized value of the field annotated `Annotation`.
 */
template<auto Annotation, wirable T, context Ctx = context {}>
  requires annotation<decltype(Annotation)>
constexpr auto read_annotated_field(std::span<std::byte const> const data) {
  static constexpr auto member = rbe::get_annotated_member<Annotation>(^^T, Ctx).value();
  using field_type             = [:type_of(member.info):];

  return deserialize_member<field_type, context {.endianness = member.absolute_layout.endianness}>(
      data.subspan<member.absolute_layout.offset.bytes, member.absolute_layout.size>()
  );
}

/**
 * @brief Reads T's `rbe::id` field out of data.
 *
 * @tparam T The type whose layout locates and types the `id` field.
 * @tparam Ctx The ambient context `T`'s layout is resolved under.
 * @param data Buffer holding at least T's wire representation.
 * @return The deserialized value of the field annotated `rbe::id`.
 */
template<wirable T, context Ctx = context {}>
constexpr auto read_id_field(std::span<std::byte const> const data) {
  return read_annotated_field<rbe::id, T, Ctx>(data);
}

/**
 * @brief Reads T's `rbe::length` field out of `data`, falling back to `T`'s own wire size when `T`
 * carries no `length` annotation.
 *
 * @tparam T The type whose layout locates and types the `length` field, if any.
 * @tparam Ctx The ambient context `T`'s wire size is computed under when there is no `length` field.
 * @param data Buffer holding at least T's wire representation.
 * @return The deserialized `length` field, or `wire_size_of(^^T, Ctx)` when `T` has none.
 */
template<wirable T, context Ctx = context {}>
constexpr auto read_length_field(std::span<std::byte const> const data) {
  if constexpr (get_annotated_member<rbe::length>(^^T, Ctx).has_value()) {
    return read_annotated_field<rbe::length, T, Ctx>(data);
  }
  else {
    return wire_size_of(^^T, Ctx);
  }
}

} // namespace rbe::detail
