/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file memory_layout2.hpp
 * @date 22/10/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/alignment.hpp>
#include <rbe/annotations/endianness.hpp>
#include <rbe/core/detail/context.hpp>
#include <rbe/core/detail/static_array.hpp>
#include <rbe/core/detail/static_sized_range.hpp>
#include <rbe/core/endian.hpp>
#include <rbe/core/wirable_concepts2.hpp>

// --- STD ---
#include <cstddef>
#include <ranges>
#include <type_traits>

// --- System ---


namespace rbe::detail {

struct empty_type { };
using member_offset = std::meta::member_offset;

// layout for primitive types, including their size and endianness
struct primitive_layout {
  std::size_t size {};
  endian::order endianness {std::endian::little};

  constexpr bool operator==(primitive_layout const&) const = default;
};

// layout for members of a class type, including their offset and size
struct member_layout {
  member_offset offset {};
  primitive_layout layout {};

  constexpr bool operator==(member_layout const&) const = default;
};

/**
 * @brief Structure desribing the static layout of a type
 *
 * The idea is that this structure represents the layout of a type
 * no matter if its size is known at compile time or not, the layout is always known at compile time.
 *
 * To do so it express the offset and size of each member relative to the start of anchor types.
 * Every runtime variable size member is an anchor type. Varialbe size members can be:
 *   - a dynamic sized range (vector, inplace_vector, etc)
 *   - a class type with a dynamic sized range member
 *
 */
struct static_type_layout {
  primitive_layout layout {};
  static_array<member_layout> members {};

  constexpr bool operator==(static_type_layout const& /**/) const = default;
};


template<wirable T>
consteval auto anchor_count_of() -> std::size_t {
  // compile time memoization
  static constexpr auto anchor_count = [](std::meta::info const info) {
    if (not is_class_type(info)) {
      return 0;
    }

    std::size_t count = 0;
    for (auto const& member: nsdm(info)) { // clang-format off
      if (invoke_concept(^^dynamic_sized_range, {type_of(member)})) {
        ++count;
      } else if (is_class_type(type_of(member))) {
        count += anchor_count_of(type_of(member));
      } // clang-format on
    }

    return count;
  }(^^T);

  return anchor_count;
}

template<wirable T>
  requires(anchor_count_of<T>() == 0)
constexpr auto anchors_of(std::span<std::byte const> const buffer) -> type_anchors<T> {
  return {};
}

template<wirable T>
constexpr auto anchors_of(std::span<std::byte const> const buffer) -> type_anchors<T> {
  return {}; // TODO: implement anchors_of for types with anchors
}

template<wirable T>
class type_anchors {
public:
  static constexpr auto count = anchor_count_of<T>();
  static constexpr auto empty = count == 0;

  using value_type   = T;
  using anchors_type = std::conditional_t<empty, empty_type, std::array<std::size_t, anchor_count>>;
  using size_type    = std::size_t;
  using buffer_type  = std::span<std::byte const>;

  explicit constexpr type_anchors() noexcept
    requires(empty)
  = default;

  explicit constexpr type_layout(buffer_type buffer) noexcept : anchors_(anchors_of<T>(buffer)) { }

  template<static_string Name>
  constexpr auto for_member() -> std::size_t {
    return for_member<nsdm_index(^^T, Name.get())>();
  }

  template<std::size_t Index>
  constexpr auto for_member() -> std::size_t {
    // return the anchor corresponding to the member at Index
    // if the member is right before any anchor member, this
    // function returns 0
    if constexpr (not empty) {
      // TODO: implement for_member for types with anchors
      return 0;
    }

    return 0;
  }

private:
  [[no_unique_address]] value_type anchors_ {};
};


// NOTE: This type can dangerously grow in size if the type has many
// anchor members. We need to warn the user that proxy might
// get fat if the type has many anchor members.
// An option would be to SOO
template<wirable T, context Ctx = context {}>
class type_layout {
public:
  // --- Type traits ---

  using value_type   = T;
  using anchors_type = type_anchors<T>;
  using buffer_type  = std::span<std::byte const>;
  using size_type    = std::size_t;

  // --- Static members ---

  static constexpr auto static_layout = static_type_layout { }

  // --- Constructors ---

  explicit constexpr type_layout() noexcept = default;

  explicit constexpr type_layout(buffer_type buffer) noexcept : anchors_(buffer) { }

  // --- Member functions ---

  template<static_string Name>
    requires(static_layout.members.size() > 0)
  [[nodiscard]] constexpr auto member() -> member_layout {
    return member<nsdm_index(^^T, Name.get())>();
  }

  template<std::size_t Index>
    requires(static_layout.members.size() > 0)
  [[nodiscard]] constexpr auto member() -> member_layout {
    static constexpr auto relative_layout = static_layout.members[Index];
    auto const base                       = anchor_offset<Index>();
    return { .offset = base + relative_layout.offset, .layout = relative_layout.layout, }
  }

  // TODO: Maybe I would return here the solved layout
  // [[nodiscard]] constexpr auto layout() const noexcept -> primitive_layout { return static_layout.layout; }

  [[nodiscard]] constexpr auto size() const noexcept -> size_type {
    if constexpr (not anchors_type::empty) {
      auto const last_member = static_layout.members.back();
      return last_member.offset + last_member.layout.size + anchors_.back();
    }
    return static_layout.layout.size;
  }

private:
  [[no_unique_address]] anchors_type anchors_ {};
};

} // namespace rbe::detail
