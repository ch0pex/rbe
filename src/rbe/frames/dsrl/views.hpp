/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file views.hpp
 * @date 06/10/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/frames/dsrl/concepts.hpp>
#include <rbe/frames/dsrl/many.hpp>

// --- STD ---
#include <concepts>
#include <cstddef>
#include <ranges>
#include <span>
#include <utility>

namespace rbe::frames::views {

namespace detail {

/// Sign-safe for integers (an id read as int against an unsigned literal), plain == for anything else, enums included
template<typename A, typename B>
[[nodiscard]] constexpr auto same_id(A const& a, B const& b) -> bool {
  if constexpr (std::integral<A> and std::integral<B>) {
    return std::cmp_equal(a, b);
  }
  else {
    return a == b;
  }
}

} // namespace detail

template<dsrl::is_frame T>
  requires frames::self_delimited<T>
struct many_fn : std::ranges::range_adaptor_closure<many_fn<T>> {

  [[nodiscard]] constexpr auto operator()(std::span<std::byte const> const data) const -> dsrl::many<T> {
    return dsrl::many<T> {data};
  }
};

template<dsrl::is_frame T>
  requires frames::self_delimited<T>
[[nodiscard]] constexpr auto many() -> many_fn<T> {
  return {};
}

/**
 * @brief Keeps the frames whose header id is any of `ids`
 *
 * A filter-in over std::views::filter, so it composes with the rest of the adaptors:
 * `data | views::many<F>() | views::with_ids(1, 3)`. Unknown ids are matched too, the id is the one on the wire.
 */
template<typename... Ids>
  requires(sizeof...(Ids) > 0)
[[nodiscard]] constexpr auto with_ids(Ids... ids) {
  return std::views::filter([... ids = std::move(ids)](auto const& frame) {
    return (detail::same_id(frame.header().id(), ids) or ...);
  });
}

/// Keeps the frames whose id the code knows, those whose payload is one of the candidates of its any
[[nodiscard]] constexpr auto known_ids() {
  return std::views::filter([](auto const& frame) { return frame.payload().known_id(); });
}

/// Keeps the frames whose id the code does not know, the ones an any would hand to its fallback
[[nodiscard]] constexpr auto unknown_ids() {
  return std::views::filter([](auto const& frame) { return not frame.payload().known_id(); });
}

} // namespace rbe::frames::views

/// The short spelling, the way std::views names std::ranges::views: the adaptors live in
/// rbe::frames::views and this is what a pipeline writes. An alias and nothing else, so there is a single
/// home for them -- rbe::views cannot hold members of its own as long as this exists.
namespace rbe {
namespace views = frames::views;
} // namespace rbe
