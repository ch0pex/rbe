/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file metadata_layout.hpp
 * @date 31/08/2026
 * @brief Locating the `id`/`length` annotated members of a wirable type, wherever they are nested
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/base.hpp>
#include <rbe/annotations/detail/utils.hpp>
#include <rbe/annotations/metadata.hpp>
#include <rbe/core/detail/context.hpp>
#include <rbe/core/detail/introspection.hpp>
#include <rbe/core/detail/static_array.hpp>
#include <rbe/core/memory_layout.hpp>

// --- STD ---
#include <meta>
#include <optional>
#include <ranges>
#include <type_traits>

namespace rbe {

// NOTE: unlike memory_layout.hpp, whose offsets are relative and meant to be consumed recursively
// during (de)serialization, the layouts here are absolute w.r.t. the top-level type -- that's what
// lets metadata be deserialized directly even when it lives several levels deep in the structure.

/**
 * @brief A member_layout together with the reflection of the annotated member itself.
 *
 * Kept separate from member_layout on purpose: member_layout backs struct_layout::members,
 * which is meant to survive past compile time
 * for ordinary runtime use carrying a `std::meta::info` in it would tie that plain POD to
 * reflection semantics. `annotated_member` only ever lives transiently, at the call site that needs
 * to name the annotated field's own type (e.g. to deserialize just that field).
 */
struct annotated_member {
  std::meta::info info;
  member_layout absolute_layout;
};

/**
 * @brief `annotated_member` for `Annotation` within `info`, descending into nested structs as needed.
 *
 * `get_wire_layout` deliberately keeps every offset relative to its own struct -- that's what lets
 * (de)serialization recurse via plain `subspan`s -- so this walks that same tree but accumulates the
 * relative offsets back into one absolute offset as the recursion unwinds, one nesting level at a
 * time. That accumulation happens identically at every level, so it generalizes to any nesting depth
 * with no extra bookkeeping. `Annotation` must be globally unique within `info` (`metadata_dim`'s
 * contract): with more than one match, whichever is found first in declaration order wins.
 */
template<auto Annotation>
  requires detail::annotation<decltype(Annotation)>
consteval auto get_annotated_member(std::meta::info info, detail::context ctx = {}) -> std::optional<annotated_member> {
  // if not class type, cannot have members, so no annotated member can be found
  if (not is_class_type(info)) {
    return std::nullopt;
  }

  auto const wire = get_wire_layout(info, ctx);

  for (auto const [member, layout]: std::views::zip(detail::nsdm(info), wire.members)) {
    if (detail::has_annotation(member, Annotation)) {
      return std::optional<annotated_member> {annotated_member {.info = member, .absolute_layout = layout}};
    }

    // member's own annotation wins over ctx, exactly like get_wire_layout does before recursing.
    auto const nested = get_annotated_member<Annotation>(type_of(member), detail::merge_context(ctx, member));
    if (nested.has_value()) {
      return std::optional<annotated_member> {annotated_member {
        .info            = nested->info,
        .absolute_layout = member_layout {
          // nested->absolute_layout.offset is relative to `member`'s own type; add this member's
          // (relative) offset within `info` to turn it into an offset absolute w.r.t. the top-level
          // call's `info`. The reported `.member` stays the *innermost* one (nested->member) -- that's
          // the reflection whose type/annotations actually describe the field callers care about.
          .offset =
              member_offset {
                .bytes = layout.offset.bytes + nested->absolute_layout.offset.bytes,
                .bits  = layout.offset.bits + nested->absolute_layout.offset.bits,
              },
          .size       = nested->absolute_layout.size,
          .endianness = nested->absolute_layout.endianness,
        },
      }};
    }
  }

  return std::nullopt;
}

/**
 * @brief The value a default-constructed object carries in the field annotated `Annotation`.
 *
 * Concrete message types fix this via a member initializer (e.g. a nested `Header header
 * {.msg_type = message_type_t::foo}`), so a default-constructed instance already holds the canonical
 * value that identifies it -- this is what lets code pick the right alternative out of a `msg_list`
 * from nothing but a runtime-observed id, without deserializing the whole message first.
 *
 */
template<auto Annotation>
  requires detail::annotation<decltype(Annotation)>
consteval auto default_annotation_value(auto const& obj) {
  using T                       = std::remove_cvref_t<decltype(obj)>;
  static constexpr auto members = detail::nsdm(^^T) | std::ranges::to<static_array>();

  template for (constexpr auto member: members) {
    if constexpr (detail::has_annotation(member, Annotation)) {
      return obj.[:member:];
    }
    else if constexpr (get_annotated_member<Annotation>(type_of(member)).has_value()) {
      return default_annotation_value<Annotation>(obj.[:member:]);
    }
  }
}

/// Convenience overload for when only the type (not a live instance) is at hand: default-constructs
/// `T` itself, so `T` must be default-constructible (already a `wirable_class` requirement).
template<auto Annotation, typename T>
  requires detail::annotation<decltype(Annotation)>
consteval auto default_annotation_value() {
  return default_annotation_value<Annotation>(T {});
}

/// Plain, comparable layout of `id`/`length` -- what `is_msg_list` needs to check that every message
/// type in a list agrees on where these fields live, without dragging reflection-only state
/// (`annotated_member::member`) into a value that's meant to be compared across unrelated types.
struct metadata_layout {
  std::optional<member_layout> id;
  std::optional<member_layout> length;
};

consteval auto get_id_layout(std::meta::info info, detail::context ctx = {}) -> std::optional<member_layout> {
  auto const member = get_annotated_member<id>(info, ctx);
  return member ? std::optional {member->absolute_layout} : std::nullopt;
}

consteval auto get_length_layout(std::meta::info info, detail::context ctx = {}) -> std::optional<member_layout> {
  auto const member = get_annotated_member<length>(info, ctx);
  return member ? std::optional {member->absolute_layout} : std::nullopt;
}

consteval auto get_metadata_layout(std::meta::info const info, detail::context ctx = {}) -> metadata_layout {
  // TODO: check wirable and well annotated
  return {
    .id     = get_id_layout(info, ctx),
    .length = get_length_layout(info, ctx),
  };
}

template<typename T>
consteval auto get_metadata_layout() -> metadata_layout {
  return get_metadata_layout(^^T);
}

} // namespace rbe
