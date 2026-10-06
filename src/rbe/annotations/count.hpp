/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file count.hpp
 * @date 06/10/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/annotation.hpp>
#include <rbe/annotations/detail/annotation_info.hpp>
#include <rbe/core/detail/introspection.hpp>
#include <rbe/core/detail/invoke_concept.hpp>
#include <rbe/core/detail/static_string.hpp>

// --- STD ---

namespace rbe {

namespace detail {

template<typename T>
concept has_compile_time_size = requires {
  { std::tuple_size<std::remove_cvref_t<T>>::value };
};

template<typename T>
concept dynamic_owning_contiguous_range = //
    std::ranges::contiguous_range<T> //
    and not std::ranges::view<T> //
    and not std::is_array_v<std::remove_cvref_t<T>> //
    and not has_compile_time_size<T>; //

struct count_dim {
  static constexpr auto kind = dimension_kind::exclusive;
};

struct count_tag {
  using dimension  = count_dim;
  using value_type = static_string;

  static consteval auto check(annotation_info const annotation, std::meta::info const entity) -> bool try {
    // Conditions:
    //   - The pointed member must exist before the annotated field
    //   - The pointed member's type must be convertible to std::size_t
    //   - The annotated field must be an owning contiguous_range whose length is not fixed at compile time
    auto const parent_type        = normalize_type(parent_of(entity));
    auto const pointed_identifier = annotation.value<static_string>();
    auto const member_idx         = nsdm_index(parent_type, entity);
    auto const pointed_idx        = nsdm_index(parent_type, *pointed_identifier);
    auto const pointed_member     = nsdm(parent_type, pointed_idx);

    return (pointed_idx < member_idx) // clang-format off
      and invoke_concept(^^std::convertible_to, {normalize_type(pointed_member), ^^std::size_t})
      and invoke_concept(^^dynamic_owning_contiguous_range, {normalize_type(entity)});
    // clang-format on
  }
  catch (std::meta::exception const& /**/) {
    return false;
  }
};

} // namespace detail

/**
 * @brief Indicates which field of the annotated type holds the count of elements
 * for a variable-length array or sequence.
 */
inline constexpr detail::annotation_kind<detail::count_tag> count {};

} // namespace rbe
